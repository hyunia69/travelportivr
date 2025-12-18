# KICC 다중 주문 결제 처리 - 구현 설계 문서

## 📋 요약

본 문서는 KICC_Scenario_Travelport 결제 게이트웨이 시스템에서 다중 주문 결제 처리 기능을 구현하기 위한 단계별 설계를 제시합니다. 현재 시스템은 전화번호당 단일 주문만 조회하고 처리합니다. 개선 사항은 PHONE_NO와 AUTH_NO 조건에 모두 일치하는 여러 주문을 조회하고, 단일 카드 입력 플로우로 모든 일치하는 주문을 처리할 수 있도록 합니다.

## 🎯 요구사항 개요

### 현재 동작
- 시스템은 PHONE_NO와 일치하는 **1개의 주문**만 조회
- 통화 세션당 단일 결제 승인
- 일대일 관계: 전화번호 → 주문

### 목표 동작
- 시스템은 PHONE_NO로 **최근 주문 1건**을 먼저 조회
- 해당 주문의 AUTH_NO를 추출
- PHONE_NO + AUTH_NO가 모두 일치하는 **여러 주문**을 조회
- 단일 ARS 입력 플로우 (카드번호, 주민번호, 비밀번호 한번만 입력)
- 다중 결제 승인 (조회된 주문당 하나씩)
- 일대다 관계: (전화번호 + 추출된 인증번호) → 여러 주문

### 예시 시나리오
```
입력: PHONE_NO: 01011111111
1단계: 최근 주문 조회 → ORDER_NO: 3333, AUTH_NO: 1111 추출
2단계: AUTH_NO: 1111로 재조회 → 주문: 1111, 2222, 3333 (3건 결제)

입력: PHONE_NO: 01022222222
1단계: 최근 주문 조회 → ORDER_NO: 5555, AUTH_NO: 2222 추출
2단계: AUTH_NO: 2222로 재조회 → 주문: 4444, 5555 (2건 결제)

입력: PHONE_NO: 01033333333
1단계: 최근 주문 조회 → ORDER_NO: 6666, AUTH_NO: 3333 추출
2단계: AUTH_NO: 3333로 재조회 → 주문: 6666 (1건 결제)
```

## 🏗️ 시스템 아키텍처

### 컴포넌트 개요
```
┌─────────────────────────────────────────────────────────────┐
│                    IVR 통화 플로우                           │
├─────────────────────────────────────────────────────────────┤
│  1. PHONE_NO 입력 (ARS)                                     │
│  2. DB 조회 1단계: PHONE_NO로 최근 주문 1건 조회            │
│  3. AUTH_NO 추출 (조회된 주문에서)                          │
│  4. DB 조회 2단계: PHONE_NO + AUTH_NO로 모든 주문 조회      │
│  5. 복수 주문 확인                                          │
│  6. TTS 안내: "N건의 주문 총 금액 ₩XX,XXX"                 │
│  7. 카드 입력 (단일 플로우)                                 │
│     - 카드번호 뒤 4자리                                     │
│     - 주민번호 / 사업자번호                                 │
│     - 비밀번호 (2자리)                                      │
│  8. 결제 루프: N개 주문 처리                                │
│  9. 결과 요약                                               │
└─────────────────────────────────────────────────────────────┘
```

## 📊 데이터 구조 설계

### Phase 1: 기존 구조 확장

#### 1.1 주문 정보 배열 구조
**위치**: `KICC_Common.h`

**새로운 구조 정의**:
```cpp
// 다중 주문 지원: 여러 주문 정보를 위한 배열 구조
typedef struct MultiOrderInfo
{
    int nOrderCount;                    // 조회된 주문 개수
    Card_ResInfo orders[MAX_ORDERS];    // 주문 정보 배열
    int nTotalAmount;                   // 모든 주문의 금액 합계
    int nProcessedCount;                // 성공적으로 처리된 주문 개수
    int nFailedCount;                   // 실패한 주문 개수
    char szFailedOrders[512];           // 실패한 주문번호 목록 (쉼표 구분)
} MultiOrderInfo;

#define MAX_ORDERS 10  // PHONE_NO + AUTH_NO 조합당 최대 주문 개수
```

#### 1.2 CKICC_Scenario 클래스 확장
**위치**: `KICC_Scenario_Travelport.h`

**새로운 멤버 변수**:
```cpp
class CKICC_Scenario : public IScenario
{
public:
    // ... 기존 멤버 ...

    // [다중 주문 지원 - 신규]
    MultiOrderInfo   m_MultiOrders;        // 다중 주문 컨테이너
    int              m_nCurrentOrderIdx;   // 현재 처리중인 주문 인덱스
    BOOL             m_bMultiOrderMode;    // 플래그: 다중 주문 처리 모드
    char             m_szAuthNo[12 + 1];   // SP에서 추출된 AUTH_NO 저장용 (로깅/디버깅)

    // ... 기존 메서드 ...
};
```

**참고**:
- `m_szAuthNo`는 SP에서 자동으로 추출된 AUTH_NO를 저장하는 용도로만 사용됩니다.
- SP 호출시 입력 매개변수로 사용되지 않고, 로깅 및 디버깅에 활용됩니다.
- `Card_ResInfo` 구조체에 AUTH_NO 필드를 추가하거나, `FetchMultiOrderResults()`에서 m_szAuthNo에 직접 저장할 수 있습니다.

### Phase 2: 데이터베이스 레이어 변경

#### 2.1 새로운 저장 프로시저
**데이터베이스**: SQL Server 저장 프로시저 수정

