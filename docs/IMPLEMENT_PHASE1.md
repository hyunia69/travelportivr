# KICC 시나리오 DB 필드 사용 구현 - Phase 1 완료 보고서

## 📋 구현 개요

**구현 일자**: 2025-11-21
**구현 범위**: Phase 1 - 데이터 구조 준비 및 DB 조회 수정
**상태**: ✅ 완료

## 🎯 Phase 1 구현 내용

Phase 1은 DB 필드(RESERVED_3, RESERVED_4, RESERVED_5)를 로드하고 검증하는 기반 구조를 구축하는 단계입니다.

### Step 1.1: 클래스 멤버 변수 추가 ✅

**파일**: `KICC_Scenario_Travelport\KICC_Scenario_Travelport.h`

**추가된 멤버 변수**:
```cpp
// ========================================
// [2025-11-21 NEW] DB 기반 카드정보 필드
// ========================================
char m_szDB_CardPrefix[12 + 1];     // RESERVED_4: 카드번호 앞 12자리
char m_szDB_ExpireDate[4 + 1];      // RESERVED_3: 유효기간 YYMM
char m_szDB_InstallPeriod[2 + 1];   // RESERVED_5: 할부개월
BOOL m_bUseDbCardInfo;              // DB 필드 사용 여부 (TRUE/FALSE)
```

**위치**: 클래스 public 멤버 영역, `m_PayResult` 변수 다음

**설명**:
- `m_szDB_CardPrefix`: RESERVED_4에서 로드한 카드번호 앞 12자리를 저장
- `m_szDB_ExpireDate`: RESERVED_3에서 로드한 유효기간(YYMM, 4자리)을 저장
- `m_szDB_InstallPeriod`: RESERVED_5에서 로드한 할부개월(2자리)을 저장
- `m_bUseDbCardInfo`: DB 필드가 유효한 경우 TRUE, 검증 실패 시 FALSE로 설정하여 기존 방식으로 폴백

---

### Step 1.2: ScenarioInit() 함수 초기화 ✅

**파일**: `KICC_Scenario_Travelport\KICC_Scenario_Travelport.cpp`
**함수**: `CKICC_Scenario::ScenarioInit()` (라인: 1792)

**추가된 초기화 코드**:
```cpp
// ========================================
// [2025-11-21 NEW] DB 카드정보 필드 초기화
// ========================================
memset(m_szDB_CardPrefix, 0x00, sizeof(m_szDB_CardPrefix));
memset(m_szDB_ExpireDate, 0x00, sizeof(m_szDB_ExpireDate));
memset(m_szDB_InstallPeriod, 0x00, sizeof(m_szDB_InstallPeriod));
m_bUseDbCardInfo = TRUE;  // 기본값: DB 사용 (FALSE로 변경하면 기존 방식)
```

**위치**: `ScenarioInit()` 함수 끝부분, `return 0;` 직전

**동작**:
1. 모든 DB 필드 버퍼를 0으로 초기화
2. `m_bUseDbCardInfo`를 TRUE로 설정하여 DB 사용 모드를 기본으로 설정
3. 이후 DB 조회 시 필드가 유효하지 않으면 FALSE로 변경되어 기존 입력 방식으로 폴백

---

### Step 1.3: DB 조회 함수 수정 ✅

**파일**: `KICC_Scenario_Travelport\ADODB.cpp`
**함수**: `sp_getKiccOrderInfoByTel2()` (라인: 1139)

**수정 위치**: 라인 1283 이후 (ADMIN_ID 로드 다음)

**추가된 코드**:
```cpp
// ========================================
// [2025-11-21 NEW] RESERVED 필드 저장
// ========================================
pScenario->m_AdoDb->GetRs(_variant_t(L"RESERVED_3"), bt);
strncpy(pScenario->m_szDB_ExpireDate, (char*)(_bstr_t)bt, sizeof(pScenario->m_szDB_ExpireDate) - 1);

pScenario->m_AdoDb->GetRs(_variant_t(L"RESERVED_4"), bt);
strncpy(pScenario->m_szDB_CardPrefix, (char*)(_bstr_t)bt, sizeof(pScenario->m_szDB_CardPrefix) - 1);

pScenario->m_AdoDb->GetRs(_variant_t(L"RESERVED_5"), bt);
strncpy(pScenario->m_szDB_InstallPeriod, (char*)(_bstr_t)bt, sizeof(pScenario->m_szDB_InstallPeriod) - 1);
```

