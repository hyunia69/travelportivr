# 발신번호(ANI) 기반 주문조회 설계

## 1. 개요

### 1.1 목적
발신번호(ANI: Automatic Number Identification)를 활용하여 고객이 휴대폰 번호를 입력하지 않고도 자동으로 주문정보를 조회하는 기능을 구현한다.

### 1.2 요구사항
1. **발신번호 자동 조회**: 통화 시작 시 발신번호로 주문정보 자동 조회
2. **조회 실패 시 수동 입력**: 주문정보가 없으면 기존 휴대폰 번호 입력 단계로 진입
3. **1회성 적용**: 고객이 주문정보 확인 후 "틀림(2번)"을 누르면 발신번호 조회를 다시 사용하지 않음
4. **기존 흐름 호환**: 기존 휴대폰 번호 입력 로직과 완벽 호환

### 1.3 핵심 원칙
```
발신번호 조회는 최초 1회만 적용
→ 고객이 "틀림"을 누르면 이후 수동 입력만 허용
```

---

## 2. 현재 시스템 분석

### 2.1 발신번호 접근 방법
```cpp
// ScenarioInit() 함수에서 발신번호 설정 (KICC_Travelport_Scenario.cpp:2818)
strncpy(szAni, m_Myport->ani, sizeof(szAni) - 1);
```

| 변수 | 설명 | 접근 방법 |
|------|------|-----------|
| `szAni` | 발신번호 저장 | 멤버 변수 직접 접근 |
| `m_Myport->ani` | WinIVR 제공 원본 값 | 포인터 접근 |
| `szDnis` | 착신번호 (DNIS) | 멤버 변수 직접 접근 |

### 2.2 현재 ARS 시나리오 흐름 (호전환 포함)

```
┌─────────────────┐
│ 1. 통화 시작    │ ← ScenarioInit()에서 szAni, szDnis 설정
└────────┬────────┘
         ▼
┌─────────────────┐
│ 2. case 0       │ ← 인사말 주석처리됨 (실제 재생 없음)
└────────┬────────┘
         ▼
┌─────────────────┐
│ 3. case 1       │ ← 전화번호 입력 안내 멘트
│   전화번호 입력 │
└────────┬────────┘
         ▼
┌─────────────────┐
│ 4. 주문정보 조회│ ← getMultiOrderInfo_host()
└────────┬────────┘
         ▼
┌─────────────────┐
│ 5. 조회 성공?   │ ← KICC_getMultiOrderInfo(1)
└────────┬────────┘
         ▼
    ┌────┴────────────────┐
    │ request_type 분기   │
    └────┬────────────────┘
  ARS ↓            ↓ SMS/TKT
┌──────────────┐  ┌──────────────┐
│ case 10      │  │              │
│ 호전환 안내  │  │              │
│ 멘트 재생    │  │              │
└──────┬───────┘  │              │
       ▼          │              │
┌──────────────┐  │              │
│ case 11      │  │              │
│ 3초 대기     │  │              │
└──────┬───────┘  │              │
       ▼          ▼              │
┌──────────────────┐             │
│ case 12          │◄────────────┘
│ 시스템 인사말    │  "안녕하세요? 항공권 인증 결재 시스템입니다"
└────────┬─────────┘
         ▼
┌─────────────────┐
│ 6. 주문정보 안내│ ← KICC_AnnounceMultiOrders(0)
│   "N건, 총 M원" │
└────────┬────────┘
         ▼
    ┌────┴────┐
    │ 1번/2번 │
    └────┬────┘
    1번 ↓     ↓ 2번
    ┌───┴───┐ ┌──────────────────┐
    │카드입력│ │전화번호 재입력(1)│
    └───────┘ └──────────────────┘
```

### 2.3 호전환 관련 케이스 정리

| Case | 함수 | 내용 | 다음 |
|------|------|------|------|
| 10 | KICC_ArsScenarioStart | 호전환 안내 멘트 재생 | → 11 |
| 11 | KICC_ArsScenarioStart | 3초 대기 (POST_TIME) | → 12 |
| 12 | KICC_ArsScenarioStart | 시스템 인사말 재생 | → KICC_AnnounceMultiOrders(0) |

---

## 3. 설계 방안

### 3.1 새로운 플래그 추가