**현재 SP**: `dbo.sp_getKiccOrderInfoByTel2`
- 매개변수: `@PHONE_NO`, `@DNIS`
- 반환: 단일 레코드 (TOP 1, 최근 주문)
- 테이블: `KICC_SHOP_ORDER` (A), `COMMON_DNIS_MID` (B)
- 조건: 7일 이내, payment_code='0', phone_no 일치

**새로운 SP**: `dbo.sp_getKiccMultiOrderInfo`
- 매개변수: `@PHONE_NO`, `@DNIS` (AUTH_NO는 내부에서 자동 추출)
- 반환: 복수 레코드 (동일 AUTH_NO를 가진 모든 주문)
- 로직: 2단계 조회
  1. PHONE_NO로 최근 주문 1건 조회하여 AUTH_NO 추출
  2. 추출된 AUTH_NO + PHONE_NO로 모든 주문 조회

**SQL 구현**:
```sql
CREATE PROCEDURE dbo.sp_getKiccMultiOrderInfo
    @PHONE_NO VARCHAR(32),
    @DNIS VARCHAR(12)
AS
BEGIN
    SET NOCOUNT ON;

    -- 변수 선언: 추출할 AUTH_NO 저장용
    DECLARE @AUTH_NO VARCHAR(12);

    -- [1단계] PHONE_NO로 가장 최근 주문 1건 조회하여 AUTH_NO 추출
    SELECT TOP 1
        @AUTH_NO = A.AUTH_NO  -- AUTH_NO 컬럼에서 인증번호 추출
    FROM KICC_SHOP_ORDER A
    INNER JOIN dbo.COMMON_DNIS_MID B
        ON A.terminal_id = B.SHOP_ID
    WHERE
        DATEDIFF(dd, A.reg_date, GETDATE()) <= 7 AND
        B.ARS_DNIS = @DNIS AND
        A.payment_code = '0' AND
        A.phone_no = @PHONE_NO
    ORDER BY A.reg_date DESC;

    -- AUTH_NO가 없으면 빈 결과 반환
    IF @AUTH_NO IS NULL
    BEGIN
        SELECT TOP 0
            A.order_no,
            A.terminal_id,
            A.terminal_pw,
            A.terminal_nm,
            A.cust_nm,
            A.good_nm,
            A.amount,
            A.phone_no,
            B.SHOP_PW,
            A.ADMIN_ID,
            A.AUTH_NO,
            A.RESERVED_3,
            A.RESERVED_4,
            A.RESERVED_5
        FROM KICC_SHOP_ORDER A
        INNER JOIN dbo.COMMON_DNIS_MID B
            ON A.terminal_id = B.SHOP_ID;
        RETURN;
    END

    -- [2단계] 추출된 AUTH_NO와 PHONE_NO가 모두 일치하는 모든 주문 조회
    SELECT
        A.order_no,
        A.terminal_id,
        A.terminal_pw,
        A.terminal_nm,
        A.cust_nm,
        A.good_nm,
        A.amount,
        A.phone_no,
        B.SHOP_PW,
        A.ADMIN_ID,
        A.AUTH_NO,
        A.RESERVED_3,
        A.RESERVED_4,
        A.RESERVED_5
    FROM KICC_SHOP_ORDER A
    INNER JOIN dbo.COMMON_DNIS_MID B
        ON A.terminal_id = B.SHOP_ID
    WHERE
        DATEDIFF(dd, A.reg_date, GETDATE()) <= 7 AND
        B.ARS_DNIS = @DNIS AND
        A.payment_code = '0' AND
        A.phone_no = @PHONE_NO AND
        A.AUTH_NO = @AUTH_NO  -- 추출된 AUTH_NO로 필터링
    ORDER BY A.reg_date DESC;

END
```

**중요 참고사항**:
- `KICC_SHOP_ORDER` 테이블에 `AUTH_NO` 컬럼이 존재함
- 기존 `sp_getKiccOrderInfoByTel2`와 동일한 테이블 구조 및 조인 사용
- AUTH_NO 컬럼을 통해 동일 인증번호를 가진 여러 주문 조회

#### 2.1.1 1시간 유효기간 버전 (sp_getKiccMultiOrderInfoHour)

**목적**: 주문 유효기간을 7일에서 **1시간**으로 단축한 버전

**차이점**:
| 구분 | sp_getKiccMultiOrderInfo | sp_getKiccMultiOrderInfoHour |
|------|--------------------------|------------------------------|
| 유효기간 | 7일 | 1시간 |
| 조건문 | `DATEDIFF(dd, ...) <= 7` | `DATEADD(HOUR, -1, GETDATE())` |
| 용도 | 일반 결제 | 즉시 결제가 필요한 경우 |