**설명**:
- DB 저장 프로시저 `sp_getKiccOrderInfoByTel2`의 결과셋에서 RESERVED_3, RESERVED_4, RESERVED_5 컬럼을 읽어옴
- 각 필드를 해당 멤버 변수에 저장
- 기존 필드(order_no, terminal_id 등)와 동일한 방식으로 처리

**중요**: DB 저장 프로시저가 RESERVED_3, RESERVED_4, RESERVED_5 컬럼을 반환하도록 수정되어야 합니다.

---

### Step 1.4: DB 필드 검증 로직 추가 ✅

**파일**: `KICC_Scenario_Travelport\ADODB.cpp`
**함수**: `sp_getKiccOrderInfoByTel2()` (라인: 1139)

**위치**: RESERVED 필드 로드 직후

**추가된 검증 코드**:
```cpp
// [2025-11-21 NEW] DB 필드 유효성 검증
if (strlen(pScenario->m_szDB_CardPrefix) != 12) {
    xprintf("[CH:%03d] [KICC] RESERVED_4 카드번호 앞자리 길이 오류: %d자리 (기대: 12자리)",
        ch, strlen(pScenario->m_szDB_CardPrefix));
    pScenario->m_bUseDbCardInfo = FALSE;  // DB 사용 불가
}

if (strlen(pScenario->m_szDB_ExpireDate) != 4) {
    xprintf("[CH:%03d] [KICC] RESERVED_3 유효기간 길이 오류: %d자리 (기대: 4자리)",
        ch, strlen(pScenario->m_szDB_ExpireDate));
    pScenario->m_bUseDbCardInfo = FALSE;
}

if (pScenario->m_bUseDbCardInfo) {
    xprintf("[CH:%03d] [KICC] DB 필드 로드 완료", ch);
    xprintf("[CH:%03d]   - RESERVED_3 (유효기간): %s", ch, pScenario->m_szDB_ExpireDate);
    xprintf("[CH:%03d]   - RESERVED_4 (카드앞자리): %s", ch, pScenario->m_szDB_CardPrefix);
    xprintf("[CH:%03d]   - RESERVED_5 (할부개월): %s", ch, pScenario->m_szDB_InstallPeriod);
    xprintf("[CH:%03d]   - DB 사용 모드: ON", ch);
}
else {
    xprintf("[CH:%03d] [KICC] DB 필드 검증 실패 - 기존 입력 방식 사용", ch);
}
```

**검증 규칙**:
1. **RESERVED_4 (카드번호 앞자리)**: 정확히 12자리여야 함
2. **RESERVED_3 (유효기간)**: 정확히 4자리(YYMM)여야 함
3. **RESERVED_5 (할부개월)**: 길이 검증은 Phase 2에서 추가 예정 (0~12 범위)

**폴백 메커니즘**:
- 검증 실패 시 `m_bUseDbCardInfo = FALSE`로 설정
- 이후 단계(Phase 2, 3, 4)에서 `m_bUseDbCardInfo` 플래그를 확인하여 기존 입력 방식으로 폴백
- 사용자는 검증 실패를 알지 못하고 기존 방식으로 계속 진행 가능

**로깅**:
- 검증 실패 시: 채널 번호와 함께 오류 메시지 기록
- 검증 성공 시: 로드된 모든 DB 필드 값을 로그에 출력하여 디버깅 용이성 확보

---

## 🔍 구현 검증 항목

### 코드 수준 검증
- ✅ 멤버 변수가 클래스 헤더에 올바르게 선언됨
- ✅ 초기화 코드가 `ScenarioInit()` 함수에 추가됨
- ✅ DB 조회 함수에서 RESERVED 필드를 로드하는 코드 추가됨
- ✅ 필드 검증 로직이 정상적으로 구현됨
- ✅ 로깅 코드가 추가되어 디버깅 가능

### 빌드 검증 (예정)
- ⏳ Visual Studio 2013에서 컴파일 오류 없이 빌드 완료
- ⏳ Debug 및 Release 구성 모두 빌드 성공
- ⏳ DLL 파일 생성 확인