```cpp
// KICC_Travelport_Scenario.h - CKICC_Scenario 클래스에 추가
class CKICC_Scenario : public IScenario
{
    // ... 기존 멤버 ...

    // ========================================
    // [NEW] 발신번호(ANI) 기반 주문조회 플래그
    // ========================================
    BOOL m_bAniChecked;    // 발신번호 조회 시도 여부 (TRUE: 이미 시도함, FALSE: 미시도)
};
```

### 3.2 플래그 동작 규칙

| 상황 | m_bAniChecked | 동작 |
|------|---------------|------|
| 통화 시작 (초기화) | FALSE | 발신번호 조회 가능 |
| 발신번호로 주문조회 시도 후 | TRUE | 발신번호 조회 불가 |
| 주문정보 확인 후 "2번" 입력 | TRUE (유지) | 휴대폰 번호 입력으로 이동 |
| 휴대폰 번호 입력 실패 재시도 | TRUE (유지) | 발신번호 조회 하지 않음 |

### 3.3 새로운 시나리오 흐름 (호전환 로직 포함)

```
┌─────────────────────┐
│ 1. 통화 시작        │ ← m_bAniChecked = FALSE 초기화
│    case 0           │
└──────────┬──────────┘
           ▼
┌─────────────────────────────────────┐
│ 2. case 20: ANI 분기점              │
│    m_bAniChecked==FALSE             │
│    AND IsValidMobileNumber(szAni)?  │
└──────────┬──────────────────────────┘
      Yes ↓                    ↓ No
      ┌───┴────────────┐  ┌──────────────────┐
      │ ANI로 주문조회 │  │ case 1:          │
      │ m_bAniChecked  │  │ 휴대폰 번호 입력 │ ← 기존 흐름
      │ = TRUE 설정    │  │ (기존 로직 유지) │
      └───────┬────────┘  └────────┬─────────┘
              ▼                    ▼
┌─────────────────────┐  ┌──────────────────────┐
│ 3. case 21:         │  │ 전화번호 입력 완료   │
│    주문 존재?       │  │ → 주문조회           │
└──────────┬──────────┘  │ → KICC_getMultiOrderInfo │
      Yes ↓     ↓ No     └────────┬─────────────┘
      ┌───┴───┐ ┌────────────────────┐       │
      │       │ │ 멘트 없이 case 1로 │       │
      │       │ │ (휴대폰 번호 입력) │       │
      ▼       │ └────────────────────┘       │
┌─────────────┴─────────────────────┐        │
│ 4. request_type 분기              │◄───────┘
│    (기존 로직과 동일)             │
└─────────────┬─────────────────────┘
        ARS ↓            ↓ SMS/TKT
┌──────────────────┐  ┌──────────────────┐
│ case 10:         │  │                  │
│ 호전환 안내 멘트 │  │                  │
└────────┬─────────┘  │                  │
         ▼            │                  │
┌──────────────────┐  │                  │
│ case 11:         │  │                  │
│ 3초 대기         │  │                  │
└────────┬─────────┘  │                  │
         ▼            ▼                  │
┌──────────────────────┐                 │
│ case 12:             │◄────────────────┘
│ 시스템 인사말        │
└──────────┬───────────┘
           ▼
┌─────────────────────┐
│ 5. 주문정보 안내    │ ← KICC_AnnounceMultiOrders(0)
│   "N건, 총 M원"     │
└──────────┬──────────┘
           ▼
      ┌────┴────┐
      │ 1번/2번 │
      └────┬────┘
      1번 ↓           ↓ 2번 (틀림)
      ┌───┴───┐  ┌──────────────────────────────┐
      │카드입력│  │ case 1: 휴대폰 번호 입력    │
      └───────┘  │ m_bAniChecked=TRUE 유지      │
                 │ → 발신번호 조회 하지 않음    │
                 └──────────────────────────────┘
```

---

## 4. 상세 구현 설계

### 4.1 초기화 - 생성자 및 ScenarioInit()

```cpp
// KICC_Travelport_Scenario.cpp - 생성자
CKICC_Scenario::CKICC_Scenario()
{
    // ... 기존 코드 ...

    // [NEW] 발신번호 조회 플래그 초기화
    m_bAniChecked = FALSE;
}

// KICC_Travelport_Scenario.cpp - ScenarioInit()
int CKICC_Scenario::ScenarioInit(LPMTP *Port, char *ArsType)
{
    // ... 기존 코드 ...

    // [NEW] 발신번호 조회 플래그 초기화
    m_bAniChecked = FALSE;

    return 0;
}
```