**SQL 구현 (CREATE)**:
```sql
CREATE PROCEDURE dbo.sp_getKiccMultiOrderInfoHour
    @PHONE_NO VARCHAR(32),
    @DNIS VARCHAR(12)
AS
BEGIN
    SET NOCOUNT ON;

    -- 변수 선언: 추출할 AUTH_NO 저장용
    DECLARE @AUTH_NO VARCHAR(12);

    -- [1단계] PHONE_NO로 가장 최근 주문 1건 조회하여 AUTH_NO 추출
    -- 유효기간: 주문 등록 후 1시간 이내
    SELECT TOP 1
        @AUTH_NO = A.AUTH_NO
    FROM KICC_SHOP_ORDER A
    INNER JOIN dbo.COMMON_DNIS_MID B
        ON A.terminal_id = B.SHOP_ID
    WHERE
        A.reg_date >= DATEADD(HOUR, -1, GETDATE()) AND  -- 1시간 이내
        B.ARS_DNIS = @DNIS AND
        A.payment_code = '0' AND
        A.phone_no = @PHONE_NO
    ORDER BY A.reg_date DESC;

    -- AUTH_NO가 없으면 빈 결과 반환
    IF @AUTH_NO IS NULL
    BEGIN
        SELECT TOP 0
            A.order_no,
            A.terminal_id,
            A.terminal_pw,
            A.terminal_nm,
            A.cust_nm,
            A.good_nm,
            A.amount,
            A.phone_no,
            B.SHOP_PW,
            A.ADMIN_ID,
            A.AUTH_NO,
            A.RESERVED_3,
            A.RESERVED_4,
            A.RESERVED_5
        FROM KICC_SHOP_ORDER A
        INNER JOIN dbo.COMMON_DNIS_MID B
            ON A.terminal_id = B.SHOP_ID;
        RETURN;
    END

    -- [2단계] 추출된 AUTH_NO와 PHONE_NO가 모두 일치하는 모든 주문 조회
    -- 유효기간: 주문 등록 후 1시간 이내
    SELECT
        A.order_no,
        A.terminal_id,
        A.terminal_pw,
        A.terminal_nm,
        A.cust_nm,
        A.good_nm,
        A.amount,
        A.phone_no,
        B.SHOP_PW,
        A.ADMIN_ID,
        A.AUTH_NO,
        A.RESERVED_3,
        A.RESERVED_4,
        A.RESERVED_5
    FROM KICC_SHOP_ORDER A
    INNER JOIN dbo.COMMON_DNIS_MID B
        ON A.terminal_id = B.SHOP_ID
    WHERE
        A.reg_date >= DATEADD(HOUR, -1, GETDATE()) AND  -- 1시간 이내
        B.ARS_DNIS = @DNIS AND
        A.payment_code = '0' AND
        A.phone_no = @PHONE_NO AND
        A.AUTH_NO = @AUTH_NO
    ORDER BY A.reg_date DESC;

END
```

**C++ 코드에서 사용 시 변경 사항**:
```cpp
// ADODB.cpp - sp_getKiccMultiOrderInfoHour 호출 시
cmd->CommandText = "dbo.sp_getKiccMultiOrderInfoHour";  // SP 이름 변경
```

**스크립트 파일**: `docs/sp_getKiccMultiOrderInfoHour.txt`

#### 2.2 ADO 데이터베이스 접근 메서드
**위치**: `ADODB.h` 및 `ADODB.cpp`

**새로운 메서드 선언** (`ADODB.h`):
```cpp
class CADODB
{
public:
    // ... 기존 메서드 ...

    // [다중 주문 지원 - 신규]
    int sp_getKiccMultiOrderInfo(
        CString szDnis,
        CString szPhoneNo
        // AUTH_NO는 SP 내부에서 자동 추출됨
    );

    BOOL FetchMultiOrderResults(
        MultiOrderInfo* pMultiOrders
    );
};
```

**구현** (`ADODB.cpp`):
```cpp
int CADODB::sp_getKiccMultiOrderInfo(
    CString szDnis,
    CString szPhoneNo)
{
    if (!ISOpen()) return FALSE;

    try {
        _CommandPtr cmd;
        cmd.CreateInstance("ADODB.Command");
        cmd->CommandText = "dbo.sp_getKiccMultiOrderInfo";
        cmd->CommandType = adCmdStoredProc;

        // 매개변수: @PHONE_NO
        _ParameterPtr pParam;
        pParam.CreateInstance("ADODB.Parameter");
        pParam->Name = L"@PHONE_NO";
        pParam->Type = adVarChar;
        pParam->Size = 32;
        pParam->Direction = adParamInput;
        cmd->Parameters->Append(pParam);
        cmd->Parameters->Item[L"@PHONE_NO"]->Value =
            _variant_t(szPhoneNo.GetBuffer(pParam->Size));

        // 매개변수: @DNIS
        pParam.CreateInstance("ADODB.Parameter");
        pParam->Name = L"@DNIS";
        pParam->Type = adVarChar;
        pParam->Size = 12;
        pParam->Direction = adParamInput;
        cmd->Parameters->Append(pParam);
        cmd->Parameters->Item[L"@DNIS"]->Value =
            _variant_t(szDnis.GetBuffer(pParam->Size));

        cmd->ActiveConnection = m_CONN;

        // 실행 및 레코드셋 열기
        _variant_t vNull;
        vNull.vt = VT_ERROR;
        vNull.scode = DISP_E_PARAMNOTFOUND;

        m_RS = cmd->Execute(&vNull, &vNull, adCmdStoredProc);
        m_RS->CursorLocation = adUseClient;

        return TRUE;
    }
    catch (_com_error e) {
        PrintProviderError();
        PrintComError(e);
        return FALSE;
    }
}

BOOL CADODB::FetchMultiOrderResults(MultiOrderInfo* pMultiOrders)
{
    if (!pMultiOrders || m_RS->adoEOF) return FALSE;

    int nCount = 0;
    memset(pMultiOrders, 0x00, sizeof(MultiOrderInfo));

    try {
        while (!m_RS->adoEOF && nCount < MAX_ORDERS) {
            Card_ResInfo* pOrder = &(pMultiOrders->orders[nCount]);

            // 모든 필드 조회
            GetFieldValue("order_no", pOrder->ORDER_NO, sizeof(pOrder->ORDER_NO));
            GetFieldValue("terminal_nm", pOrder->TERMINAL_NM, sizeof(pOrder->TERMINAL_NM));
            GetFieldValue("terminal_id", pOrder->TERMINAL_ID, sizeof(pOrder->TERMINAL_ID));
            GetFieldValue("terminal_pw", pOrder->TERMINAL_PW, sizeof(pOrder->TERMINAL_PW));
            GetFieldValue("cust_nm", pOrder->CUST_NM, sizeof(pOrder->CUST_NM));
            GetFieldValue("good_nm", pOrder->GOOD_NM, sizeof(pOrder->GOOD_NM));

            long lAmount = 0;
            GetFieldValue("amount", &lAmount);
            pOrder->TOTAMOUNT = (int)lAmount;
            pMultiOrders->nTotalAmount += pOrder->TOTAMOUNT;

            GetFieldValue("phone_no", pOrder->PHONE_NO, sizeof(pOrder->PHONE_NO));
            GetFieldValue("SHOP_PW", pOrder->RESERVED_2, sizeof(pOrder->RESERVED_2));  // SHOP_PW
            GetFieldValue("ADMIN_ID", pOrder->ADMIN_ID, sizeof(pOrder->ADMIN_ID));

            // AUTH_NO 필드 조회 (m_szAuthNo 저장용, Card_ResInfo 구조에 AUTH_NO 필드 추가 필요)
            char szAuthNo[12 + 1];
            GetFieldValue("AUTH_NO", szAuthNo, sizeof(szAuthNo));

            // RESERVED 필드들
            GetFieldValue("RESERVED_3", pOrder->RESERVED_3, sizeof(pOrder->RESERVED_3));
            GetFieldValue("RESERVED_4", pOrder->RESERVED_4, sizeof(pOrder->RESERVED_4));
            GetFieldValue("RESERVED_5", pOrder->RESERVED_5, sizeof(pOrder->RESERVED_5));

            nCount++;
            m_RS->MoveNext();
        }

        pMultiOrders->nOrderCount = nCount;
        return TRUE;
    }
    catch (_com_error e) {
        PrintComError(e);
        return FALSE;
    }
}
```