### 런타임 검증 (예정)
- ⏳ DB에서 RESERVED_3/4/5 필드가 정상적으로 로드됨
- ⏳ 필드 길이 검증이 정상 동작함
- ⏳ 검증 실패 시 폴백 메커니즘이 동작함
- ⏳ 로그 파일에 DB 필드 로드 정보가 기록됨

---

## 📊 테스트 케이스

### Test Case 1: DB 필드 정상 케이스
**전제 조건**:
- DB 테이블에 다음 데이터 존재:
  - `RESERVED_3 = "2512"` (2025년 12월)
  - `RESERVED_4 = "123456789012"` (카드번호 앞 12자리)
  - `RESERVED_5 = "03"` (할부 3개월)

**예상 결과**:
```
[CH:001] [KICC] DB 필드 로드 완료
[CH:001]   - RESERVED_3 (유효기간): 2512
[CH:001]   - RESERVED_4 (카드앞자리): 123456789012
[CH:001]   - RESERVED_5 (할부개월): 03
[CH:001]   - DB 사용 모드: ON
```
- ✅ `m_bUseDbCardInfo = TRUE`
- ✅ 모든 필드가 정상적으로 로드됨

---

### Test Case 2: RESERVED_4 길이 오류 (폴백)
**전제 조건**:
- `RESERVED_3 = "2512"` (정상)
- `RESERVED_4 = "1234567890"` (10자리, 오류)
- `RESERVED_5 = "03"` (정상)

**예상 결과**:
```
[CH:001] [KICC] RESERVED_4 카드번호 앞자리 길이 오류: 10자리 (기대: 12자리)
[CH:001] [KICC] DB 필드 검증 실패 - 기존 입력 방식 사용
```
- ✅ `m_bUseDbCardInfo = FALSE`
- ✅ Phase 2 이후 기존 입력 방식으로 동작

---

### Test Case 3: RESERVED_3 길이 오류 (폴백)
**전제 조건**:
- `RESERVED_3 = "25"` (2자리, 오류)
- `RESERVED_4 = "123456789012"` (정상)
- `RESERVED_5 = "03"` (정상)

**예상 결과**:
```
[CH:001] [KICC] RESERVED_3 유효기간 길이 오류: 2자리 (기대: 4자리)
[CH:001] [KICC] DB 필드 검증 실패 - 기존 입력 방식 사용
```
- ✅ `m_bUseDbCardInfo = FALSE`
- ✅ Phase 2 이후 기존 입력 방식으로 동작

---

### Test Case 4: NULL 값 처리
**전제 조건**:
- `RESERVED_3 = NULL`
- `RESERVED_4 = NULL`
- `RESERVED_5 = NULL`

**예상 결과**:
```
[CH:001] [KICC] RESERVED_4 카드번호 앞자리 길이 오류: 0자리 (기대: 12자리)
[CH:001] [KICC] RESERVED_3 유효기간 길이 오류: 0자리 (기대: 4자리)
[CH:001] [KICC] DB 필드 검증 실패 - 기존 입력 방식 사용
```
- ✅ `m_bUseDbCardInfo = FALSE`
- ✅ Phase 2 이후 기존 입력 방식으로 동작

---

## 🗄️ DB 스키마 요구사항

### KICC_SHOP_ORDER 테이블

**기존 컬럼** (이미 존재):
- `ORDER_NO` (주문번호)
- `TERMINAL_ID` (가맹점 ID)
- `TERMINAL_NM` (가맹점명)
- `CUST_NM` (고객명)
- `GOOD_NM` (상품명)
- `TOTAMOUNT` (금액)
- `PHONE_NO` (전화번호)

**추가 필요 컬럼**:
- `RESERVED_3` VARCHAR(4) - 유효기간 (YYMM, 예: "2512")
- `RESERVED_4` VARCHAR(12) - 카드번호 앞 12자리 (예: "123456789012")
- `RESERVED_5` VARCHAR(2) - 할부개월 (예: "03")

### 저장 프로시저 수정

**프로시저명**: `dbo.sp_getKiccOrderInfoByTel2`