### 4.2 발신번호 유효성 검사 함수

```cpp
// KICC_Travelport_Scenario.cpp - 새로운 유틸리티 함수
BOOL IsValidMobileNumber(const char* phoneNo)
{
    // 길이 검사: 10~11자리
    int len = strlen(phoneNo);
    if (len < 10 || len > 11) return FALSE;

    // 휴대폰 번호 prefix 검사
    if (strncmp(phoneNo, "010", 3) != 0 &&
        strncmp(phoneNo, "011", 3) != 0 &&
        strncmp(phoneNo, "012", 3) != 0 &&
        strncmp(phoneNo, "016", 3) != 0 &&
        strncmp(phoneNo, "017", 3) != 0 &&
        strncmp(phoneNo, "018", 3) != 0 &&
        strncmp(phoneNo, "019", 3) != 0)
    {
        return FALSE;
    }

    // 숫자만 포함되어 있는지 검사
    for (int i = 0; i < len; i++)
    {
        if (!isdigit(phoneNo[i])) return FALSE;
    }

    return TRUE;
}
```

### 4.3 KICC_ArsScenarioStart() 수정

```cpp
int KICC_ArsScenarioStart(/* [in] */int state)
{
    // ... 기존 변수 선언 ...

    switch (state)
    {
    case 0:
    {
        // 기존 코드: 인사말 (주석처리됨)
        // [MODIFIED] case 0 완료 후 → case 20 (ANI 분기점)으로 이동
        char TempPath[1024 + 1] = { 0x00, };
        new_guide();
        info_printf(localCh, "KICC_ArsScenarioStart [%d] 인사말...", state);
        eprintf("KICC_ArsScenarioStart [%d] 인사말", state);
        // sprintf_s(TempPath, sizeof(TempPath), "audio\\shop_intro\\%s", (*lpmt)->dnis);
        // set_guide(VOC_WAVE_ID, TempPath);  // "인사말"

        // [MODIFIED] 기존: case 1로 이동 → 변경: case 20 (ANI 분기점)으로 이동
        setPostfunc(POST_PLAY, KICC_ArsScenarioStart, 20, 0);
        return send_guide(NODTMF);
    }

    // [NEW] 발신번호 자동 조회 분기점
    case 20:
    {
        // 발신번호 조회 조건 확인
        // 1. 아직 발신번호 조회를 시도하지 않았는가? (m_bAniChecked == FALSE)
        // 2. 발신번호가 유효한 휴대폰 번호인가?

        if (!pScenario->m_bAniChecked && IsValidMobileNumber(pScenario->szAni))
        {
            eprintf("KICC_ArsScenarioStart [%d] 발신번호 자동 조회 시도: ANI=%s",
                    state, pScenario->szAni);

            // 발신번호를 입력 전화번호로 설정
            memset(pScenario->m_szInputTel, 0x00, sizeof(pScenario->m_szInputTel));
            strncpy(pScenario->m_szInputTel, pScenario->szAni, sizeof(pScenario->m_szInputTel) - 1);

            // 발신번호 조회 시도 플래그 설정 (이후 다시 시도하지 않음)
            pScenario->m_bAniChecked = TRUE;

            // 다중 주문 모드 활성화
            pScenario->m_bMultiOrderMode = TRUE;

            // 주문조회 진행 → case 21로 결과 처리
            setPostfunc(POST_NET, KICC_ArsScenarioStart, 21, 0);
            return getMultiOrderInfo_host(90);
        }
        else
        {
            // 발신번호 조회 불가 → 기존 전화번호 입력 단계로 이동
            if (pScenario->m_bAniChecked)
            {
                eprintf("KICC_ArsScenarioStart [%d] 발신번호 이미 조회 시도됨 → 전화번호 입력", state);
            }
            else
            {
                eprintf("KICC_ArsScenarioStart [%d] 발신번호 유효하지 않음 (ANI=%s) → 전화번호 입력",
                        state, pScenario->szAni);
            }

            return KICC_ArsScenarioStart(1);  // 휴대폰 번호 입력 state
        }
    }

    // [NEW] 발신번호 기반 주문조회 결과 처리
    case 21:
    {
        if (pScenario->m_DBAccess == -1 || pScenario->m_MultiOrders.nOrderCount == 0)
        {
            // 주문 없음 또는 DB 오류 → 휴대폰 번호 입력으로 이동
            // [IMPORTANT] ANI 자동 조회 실패 시에는 "주문정보가 없습니다" 멘트 없이
            // 바로 휴대폰 번호 입력 단계로 이동 (사용자 경험 개선)
            eprintf("KICC_ArsScenarioStart [%d] ANI 기반 주문조회 실패/없음 → 전화번호 입력 (멘트 없음)", state);

            // 멘트 없이 바로 휴대폰 번호 입력으로 이동
            return KICC_ArsScenarioStart(1);
        }

        // 주문조회 성공 → 호전환/인사말 흐름으로 진행
        eprintf("KICC_ArsScenarioStart [%d] ANI 기반 주문조회 성공: %d건",
                state, pScenario->m_MultiOrders.nOrderCount);

        // request_type에 따른 분기 (기존 KICC_getMultiOrderInfo의 로직과 동일)
        // ※ 호전환 안내(case 10) → 3초 대기(case 11) → 인사말(case 12) 흐름 유지
        if (strcmp(pScenario->m_szRequestType, "SMS") == 0 ||
            strcmp(pScenario->m_szRequestType, "TKT") == 0)
        {
            // SMS/TKT: 호전환 생략 → 바로 인사말 → 결제안내
            eprintf("KICC_ArsScenarioStart [%d] → 시스템 인사말 (case 12) [request_type=%s]",
                    state, pScenario->m_szRequestType);
            return KICC_ArsScenarioStart(12);
        }
        else
        {
            // ARS: 호전환 안내 → 3초 대기 → 인사말 → 결제안내
            eprintf("KICC_ArsScenarioStart [%d] → 호전환 안내 (case 10) [request_type=%s]",
                    state, strlen(pScenario->m_szRequestType) > 0 ? pScenario->m_szRequestType : "ARS(default)");
            return KICC_ArsScenarioStart(10);
        }
    }

    case 1:
        // 기존 코드 유지 (휴대폰 번호 입력 안내)
        // ...

    // case 10, 11, 12 - 기존 호전환 로직 그대로 유지
    // case 10: 호전환 안내 멘트 재생 → case 11
    // case 11: 3초 대기 (POST_TIME) → case 12
    // case 12: 시스템 인사말 재생 → KICC_AnnounceMultiOrders(0)

    // ... 나머지 case 유지 ...
    }
}
```