### Phase 3: 워크플로우 상태 머신 설계

#### 3.1 수정된 통화 플로우
**위치**: `KICC_Scenario_Travelport.cpp`

**현재 플로우**:
```
State 0: PHONE_NO 입력
State 1: 데이터베이스 조회 (단일 주문)
State 2: 주문 확인 (예/아니오)
State 3: 카드 입력
State 4: 결제 처리
State 5: 종료
```

**새로운 다중 주문 플로우**:
```
State 0: PHONE_NO 입력
State 1: 데이터베이스 조회 (복수 주문)
        - SP 내부에서 자동으로 AUTH_NO 추출 및 다중 주문 조회
State 2: 다중 주문 안내 (N건, 총 금액)
State 3: 주문 확인 (예/아니오)
State 4: 카드 입력 (단일 플로우)
State 5: 결제 루프 초기화
State 6: 주문 [0] 처리
State 7: 주문 [1] 처리
...
State N: 주문 [N-1] 처리
State 99: 최종 요약
State 100: 종료
```

#### 3.2 주요 상태 구현

**State 1: 다중 주문 데이터베이스 조회**
```cpp
int KICC_getMultiOrderInfo(int state)
{
    CKICC_Scenario* pScenario = (CKICC_Scenario*)((*lpmt)->pScenario);
    int c = *((*lpmt)->dtmfs);
    (*lpmt)->PrevCall = KICC_getMultiOrderInfo;
    (*lpmt)->prevState = state;

    switch(state)
    {
    case 0:
        // 데이터베이스 조회 시작
        // AUTH_NO는 SP 내부에서 자동으로 추출됨
        info_printf(localCh, "다중 주문 조회: PHONE=%s (AUTH_NO는 SP에서 자동 추출)",
                    pScenario->m_szInputTel);

        setPostfunc(POST_NET, KICC_getMultiOrderInfo, 1, 0);
        return getMultiOrderInfo_host(90);  // 새로운 데이터베이스 호출

    case 1:
        // 조회 결과 확인
        if (pScenario->m_DBAccess == -1) {
            // 데이터베이스 오류
            info_printf(localCh, "다중 주문 조회 실패");
            return KICC_ExitSvc(0);
        }

        if (pScenario->m_MultiOrders.nOrderCount == 0) {
            // 주문 없음
            info_printf(localCh, "주문 조회되지 않음");
            set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\no_order_msg");
            setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
            return send_guide(NODTMF);
        }

        // 조회된 첫 번째 주문에서 AUTH_NO 저장 (로깅용)
        // 참고: Card_ResInfo 구조체에 AUTH_NO 필드 추가 필요
        // 또는 FetchMultiOrderResults()에서 별도로 저장
        if (pScenario->m_MultiOrders.nOrderCount > 0) {
            info_printf(localCh, "조회된 주문 건수: %d, 총 금액: ₩%d",
                       pScenario->m_MultiOrders.nOrderCount,
                       pScenario->m_MultiOrders.nTotalAmount);
        }

        // 주문 개수 및 총 금액 안내
        return KICC_AnnounceMultiOrders(0);
    }

    return RS_OK;
}
```