**기존 SELECT 쿼리**:
```sql
SELECT
    ORDER_NO,
    TERMINAL_ID,
    TERMINAL_NM,
    CUST_NM,
    GOOD_NM,
    TOTAMOUNT,
    PHONE_NO,
    SHOP_PW,
    ADMIN_ID
FROM KICC_SHOP_ORDER
WHERE PHONE_NO = @PHONE_NO
  AND ORDER_STATUS = 'PENDING'
```

**수정된 SELECT 쿼리** (RESERVED 컬럼 추가):
```sql
SELECT
    ORDER_NO,
    TERMINAL_ID,
    TERMINAL_NM,
    CUST_NM,
    GOOD_NM,
    TOTAMOUNT,
    PHONE_NO,
    SHOP_PW,
    ADMIN_ID,
    RESERVED_3,    -- [NEW] 유효기간
    RESERVED_4,    -- [NEW] 카드번호 앞자리
    RESERVED_5     -- [NEW] 할부기간
FROM KICC_SHOP_ORDER
WHERE PHONE_NO = @PHONE_NO
  AND ORDER_STATUS = 'PENDING'
```

**중요**: 이 수정이 없으면 애플리케이션이 RESERVED 컬럼을 읽을 수 없어 항상 폴백 모드로 동작합니다.

---

## ⚠️ 주의사항

### 1. DB 스키마 의존성
- **필수**: DB 저장 프로시저 `sp_getKiccOrderInfoByTel2`가 RESERVED_3/4/5 컬럼을 반환해야 함
- **영향**: 저장 프로시저 수정 없이 코드만 배포하면 항상 기존 방식으로 폴백됨
- **조치**: DB 팀과 협력하여 저장 프로시저 수정 및 테이블 컬럼 추가 필요

### 2. 기존 코드 보존
- 기존 입력 방식 코드는 삭제되지 않음
- `m_bUseDbCardInfo` 플래그를 통해 동적으로 선택
- 검증 실패 시 자동으로 기존 방식으로 폴백

### 3. 로그 레벨
- 검증 실패 및 성공 모두 로그에 기록됨
- 운영 환경에서는 로그 레벨 조정 필요 (xprintf 사용)

### 4. 보안 고려사항
- 카드번호 앞 12자리가 로그에 출력됨
- 프로덕션 환경에서는 마스킹 처리 고려 필요
- 예: `123456******` 형식으로 로그 출력

---

## 🚀 다음 단계 (Phase 2)

Phase 1 완료 후 다음 단계:

### Phase 2: 카드번호 입력 로직 수정
**파일**: `KICC_CardInput.cpp`

**작업 내용**:
1. State 0: 카드번호 뒤 4자리만 입력받도록 변경
2. State 1: DB 앞 12자리 + 입력 4자리 = 16자리 완성
3. State 3: DB 사용 시 `KICC_JuminNo(0)` 호출 (유효기간 건너뛰기)

**예상 코드 변경**:
```cpp
if (pScenario->m_bUseDbCardInfo) {
    return send_guide(5);  // 4자리만 입력
}
else {
    return send_guide(17); // 기존 16자리 입력
}
```

### Phase 3: 유효기간 입력 단계 건너뛰기
**파일**: `KICC_EffecDate.cpp`

**작업 내용**:
- 함수 상단에 주석 추가 (DB 모드에서는 호출되지 않음)
- 기존 코드는 보존

### Phase 4: 할부개월 입력 단계 건너뛰기
**파일**: `KICC_JuminNo.cpp`

**작업 내용**:
- State 3에서 DB 사용 시 `KICC_CardPw(0)` 직접 호출
- 할부개월을 DB에서 로드하여 설정

---

## 📞 문의 및 지원

### 배포 전 확인 사항
1. ✅ Phase 1 코드 구현 완료
2. ⏳ DB 저장 프로시저 수정 완료 확인
3. ⏳ DB 테이블에 RESERVED_3/4/5 컬럼 추가 확인
4. ⏳ 테스트 데이터 준비 완료
5. ⏳ 빌드 및 배포 테스트 완료

### 구현 이슈
- DB 저장 프로시저 수정 권한 및 일정 확인 필요
- 테스트 환경 구성 필요
- Phase 2~4 구현 일정 협의 필요

---

**문서 버전**: 1.0
**작성일**: 2025-11-21
**작성자**: Claude Code Implementation
**다음 작업**: Phase 2 구현 (KICC_CardInput.cpp 수정)