### 4.4 호전환 로직 흐름 (case 10, 11, 12)

ANI 기반 주문조회가 성공하면 **기존 호전환 로직을 그대로 사용**합니다.

| Case | 동작 | request_type 분기 |
|------|------|-------------------|
| 10 | 호전환 안내 멘트 재생 | ARS만 해당 |
| 11 | 3초 대기 (POST_TIME) | ARS만 해당 |
| 12 | 시스템 인사말 재생 | ARS, SMS, TKT 모두 |

```cpp
// case 21에서 분기:
// - ARS → case 10 (호전환 안내) → case 11 (3초 대기) → case 12 (인사말)
// - SMS/TKT → case 12 (인사말만, 호전환 생략)
```

### 4.5 KICC_AnnounceMultiOrders() - 2번 입력 시 처리

```cpp
// KICC_AnnounceMultiOrders() 함수 내 case 2
case 2:
    // ... 기존 코드 ...

    if (c == '1') {
        // 카드 입력으로 진행
        return KICC_CardInput(0);
    }
    else {
        // [MODIFIED] 2번 입력 시 휴대폰 번호 입력 단계로 복귀
        // 이 시점에서 m_bAniChecked는 이미 TRUE이므로
        // KICC_ArsScenarioStart(20)으로 가더라도 case 1로 자동 분기됨

        info_printf(localCh, "KICC_AnnounceMultiOrders[%d] 아니오 - 전화번호 재입력 (ANI 조회 안함)", state);
        eprintf("KICC_AnnounceMultiOrders[%d] 아니오 - m_bAniChecked=%d", state, pScenario->m_bAniChecked);

        if (strcmp(pScenario->szArsType, "ARS") == 0) {
            return KICC_ArsScenarioStart(1);  // 직접 case 1로 이동 (휴대폰 번호 입력)
        }
        else if (strcmp(pScenario->szArsType, "SMS") == 0) {
            return KICC_SMSScenarioStart(1);
        }
        else {
            return pScenario->jobArs(0);
        }
    }
```