**State 2: 다중 주문 안내**
```cpp
int KICC_AnnounceMultiOrders(int state)
{
    CKICC_Scenario* pScenario = (CKICC_Scenario*)((*lpmt)->pScenario);
    int c = *((*lpmt)->dtmfs);
    (*lpmt)->PrevCall = KICC_AnnounceMultiOrders;
    (*lpmt)->prevState = state;

    switch(state)
    {
    case 0:
        new_guide();
        info_printf(localCh, "안내: %d건 주문, 총 금액 ₩%d",
                    pScenario->m_MultiOrders.nOrderCount,
                    pScenario->m_MultiOrders.nTotalAmount);

        if (TTS_Play) {
            setPostfunc(POST_NET, KICC_AnnounceMultiOrders, 1, 0);
            return TTS_Play(
                (*lpmt)->chanID, 92,
                "%d건의 주문이 조회되었습니다. 총 결제 금액은 %d원입니다.",
                pScenario->m_MultiOrders.nOrderCount,
                pScenario->m_MultiOrders.nTotalAmount
            );
        }

        return send_guide(NODTMF);

    case 1:
        // 확인 프롬프트
        new_guide();
        set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\input_confirm");
        setPostfunc(POST_DTMF, KICC_AnnounceMultiOrders, 2, 0);
        return send_guide(1);

    case 2:
        // 사용자 확인
        if (!check_validdtmf(c, "12")) {
            return send_error();
        }

        if (c == '1') {
            // 카드 입력으로 진행
            info_printf(localCh, "사용자가 다중 주문 결제 확인");
            return KICC_CardInput(0);
        }
        else {
            // 취소 및 종료
            info_printf(localCh, "사용자가 다중 주문 결제 취소");
            return KICC_ExitSvc(0);
        }
    }

    return RS_OK;
}
```

**State 5-N: 결제 처리 루프**
```cpp
int KICC_ProcessMultiPayments(int state)
{
    CKICC_Scenario* pScenario = (CKICC_Scenario*)((*lpmt)->pScenario);
    (*lpmt)->PrevCall = KICC_ProcessMultiPayments;
    (*lpmt)->prevState = state;

    int nCurrentIdx = pScenario->m_nCurrentOrderIdx;
    int nTotalCount = pScenario->m_MultiOrders.nOrderCount;

    switch(state)
    {
    case 0:
        // 결제 루프 초기화
        pScenario->m_nCurrentOrderIdx = 0;
        pScenario->m_MultiOrders.nProcessedCount = 0;
        pScenario->m_MultiOrders.nFailedCount = 0;
        memset(pScenario->m_MultiOrders.szFailedOrders, 0x00,
               sizeof(pScenario->m_MultiOrders.szFailedOrders));

        info_printf(localCh, "%d건 주문 처리 시작", nTotalCount);
        return KICC_ProcessMultiPayments(1);

    case 1:
        // 모든 주문 처리 완료 확인
        if (nCurrentIdx >= nTotalCount) {
            info_printf(localCh, "모든 주문 처리 완료: 성공=%d, 실패=%d",
                       pScenario->m_MultiOrders.nProcessedCount,
                       pScenario->m_MultiOrders.nFailedCount);
            return KICC_MultiPaymentSummary(0);
        }

        // 현재 주문을 m_CardResInfo에 복사하여 처리
        memcpy(&pScenario->m_CardResInfo,
               &pScenario->m_MultiOrders.orders[nCurrentIdx],
               sizeof(Card_ResInfo));

        info_printf(localCh, "주문 처리중 [%d/%d]: %s, 금액: ₩%d",
                   nCurrentIdx + 1, nTotalCount,
                   pScenario->m_CardResInfo.ORDER_NO,
                   pScenario->m_CardResInfo.TOTAMOUNT);

        // 첫 입력에서 받은 카드 정보 복사 (모든 주문에 공유)
        memcpy(&pScenario->m_CardResInfo.InstPeriod,
               pScenario->m_CardInfo.InstPeriod,
               sizeof(pScenario->m_CardInfo.InstPeriod));

        // 현재 주문에 대한 결제 시작
        setPostfunc(POST_NET, KICC_ProcessMultiPayments, 2, 0);
        return KiccPaymemt_host(90);

    case 2:
        // 결제 결과 확인
        if (pScenario->m_PayResult == 1) {
            // 성공
            pScenario->m_MultiOrders.nProcessedCount++;
            info_printf(localCh, "주문 %s 결제 성공",
                       pScenario->m_CardResInfo.ORDER_NO);
        }
        else {
            // 실패
            pScenario->m_MultiOrders.nFailedCount++;

            // 실패한 주문 목록에 추가
            if (strlen(pScenario->m_MultiOrders.szFailedOrders) > 0) {
                strcat_s(pScenario->m_MultiOrders.szFailedOrders,
                        sizeof(pScenario->m_MultiOrders.szFailedOrders), ", ");
            }
            strcat_s(pScenario->m_MultiOrders.szFailedOrders,
                    sizeof(pScenario->m_MultiOrders.szFailedOrders),
                    pScenario->m_CardResInfo.ORDER_NO);

            info_printf(localCh, "주문 %s 결제 실패: %s",
                       pScenario->m_CardResInfo.ORDER_NO,
                       pScenario->m_CardResInfo.REPLY_MESSAGE);
        }

        // 다음 주문으로 이동
        pScenario->m_nCurrentOrderIdx++;
        return KICC_ProcessMultiPayments(1);
    }

    return RS_OK;
}
```