---

## 5. 흐름도 (최종) - 호전환 로직 포함

```
                         ┌─────────────────────┐
                         │  통화 시작          │
                         │ m_bAniChecked=FALSE │
                         └──────────┬──────────┘
                                    ▼
                         ┌─────────────────────┐
                         │ case 0 (인사말 X)   │ ← 인사말 주석처리됨
                         └──────────┬──────────┘
                                    ▼
                         ┌─────────────────────┐
                         │ case 20: ANI 분기   │
                         └──────────┬──────────┘
                                    ▼
               ┌────────────────────┴────────────────────┐
               │ m_bAniChecked==FALSE                    │
               │ AND IsValidMobileNumber(szAni)?         │
               └────────────────────┬────────────────────┘
                         Yes ↓                  ↓ No
               ┌─────────────┴──────┐  ┌──────────────────────┐
               │ ANI로 주문조회     │  │ case 1: 전화번호 입력│
               │ m_bAniChecked=TRUE │  │ (기존 흐름)          │
               └─────────────┬──────┘  └────────┬─────────────┘
                             ▼                  ▼
               ┌─────────────────────┐  ┌──────────────────────┐
               │ case 21: 주문존재? │  │ 전화번호 입력 완료   │
               └─────────────┬───────┘  │ → 주문조회           │
                     Yes ↓       ↓ No   │ → KICC_getMultiOrderInfo │
               ┌─────────┴────┐  │      └────────┬─────────────┘
               │              │  │               │
               ▼              │  ▼               │
┌──────────────────────────────┴─────────────────┴──────────────┐
│                    request_type 분기                          │
└──────────────────────────────┬────────────────────────────────┘
                     ARS ↓                    ↓ SMS/TKT
           ┌──────────────────────┐  ┌──────────────────────┐
           │ case 10: 호전환 안내 │  │                      │
           │ 멘트 재생            │  │                      │
           └──────────┬───────────┘  │                      │
                      ▼              │                      │
           ┌──────────────────────┐  │                      │
           │ case 11: 3초 대기    │  │                      │
           │ (POST_TIME)          │  │                      │
           └──────────┬───────────┘  │                      │
                      ▼              ▼                      │
           ┌──────────────────────────┐                     │
           │ case 12: 시스템 인사말   │◄────────────────────┘
           │ "안녕하세요? 항공권..."  │
           └──────────┬───────────────┘
                      ▼
           ┌──────────────────────────┐
           │ KICC_AnnounceMultiOrders │
           │ 주문정보 안내            │
           │ "N건, 총 M원"            │
           └──────────┬───────────────┘
                      ▼
                ┌────┴────┐
                │ 1번/2번 │
                └────┬────┘
           1번 ↓               ↓ 2번 (틀림)
     ┌─────────┴─────────┐  ┌───────────────────────────────────┐
     │ KICC_CardInput    │  │ case 1: 전화번호 입력             │
     │ 카드 결제 진행    │  │ m_bAniChecked=TRUE 유지           │
     └───────────────────┘  │ → 발신번호 조회 하지 않음         │
                            │ → 고객이 직접 번호 입력           │
                            └───────────────────────────────────┘
```

### 5.1 호전환 로직 정리

| 상황 | 호전환 (10→11→12) | 인사말만 (12) |
|------|-------------------|---------------|
| ANI 조회 성공, request_type=ARS | ✅ | - |
| ANI 조회 성공, request_type=SMS/TKT | - | ✅ |
| 전화번호 입력 후 조회 성공, ARS | ✅ | - |
| 전화번호 입력 후 조회 성공, SMS/TKT | - | ✅ |

---

## 6. 구현 파일 목록