**State 99: 최종 요약**
```cpp
int KICC_MultiPaymentSummary(int state)
{
    CKICC_Scenario* pScenario = (CKICC_Scenario*)((*lpmt)->pScenario);
    (*lpmt)->PrevCall = KICC_MultiPaymentSummary;
    (*lpmt)->prevState = state;

    switch(state)
    {
    case 0:
        new_guide();

        if (pScenario->m_MultiOrders.nFailedCount == 0) {
            // 모두 성공
            info_printf(localCh, "모든 %d건 주문이 성공적으로 처리됨",
                       pScenario->m_MultiOrders.nProcessedCount);

            if (TTS_Play) {
                setPostfunc(POST_NET, KICC_MultiPaymentSummary, 1, 0);
                return TTS_Play(
                    (*lpmt)->chanID, 92,
                    "총 %d건의 결제가 정상적으로 처리되었습니다.",
                    pScenario->m_MultiOrders.nProcessedCount
                );
            }
        }
        else {
            // 일부 실패
            info_printf(localCh, "부분 성공: 성공=%d, 실패=%d",
                       pScenario->m_MultiOrders.nProcessedCount,
                       pScenario->m_MultiOrders.nFailedCount);

            if (TTS_Play) {
                setPostfunc(POST_NET, KICC_MultiPaymentSummary, 1, 0);
                return TTS_Play(
                    (*lpmt)->chanID, 92,
                    "%d건 결제 완료, %d건 결제 실패. 실패 주문번호: %s",
                    pScenario->m_MultiOrders.nProcessedCount,
                    pScenario->m_MultiOrders.nFailedCount,
                    pScenario->m_MultiOrders.szFailedOrders
                );
            }
        }

        return send_guide(NODTMF);

    case 1:
        // 요약 후 종료
        setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
        return send_guide(NODTMF);
    }

    return RS_OK;
}
```

### Phase 4: 스레드 안전 데이터베이스 접근

#### 4.1 다중 주문 조회 스레드
**위치**: `ADODB.cpp`

```cpp
unsigned int __stdcall getMultiOrderInfoProc(void* arg)
{
    LPMTP* lpmt = (LPMTP*)arg;
    CKICC_Scenario* pScenario = (CKICC_Scenario*)((*lpmt)->pScenario);
    int ch = (*lpmt)->chanID;

    xprintf("[CH:%03d] getMultiOrderInfoProc 시작", ch);

    pScenario->m_DBAccess = 0;

    if (pScenario->m_AdoDb == NULL) {
        pScenario->m_AdoDb = new CADODB;
    }

    if (pScenario->m_AdoDb == NULL) {
        xprintf("[CH:%03d] CADODB 인스턴스 생성 실패", ch);
        pScenario->m_DBAccess = -1;
        (*lpmt)->ppftbl[POST_NET].postcode = HI_COMM;
        _endthreadex(0);
        return 0;
    }

    // 데이터베이스 연결
    if (!pScenario->m_AdoDb->Open()) {
        xprintf("[CH:%03d] 데이터베이스 연결 실패", ch);
        pScenario->m_DBAccess = -1;
        (*lpmt)->ppftbl[POST_NET].postcode = HI_COMM;
        _endthreadex(0);
        return 0;
    }

    // 다중 주문 조회 실행
    // AUTH_NO는 SP 내부에서 자동으로 추출됨
    CString szDnis(pScenario->szDnis);
    CString szPhoneNo(pScenario->m_szInputTel);

    if (!pScenario->m_AdoDb->sp_getKiccMultiOrderInfo(szDnis, szPhoneNo)) {
        xprintf("[CH:%03d] 다중 주문 조회 실패", ch);
        pScenario->m_DBAccess = -1;
        pScenario->m_AdoDb->Close();
        (*lpmt)->ppftbl[POST_NET].postcode = HI_COMM;
        _endthreadex(0);
        return 0;
    }

    // 일치하는 모든 주문 가져오기
    if (!pScenario->m_AdoDb->FetchMultiOrderResults(&pScenario->m_MultiOrders)) {
        xprintf("[CH:%03d] 다중 주문 결과 가져오기 실패", ch);
        pScenario->m_DBAccess = -1;
        pScenario->m_AdoDb->Close();
        (*lpmt)->ppftbl[POST_NET].postcode = HI_COMM;
        _endthreadex(0);
        return 0;
    }

    xprintf("[CH:%03d] %d건 주문 조회됨, 총액: ₩%d",
            ch,
            pScenario->m_MultiOrders.nOrderCount,
            pScenario->m_MultiOrders.nTotalAmount);

    pScenario->m_DBAccess = 1;  // 성공
    pScenario->m_AdoDb->Close();
    (*lpmt)->ppftbl[POST_NET].postcode = HI_OK;

    xprintf("[CH:%03d] getMultiOrderInfoProc 종료", ch);
    _endthreadex(0);
    return 0;
}

int getMultiOrderInfo_host(int holdm)
{
    CKICC_Scenario* pScenario = (CKICC_Scenario*)((*lpmt)->pScenario);

    pScenario->m_DBAccess = 0;

    if (holdm != 0) {
        if (new_guide) new_guide();
        (*lpmt)->trials = 0;
        (*lpmt)->Hmusic = HM_LOOP;
        if (set_guide) set_guide(holdm);
        if (send_guide) send_guide(NODTMF);
    }

    if (pScenario->m_AdoDb != NULL) {
        pScenario->m_DBAccess = -1;
        (*lpmt)->ppftbl[POST_NET].postcode = HI_OK;
        ADO_Quithostio("다중 주문 조회 DB가 이미 열려있음", (*lpmt)->chanID);
        return 0;
    }

    (*lpmt)->ppftbl[POST_NET].postcode = HI_NCMPLT;
    pScenario->m_hThread = (HANDLE)_beginthreadex(
        NULL, 0,
        getMultiOrderInfoProc,
        (LPVOID)(*lpmt),
        0,
        &(pScenario->threadID)
    );

    return 0;
}
```

## 🧪 테스트 전략

### 테스트 케이스 1: 단일 주문 일치
```
입력:
- PHONE_NO: 01033333333
- AUTH_NO: 3333

예상 결과:
- 1건 주문 조회됨 (ORDER_NO: 6666)
- 표준 단일 결제 플로우
- 성공 메시지: "1건의 결제가 완료되었습니다"
```

### 테스트 케이스 2: 복수 주문 일치
```
입력:
- PHONE_NO: 01011111111

1단계 조회:
- 최근 주문 1건 조회 → ORDER_NO: 3333, AUTH_NO: 1111

2단계 조회:
- AUTH_NO: 1111로 재조회 → ORDER_NO: 1111, 2222, 3333 (3건)

예상 결과:
- 3건 주문 조회됨 (ORDER_NO: 1111, 2222, 3333)
- 다중 주문 안내
- 단일 카드 입력
- 3건 결제 승인
- 성공 메시지: "3건의 결제가 정상적으로 처리되었습니다"
```

### 테스트 케이스 3: 주문 없음
```
입력:
- PHONE_NO: 01099999999

1단계 조회:
- 최근 주문 조회 실패 (주문 없음)

예상 결과:
- 0건 주문 조회됨
- 오류 안내: "주문이 조회되지 않았습니다"
- 통화 종료
```

### 테스트 케이스 4: 부분 결제 실패
```
입력:
- PHONE_NO: 01022222222

1단계 조회:
- 최근 주문 1건 조회 → ORDER_NO: 5555, AUTH_NO: 2222

2단계 조회:
- AUTH_NO: 2222로 재조회 → ORDER_NO: 4444, 5555 (2건)

시나리오:
- 2건 주문 조회됨 (ORDER_NO: 4444, 5555)
- 주문 4444 결제 성공
- 주문 5555 결제 실패 (잔액 부족)

예상 결과:
- 요약: "1건 결제 완료, 1건 결제 실패. 실패 주문번호: 5555"
```

### 테스트 케이스 5: 데이터베이스 연결 실패
```
시나리오:
- 조회 중 데이터베이스 사용 불가

예상 결과:
- 오류 안내: "주문 조회 시스템 오류"
- 통화 종료
```

## 📝 구현 체크리스트

### Phase 1: 데이터 구조 준비
- [ ] `KICC_Common.h`에 `MultiOrderInfo` 구조 정의
- [ ] `CKICC_Scenario` 클래스에 멤버 변수 추가
- [ ] 새 멤버 초기화를 위한 생성자 업데이트
- [ ] `MAX_ORDERS` 상수 정의 추가

### Phase 2: 데이터베이스 레이어
- [ ] SQL 저장 프로시저 `sp_getKiccMultiOrderInfo` 생성
- [ ] 샘플 데이터로 저장 프로시저 테스트
- [ ] `CADODB::sp_getKiccMultiOrderInfo()` 메서드 구현
- [ ] `CADODB::FetchMultiOrderResults()` 메서드 구현
- [ ] 스레드 함수 `getMultiOrderInfoProc()` 추가
- [ ] 호스트 인터페이스 `getMultiOrderInfo_host()` 추가

### Phase 3: 상태 머신 로직
- [ ] `KICC_getMultiOrderInfo()` 상태 함수 구현
- [ ] `KICC_AnnounceMultiOrders()` 상태 함수 구현
- [ ] `KICC_ProcessMultiPayments()` 상태 함수 구현
- [ ] `KICC_MultiPaymentSummary()` 상태 함수 구현
- [ ] `KICC_jobArs()`를 다중 주문 플로우로 라우팅하도록 업데이트

### Phase 4: 통합 포인트
- [ ] 기존 `KICC_ArsScenarioStart()` 유지 (AUTH_NO 입력 로직 제거 불필요)
- [ ] 다중 주문 컨텍스트 지원을 위한 카드 입력 플로우 업데이트
- [ ] 결제 게이트웨이 호출이 카드 정보를 재사용하도록 보장
- [ ] 다중 주문 처리를 위한 로깅 추가
- [ ] `Card_ResInfo` 구조체에 AUTH_NO 필드 추가 (선택사항)
- [ ] `FetchMultiOrderResults()`에서 AUTH_NO 값을 조회하여 로깅

### Phase 5: 테스트
- [ ] 단위 테스트: 복수 결과로 데이터베이스 조회
- [ ] 단위 테스트: 3건 주문으로 결제 루프
- [ ] 통합 테스트: 엔드투엔드 통화 플로우
- [ ] 엣지 케이스: MAX_ORDERS 경계
- [ ] 엣지 케이스: 모든 결제 실패
- [ ] 엣지 케이스: 데이터베이스 타임아웃
- [ ] 부하 테스트: 동시 다중 주문 통화

### Phase 6: 배포
- [ ] 운영 데이터베이스에 저장 프로시저 배포
- [ ] Release 구성 빌드
- [ ] 스테이징 환경에서 테스트
- [ ] 롤백 계획 수립
- [ ] 운영 환경에 배포
- [ ] 오류 로그 모니터링

## 🔧 구성 변경사항

### 데이터베이스 구성
**파일**: `KiccPay_Travelport_para.ini` (연결 설정 변경시)

기존 연결 매개변수를 재사용하는 경우 변경 불필요.

### 상수
**파일**: `KICC_Common.h`
```cpp
#define MAX_ORDERS 10  // 설정 가능: 전화번호 + 인증번호당 최대 주문 개수
```