| 파일 | 수정 내용 |
|------|-----------|
| `KICC_Travelport_Scenario.h` | `m_bAniChecked` 멤버 변수 추가 |
| `KICC_Travelport_Scenario.cpp` | 생성자/ScenarioInit()에서 초기화 |
| `KICC_Travelport_Scenario.cpp` | `IsValidMobileNumber()` 함수 추가 |
| `KICC_Travelport_Scenario.cpp` | `KICC_ArsScenarioStart()` case 20, 21 추가 |
| `KICC_Travelport_Scenario.cpp` | `KICC_ArsScenarioStart()` case 0 수정 (→case 20) |

---

## 7. 테스트 시나리오

### 7.1 정상 케이스: 발신번호로 주문 조회 성공 (ARS 타입)
1. 발신번호 010-1234-5678로 전화 수신
2. 시스템이 자동으로 010-1234-5678로 주문 조회
3. 주문 존재, request_type=ARS
4. **"삼초 이내에 삼자 통화 또는 호전환하시기 바랍니다" 멘트 재생** (case 10)
5. 3초 대기 (case 11)
6. "안녕하세요? 항공권 인증 결재 시스템입니다" 시스템 인사말 재생 (case 12)
7. 주문정보 안내 → 고객이 1번 입력 → 결제 진행

### 7.1-1 정상 케이스: 발신번호로 주문 조회 성공 (SMS/TKT 타입)
1. 발신번호 010-1234-5678로 전화 수신
2. 시스템이 자동으로 010-1234-5678로 주문 조회
3. 주문 존재, request_type=SMS 또는 TKT
4. **호전환 멘트 생략** → 바로 시스템 인사말 재생 (case 12)
5. 주문정보 안내 → 고객이 1번 입력 → 결제 진행

### 7.2 정상 케이스: 주문정보 틀림 후 재입력
1. 발신번호 010-1234-5678로 전화 수신
2. 시스템이 자동으로 주문 조회 → 주문 존재
3. 고객이 2번 입력 (틀림)
4. 휴대폰 번호 입력 안내 (발신번호 조회 하지 않음)
5. 고객이 다른 번호 입력 → 해당 번호로 주문 조회

### 7.3 발신번호 주문 없음
1. 발신번호 010-1234-5678로 전화 수신
2. 시스템이 자동으로 주문 조회 → 주문 없음
3. **멘트 없이** 바로 휴대폰 번호 입력으로 이동 (고객은 ANI 조회가 실패했는지 인지하지 못함)
4. 고객이 번호 입력 → 해당 번호로 주문 조회
5. 주문 없으면 **"주문정보가 없습니다" 멘트 재생** → 재입력 안내

### 7.4 유효하지 않은 발신번호
1. 발신번호 02-1234-5678 (유선전화)로 전화 수신
2. 발신번호가 휴대폰 형식이 아님 → 바로 휴대폰 번호 입력 안내
3. 고객이 번호 입력 → 해당 번호로 주문 조회

### 7.5 발신번호 비어있음
1. 발신번호가 없는 상태로 전화 수신 (szAni = "")
2. 발신번호 조회 불가 → 바로 휴대폰 번호 입력 안내

---

## 8. 음성 파일

| 파일명 | 내용 | 용도 |
|--------|------|------|
| `ment/Travelport/no_order_msg` | "주문정보가 없습니다" | 휴대폰 번호 수동 입력 후 주문조회 실패 시에만 재생 |

> **참고**: ANI 자동 조회 실패 시에는 음성 멘트 없이 바로 휴대폰 번호 입력 단계로 진행

---

## 9. 주의사항

1. **발신번호 형식**: 발신번호는 통신사에 따라 `010XXXXXXXX` 또는 `010-XXXX-XXXX` 형식일 수 있음. 하이픈 제거 로직 필요 시 추가 구현
2. **NULL 체크**: `szAni`가 NULL인 경우 처리 필요
3. **로그 기록**: 발신번호 기반 조회 시 보안을 위해 전체 번호가 아닌 마스킹된 형태로 로그 기록 권장 (예: `010****5678`)
4. **기존 호환성**: `m_bAniChecked` 플래그를 추가하더라도 기존 SMS/TKT 시나리오에 영향 없음

---

## 10. 변경 이력

| 일자 | 버전 | 내용 |
|------|------|------|
| 2025-12-22 | 1.0 | 최초 설계 문서 작성 |
| 2025-12-22 | 1.1 | 호전환 안내 멘트(case 10), 3초 대기(case 11), 인사말(case 12) 로직 반영 |