## 📊 성능 고려사항

### 메모리 영향
- **통화당 오버헤드**: ~5KB (10건 주문 × 건당 500바이트 `Card_ResInfo`)
- **최소 영향**: 채널당 구조 할당, 통화 종료시 해제

### 데이터베이스 부하
- **쿼리 복잡도**: 기존 단일 주문 쿼리와 유사
- **네트워크 오버헤드**: 약간 증가 (복수 행 반환)
- **완화 방안**: `(PHONE_NO, AUTH_NO, DNIS)` 인덱스 권장

### 결제 게이트웨이 영향
- **순차 처리**: 주문을 하나씩 처리
- **총 시간**: 선형 증가 (N × 평균 결제 시간)
- **일반적인 경우**: 2-3건 주문 = 총 6-10초 처리

## 🔒 보안 고려사항

### PCI 준수
- **카드 데이터 처리**: 기존 보안 플로우 변경 없음
- **단일 입력**: 카드 번호 한 번만 입력, 모든 주문에 재사용
- **저장 안함**: 카드 데이터는 메모리에만 유지, 통화 종료시 삭제

### 승인
- **주문별 검증**: 각 주문마다 별도의 결제 게이트웨이 승인 필요
- **실패 격리**: 한 건의 결제 실패가 다른 건에 영향 안줌
- **감사 추적**: 모든 거래가 개별적으로 로깅됨

## 🚨 오류 처리

### 데이터베이스 오류
```cpp
if (pScenario->m_DBAccess == -1) {
    // 데이터베이스 연결 또는 조회 실패
    info_printf(localCh, "다중 주문 조회 실패");
    set_guide(399);  // 시스템 오류 안내
    setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
    return send_guide(NODTMF);
}
```

### 결제 게이트웨이 오류
```cpp
if (pScenario->m_PayResult != 1) {
    // 현재 주문 결제 실패
    pScenario->m_MultiOrders.nFailedCount++;

    // 실패 사유 로깅
    info_printf(localCh, "주문 %s 결제 실패: %s",
               pScenario->m_CardResInfo.ORDER_NO,
               pScenario->m_CardResInfo.REPLY_MESSAGE);

    // 다음 주문으로 계속 진행 (전체 배치 중단 안함)
}
```

### 스레드 안전성
- **임계 섹션**: 기존 스레드 안전 ADO 접근 유지
- **공유 상태 없음**: 각 채널은 격리된 `CKICC_Scenario` 인스턴스 보유
- **스레드 라이프사이클**: 결제 스레드는 채널당 관리됨

## 📚 코드 참조

### 수정된 주요 파일
1. **KICC_Common.h** - 데이터 구조 정의
2. **KICC_Scenario_Travelport.h** - 클래스 멤버 추가
3. **KICC_Scenario_Travelport.cpp** - 상태 머신 로직
4. **ADODB.h** - 데이터베이스 메서드 선언
5. **ADODB.cpp** - 데이터베이스 구현 + 스레딩
6. **KICCpayMent.cpp** - 결제 게이트웨이 통합

### 수정된 주요 함수
- `KICC_jobArs()` - 메인 상태 머신 진입점
- `KICC_ArsScenarioStart()` - ARS 플로우 초기화
- `getOrderInfo_host()` - `getMultiOrderInfo_host()`로 **대체됨**

### 추가된 주요 함수
- `KICC_getMultiOrderInfo()` - 다중 주문 조회 상태 핸들러
- `KICC_AnnounceMultiOrders()` - N건 주문 TTS 안내
- `KICC_ProcessMultiPayments()` - 결제 루프 조정자
- `KICC_MultiPaymentSummary()` - 최종 결과 안내
- `getMultiOrderInfoProc()` - 데이터베이스 조회 스레드
- `getMultiOrderInfo_host()` - 데이터베이스 호스트 인터페이스
- `CADODB::sp_getKiccMultiOrderInfo()` - ADO 저장 프로시저 호출
- `CADODB::FetchMultiOrderResults()` - 결과 집합 파서

## 🎓 개발자 노트

### 하위 호환성
- **기존 단일 주문 플로우**: 변경 없이 보존됨
- **새로운 다중 주문 플로우**: 구성 또는 DNIS로 트리거됨
- **데이터베이스 스키마**: 기존 테이블 변경 없음 (새 SP만 추가)

### 향후 개선사항
- **설정 가능한 MAX_ORDERS**: INI 파일로 이동
- **부분 재시도**: 배치 완료 후 실패한 주문 재시도
- **SMS 알림**: 주문별 성공/실패 알림
- **상세 영수증**: SMS를 통한 개별 주문 승인 코드

### 유지보수
- **로깅**: 다중 주문 플로우는 디버깅을 위해 광범위하게 로깅됨
- **모니터링**: 운영 환경에서 통화당 평균 주문 건수 추적
- **성능**: 대량 배치의 결제 처리 시간 모니터링

## 📞 지원 연락처

### 기술 문의
- **IVR 시스템**: WinIVR 개발팀
- **데이터베이스**: SQL Server DBA 팀
- **결제 게이트웨이**: KICC 통합 지원

### 에스컬레이션 경로
1. 개발팀 (구현 문제)
2. QA팀 (테스트 실패)
3. 운영팀 (운영 사고)

---

**문서 버전**: 1.0
**작성자**: 구현 설계팀
**작성일**: 2025-11-22
**상태**: 구현 준비 완료

**승인 필요**:
- [ ] 기술 리더
- [ ] 데이터베이스 관리자
- [ ] QA 매니저
- [ ] 제품 책임자
