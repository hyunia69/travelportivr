# 삼자통화/호전환 대기 안내 멘트 추가 설계

## 1. 개요

### 1.1 목적
주문조회 성공 후, 삼자통화/호전환을 위한 안내 멘트와 대기 시간을 추가하여 상담원이 통화에 참여할 수 있는 시간을 확보합니다.

### 1.2 request_type별 적용 범위
주문 조회 결과의 `request_type` 필드 값에 따라 호전환 멘트 재생 여부가 결정됩니다.

| request_type | 삼자통화 안내 | 3초 대기 | 인사말 | 설명 |
|--------------|--------------|---------|--------|------|
| **ARS** | ✅ 적용 | ✅ 적용 | ✅ 적용 | 상담원 호전환 대기 필요 |
| **SMS** | ❌ 미적용 | ❌ 미적용 | ✅ 적용 | SMS 주문, 호전환 불필요 |
| **TKT** | ❌ 미적용 | ❌ 미적용 | ✅ 적용 | 티켓 주문, 호전환 불필요 |

### 1.3 변경 요구사항
| 항목 | 현재 | 변경 후 (ARS 타입) | 변경 후 (SMS/TKT 타입) |
|------|------|-------------------|----------------------|
| 휴대폰 번호 입력 후 | 즉시 주문조회 → 결제안내 | 주문조회 → 삼자통화 안내 → 3초 대기 → 인사말 → 결제안내 | 주문조회 → 인사말 → 결제안내 |
| 주문조회 실패 시 | 오류 안내 후 종료 | 휴대폰 번호 입력 화면으로 즉시 복귀 | 휴대폰 번호 입력 화면으로 즉시 복귀 |

### 1.4 신규 음성 파일
| 파일명 | 경로 | 멘트 내용 |
|--------|------|----------|
| transfer_guide.wav | ment/Travelport/transfer_guide | "삼초 이내에 삼자 통화 또는 호전환하시기 바랍니다" |
| intro.wav | ment/Travelport/intro | "안녕하세요? 항공권 인증 결재 시스템입니다" |

---

## 2. 현재 흐름 분석

### 2.1 현재 ARS 시나리오 흐름 (SKIP_PHONE_CONFIRM = TRUE 기준)

```
KICC_ArsScenarioStart(0)
    │
    ▼
[Case 0: 인사말]
    → setPostfunc(POST_PLAY, KICC_ArsScenarioStart, 1, 0)
    → send_guide(NODTMF)
    │
    ▼
[Case 1: 전화번호 입력 안내]
    → "ment/Travelport/input_telnum_start" 재생
    → setPostfunc(POST_DTMF, KICC_ArsScenarioStart, 2, 0)
    → send_guide(13)  // 13자리 DTMF 대기
    │
    ▼
[Case 2: 전화번호 검증 및 주문조회]
    → 전화번호 형식 검증
    → m_szInputTel 저장
    → m_bMultiOrderMode = TRUE
    → setPostfunc(POST_NET, KICC_getMultiOrderInfo, 0, 0)
    → getMultiOrderInfo_host(90)  // ◀ 즉시 주문조회 진행
    │
    ▼
[KICC_getMultiOrderInfo: 주문조회 결과 처리]
    → (성공 시) 바로 결제 안내 멘트로 진행
    → (실패 시) 오류 안내 후 종료
```

### 2.2 문제점
- 주문조회 성공 후 즉시 결제 안내로 진행
- 상담원이 삼자통화/호전환할 시간이 없음
- 고객에게 시스템 안내(인사말)가 없음
- 주문조회 실패 시 전화번호 재입력 기회가 없음

---

## 3. 변경 설계

### 3.1 변경된 흐름도 (ARS 타입별 분기)

```
KICC_ArsScenarioStart(0)
    │
    ▼
[Case 0: 초기 인사말] (기존)
    │
    ▼
[Case 1: 전화번호 입력 안내] (기존)
    │
    ▼
[Case 2: 전화번호 검증] (기존 유지)
    → 전화번호 형식 검증
    → m_szInputTel 저장
    → m_bMultiOrderMode = TRUE
    → 주문조회 API 호출
    │
    ▼
[KICC_getMultiOrderInfo: 주문조회 결과 처리] (변경)
    │
    ├── (성공 시) ─────────────────────────────────────
    │       │
    │       ▼
    │   ┌─────────────────────────────────────────────┐
    │   │ ARS 타입 분기 (szArsType 확인)              │
    │   └─────────────────────────────────────────────┘
    │       │
    │       ├── (ARS 타입) ───────────────────────────
    │       │       │
    │       │       ▼
    │       │   [Case 10: 삼자통화 안내] [NEW]
    │       │       → new_guide()
    │       │       → set_guide(VOC_WAVE_ID, "ment/Travelport/transfer_guide")
    │       │         "삼초 이내에 삼자 통화 또는 호전환하시기 바랍니다"
    │       │       → setPostfunc(POST_PLAY, KICC_ArsScenarioStart, 11, 0)
    │       │       → send_guide(NODTMF)
    │       │       │
    │       │       ▼
    │       │   [Case 11: 3초 대기] [NEW]
    │       │       → new_guide()
    │       │       → set_guide(VOC_WAVE_ID, "ment/Travelport/wait_3sec")
    │       │       → setPostfunc(POST_PLAY, KICC_ArsScenarioStart, 12, 0)
    │       │       → send_guide(NODTMF)
    │       │       │
    │       │       ▼
    │       │   [Case 12: 시스템 인사말] [NEW]
    │       │       → (아래 SMS/TKT와 동일)
    │       │
    │       └── (SMS/TKT 타입) ────────────────────────
    │               │
    │               ▼
    │           [Case 12: 시스템 인사말] [NEW]
    │               → new_guide()
    │               → set_guide(VOC_WAVE_ID, "ment/Travelport/intro")
    │                 "안녕하세요? 항공권 인증 결재 시스템입니다"
    │               → setPostfunc(POST_PLAY, 결제안내함수, 0, 0)
    │               → send_guide(NODTMF)
    │               │
    │               ▼
    │           [결제 안내 진행] (기존 흐름)
    │
    └── (실패 시) ─────────────────────────────────────
            │
            ▼
        [Case 1: 전화번호 입력 안내로 복귀] [NEW]
            → 즉시 전화번호 입력 화면으로 이동
            → return KICC_ArsScenarioStart(1);
─────────────────────────────────────────────────────────
```

### 3.2 상태(State) 번호 할당

| State | 역할 | 적용 조건 | 비고 |
|-------|------|----------|------|
| 0 | 초기 인사말 | 전체 | 기존 유지 |
| 1 | 전화번호 입력 안내 | 전체 | 기존 유지 |
| 2 | 전화번호 검증 및 주문조회 | 전체 | 기존 유지 (주문조회 API 호출) |
| 3 | (기존 TTS 확인) | 전체 | SKIP_PHONE_CONFIRM=FALSE 시에만 사용 |
| 4 | (기존 확인 처리) | 전체 | SKIP_PHONE_CONFIRM=FALSE 시에만 사용 |
| **10** | 삼자통화 안내 | **ARS 타입만** | **신규** - 주문조회 성공 후 진입 |
| **11** | 3초 대기 | **ARS 타입만** | **신규** |
| **12** | 시스템 인사말 | 전체 | **신규** - SMS/TKT는 직접 진입 |
| 0xffff | 종료 | 전체 | 기존 유지 |

---

## 4. 코드 변경 상세

### 4.1 KICC_getMultiOrderInfo() 함수 수정

주문조회 결과 처리 부분에서 성공/실패에 따른 분기 추가

#### 4.1.1 주문조회 성공 시 - request_type에 따른 분기

**현재 코드 (주문조회 성공 시):**
```cpp
// 주문조회 성공 - 바로 결제 안내 멘트로 진행
info_printf(localCh, "KICC_getMultiOrderInfo 주문조회 성공, 결제 안내 진행");
// ... 결제 안내 로직 ...
```

**변경 코드:**
```cpp
// [MODIFIED] 주문조회 성공 - 주문의 request_type에 따른 분기 처리
// m_szRequestType: DB에서 조회한 첫 번째 주문의 request_type 값
// 주의: SP에서 request_type이 반환되지 않으면 빈 값이 될 수 있음

// 디버깅: m_szRequestType 값 확인
eprintf("KICC_getMultiOrderInfo[%d] m_szRequestType='%s', len=%d",
        state, pScenario->m_szRequestType, strlen(pScenario->m_szRequestType));

// request_type이 "SMS" 또는 "TKT"인 경우에만 호전환 생략
// 빈 값이거나 "ARS"이면 호전환 멘트 재생 (기본값 = ARS 동작)
if (strcmp(pScenario->m_szRequestType, "SMS") == 0 || strcmp(pScenario->m_szRequestType, "TKT") == 0)
{
    // SMS/TKT 타입: 바로 인사말 → 결제안내 (삼자통화 안내/대기 생략)
    eprintf("KICC_getMultiOrderInfo[%d] → 시스템 인사말 (Case 12) [request_type=%s]", state, pScenario->m_szRequestType);
    return KICC_ArsScenarioStart(12);
}
else
{
    // ARS 타입 또는 빈값(기본값): 삼자통화 안내 → 3초 대기 → 인사말 → 결제안내
    eprintf("KICC_getMultiOrderInfo[%d] → 삼자통화 안내 (Case 10) [request_type=%s]", state,
            strlen(pScenario->m_szRequestType) > 0 ? pScenario->m_szRequestType : "ARS(default)");
    return KICC_ArsScenarioStart(10);
}
```

#### 4.1.2 주문조회 실패 시 - 전화번호 입력으로 복귀

**현재 코드 (주문조회 실패 시):**
```cpp
// 주문조회 실패 - 오류 안내 후 종료
info_printf(localCh, "KICC_getMultiOrderInfo 주문조회 실패");
// ... 오류 안내 및 종료 처리 ...
```

**변경 코드:**
```cpp
// [MODIFIED] 주문조회 실패 - 전화번호 입력 화면으로 즉시 복귀
info_printf(localCh, "KICC_getMultiOrderInfo 주문조회 실패, 전화번호 재입력으로 이동");
eprintf("KICC_getMultiOrderInfo 주문조회 실패 → 전화번호 입력 (Case 1)");

// [NEW] 전화번호 입력 화면으로 복귀 (오류 멘트 없이 바로)
return KICC_ArsScenarioStart(1);
```

### 4.2 KICC_ArsScenarioStart() 함수에 신규 Case 추가

#### 4.2.1 신규 Case 추가 (Case 10, 11, 12)

```cpp
    // [NEW] 삼자통화/호전환 안내 멘트 (주문조회 성공 후 진입)
    case 10:
        new_guide();
        info_printf(localCh, "KICC_ArsScenarioStart [%d] 삼자통화 안내 멘트 재생", state);
        eprintf("KICC_ArsScenarioStart [%d] 삼자통화 안내: 삼초 이내에 삼자 통화 또는 호전환하시기 바랍니다", state);
        set_guide(VOC_WAVE_ID, "ment/Travelport/transfer_guide");  // "삼초 이내에 삼자 통화 또는 호전환하시기 바랍니다"
        setPostfunc(POST_PLAY, KICC_ArsScenarioStart, 11, 0);
        return send_guide(NODTMF);

    // [NEW] 3초 대기
    case 11:
        new_guide();
        info_printf(localCh, "KICC_ArsScenarioStart [%d] 3초 대기 중...", state);
        eprintf("KICC_ArsScenarioStart [%d] 3초 대기 (삼자통화/호전환 대기)", state);
        set_guide(VOC_WAVE_ID, "ment/Travelport/wait_3sec");  // 3초 무음 또는 대기음
        setPostfunc(POST_PLAY, KICC_ArsScenarioStart, 12, 0);
        return send_guide(NODTMF);

    // [NEW] 시스템 인사말 후 결제 안내 진행
    case 12:
        new_guide();
        info_printf(localCh, "KICC_ArsScenarioStart [%d] 시스템 인사말 재생", state);
        eprintf("KICC_ArsScenarioStart [%d] 인사말: 안녕하세요? 항공권 인증 결재 시스템입니다", state);
        set_guide(VOC_WAVE_ID, "ment/Travelport/intro");  // "안녕하세요? 항공권 인증 결재 시스템입니다"
        // 인사말 재생 후 기존 결제 안내 흐름으로 진행
        setPostfunc(POST_PLAY, KICC_MultiOrderAnnounce, 0, 0);  // 또는 기존 결제 안내 함수
        return send_guide(NODTMF);
```

### 4.3 Case 2, Case 4는 변경 없음

주문조회 API 호출은 기존과 동일하게 Case 2에서 수행됩니다.
KICC_getMultiOrderInfo() 함수에서 결과에 따라 분기 처리합니다.

### 4.4 데이터 구조 변경

#### 4.4.1 KICC_Common.h - Card_ResInfo 구조체
```cpp
// [NEW] 주문 요청 타입 (ARS/SMS/TKT)
char REQUEST_TYPE[10 + 1];  // "ARS", "SMS", "TKT"
}Card_ResInfo;
```

#### 4.4.2 KICC_Travelport_Scenario.h - CKICC_Scenario 클래스
```cpp
char             m_szAuthNo[12 + 1];       // SP에서 추출된 AUTH_NO 저장용
char             m_szRequestType[10 + 1];  // [NEW] 첫 주문의 요청 타입 (ARS/SMS/TKT)
```

#### 4.4.3 ADODB.cpp - FetchMultiOrderResults()
```cpp
// [NEW] request_type: 주문 요청 타입 (ARS/SMS/TKT)
bt = "";
GetRs(_variant_t(L"request_type"), bt);
strncpy(pOrder->REQUEST_TYPE, (char*)(_bstr_t)bt, sizeof(pOrder->REQUEST_TYPE) - 1);

// 첫 번째 주문의 request_type을 시나리오에 저장
strncpy(pScenario->m_szRequestType, pOrder->REQUEST_TYPE, sizeof(pScenario->m_szRequestType) - 1);
eprintf("[KICC] 다중주문 request_type: %s", pScenario->m_szRequestType);
```

### 4.5 저장 프로시저(SP) 수정

#### 4.5.1 수정 대상 SP
| SP 이름 | 유효기간 | 파일 위치 |
|---------|---------|----------|
| sp_getKiccMultiOrderInfoHour | 1시간 이내 | docs/sp_getKiccMultiOrderInfoHour.txt |
| sp_getKiccMultiOrderInfo | 7일 이내 | docs/sp_getKiccMultiOrderInfo.txt |

#### 4.5.2 SP 변경 내용
두 SP 모두 SELECT 문에 `request_type` 컬럼 추가:

```sql
-- 기존 SELECT 마지막에 추가
    A.RESERVED_4,
    A.RESERVED_5,
    A.request_type   -- [NEW] ARS/SMS/TKT 구분
FROM KICC_SHOP_ORDER A
```

**적용 방법**: 수정된 SP 파일 내용을 SSMS에서 실행

---

## 5. 음성 파일 요구사항

### 5.1 필수 음성 파일

| 파일 경로 | 멘트 내용 | 예상 길이 | 비고 |
|----------|----------|----------|------|
| ment/Travelport/transfer_guide.wav | "삼초 이내에 삼자 통화 또는 호전환하시기 바랍니다" | 약 3초 | 신규 녹음 필요 |
| ment/Travelport/intro.wav | "안녕하세요? 항공권 인증 결재 시스템입니다" | 약 2.5초 | 신규 녹음 필요 |
| ment/Travelport/wait_3sec.wav | (무음 또는 대기음) | 정확히 3초 | 신규 생성 필요 |

### 5.2 대기음 옵션

wait_3sec.wav 파일의 옵션:

| 옵션 | 설명 | 권장 |
|------|------|------|
| A | 완전 무음 (silence) | 고객이 연결 끊김으로 오해 가능 |
| B | 틱톡 소리 (tick sound) | 대기 중임을 인지 가능 |
| C | 잔잔한 배경음 (hold music) | 자연스러운 대기 경험 |

**권장**: 옵션 C (잔잔한 배경음) - 3초간 대기 중임을 고객이 인지할 수 있음

---

## 6. 변경된 전체 시나리오 흐름도 (ARS 타입별)

```
┌─────────────────┐
│   통화 시작      │
└────────┬────────┘
         │
         ▼
┌─────────────────────────────────┐
│ Case 0: 초기 인사말              │
│ (기존 - 현재 비어있음)            │
└────────────────┬────────────────┘
                 │
                 ▼
┌─────────────────────────────────┐
│ Case 1: 전화번호 입력 안내        │◀──────────────┐
│ "휴대폰 번호를 입력 후            │               │
│  우물 정자를 눌러주세요"          │               │
└────────────────┬────────────────┘               │
                 │                                │
                 ▼                                │
         ┌───────────────┐                        │
         │ 전화번호 입력  │                        │
         └───────┬───────┘                        │
                 │                                │
                 ▼                                │
┌─────────────────────────────────┐               │
│ Case 2: 전화번호 검증 및 주문조회 │               │
│ - 형식 검증                      │               │
│ - m_szInputTel 저장              │               │
│ - m_bMultiOrderMode = TRUE       │               │
│ - 주문조회 API 호출              │               │
└────────────────┬────────────────┘               │
                 │                                │
                 ▼                                │
┌─────────────────────────────────┐               │
│ KICC_getMultiOrderInfo          │               │
│ 주문조회 결과 처리               │               │
└────────┬───────────────┬────────┘               │
         │               │                        │
    (성공)              (실패)                     │
         │               │                        │
         ▼               └────────────────────────┘
┌─────────────────────────────────┐      (즉시 Case 1로 복귀)
│ ARS 타입 분기 (szArsType 확인)   │
└────────┬───────────────┬────────┘
         │               │
   (ARS 타입)       (SMS/TKT 타입)
         │               │
         ▼               │
┌─────────────────────────────────┐               │
│ Case 10: 삼자통화 안내 [NEW]     │               │
│ "삼초 이내에 삼자 통화 또는       │               │
│  호전환하시기 바랍니다"           │               │
│ ※ ARS 타입에만 적용              │               │
└────────────────┬────────────────┘               │
                 │                                │
                 ▼                                │
┌─────────────────────────────────┐               │
│ Case 11: 3초 대기 [NEW]          │               │
│ (무음 또는 대기음)                │               │
│ ※ ARS 타입에만 적용              │               │
└────────────────┬────────────────┘               │
                 │                                │
                 ▼                                │
┌─────────────────────────────────┐◀──────────────┘
│ Case 12: 시스템 인사말 [NEW]     │  (SMS/TKT는 직접 진입)
│ "안녕하세요? 항공권 인증          │
│  결재 시스템입니다"               │
│ ※ 모든 타입 공통                 │
└────────────────┬────────────────┘
                 │
                 ▼
┌─────────────────────────────────┐
│ 다중 주문 결제 안내               │
│ "고객님의 항공권 결제 금액은      │
│  {N}건, 총 승인금액은 {금액}원입니다"│
└─────────────────────────────────┘
                 │
                 ▼
            (이하 기존 흐름)
```

### 6.1 ARS 타입별 흐름 요약

| ARS 타입 | 흐름 |
|----------|------|
| **ARS** | 주문조회 성공 → Case 10 (삼자통화 안내) → Case 11 (3초 대기) → Case 12 (인사말) → 결제안내 |
| **SMS** | 주문조회 성공 → Case 12 (인사말) → 결제안내 |
| **TKT** | 주문조회 성공 → Case 12 (인사말) → 결제안내 |

---

## 7. 음성 파일 맵핑 업데이트

### 7.1 기존 음성 파일 맵핑에 추가

| 멘트 ID | 파일 경로 | 설명 |
|---------|----------|------|
| transfer_guide | ment/Travelport/transfer_guide | 삼자통화 안내 [신규] |
| intro | ment/Travelport/intro | 시스템 인사말 [신규] |
| wait_3sec | ment/Travelport/wait_3sec | 3초 대기음 [신규] |

---

## 8. 테스트 시나리오

### 8.1 정상 케이스 - ARS 타입, 주문조회 성공

```
테스트: [ARS 타입] 주문조회 성공 후 삼자통화 대기 흐름
조건:
  - ARS 타입 (szArsType = "ARS")
  - 유효한 휴대폰 번호 입력, 해당 번호로 주문 존재
기대결과:
  1. "휴대폰 번호를 입력 후 우물 정자를 눌러주세요" 재생
  2. 고객 전화번호 입력 (예: 01012345678#)
  3. 주문조회 API 호출 → 성공
  4. "삼초 이내에 삼자 통화 또는 호전환하시기 바랍니다" 재생 ← ARS만 적용
  5. 3초 대기 (대기음 재생) ← ARS만 적용
  6. "안녕하세요? 항공권 인증 결재 시스템입니다" 재생
  7. "고객님의 항공권 결제 금액은 N건, 총 승인금액은 XXX원입니다" 재생
```

### 8.2 정상 케이스 - SMS/TKT 타입, 주문조회 성공

```
테스트: [SMS/TKT 타입] 주문조회 성공 후 즉시 인사말 진행
조건:
  - SMS 또는 TKT 타입 (szArsType = "SMS" 또는 "TKT")
  - 유효한 휴대폰 번호 입력, 해당 번호로 주문 존재
기대결과:
  1. "휴대폰 번호를 입력 후 우물 정자를 눌러주세요" 재생
  2. 고객 전화번호 입력 (예: 01012345678#)
  3. 주문조회 API 호출 → 성공
  4. "안녕하세요? 항공권 인증 결재 시스템입니다" 재생 ← 바로 인사말 진행
  5. "고객님의 항공권 결제 금액은 N건, 총 승인금액은 XXX원입니다" 재생
  ※ 삼자통화 안내 및 3초 대기 없이 즉시 인사말로 진행
```

### 8.3 주문조회 실패 케이스 - 전화번호 재입력

```
테스트: 주문조회 실패 시 전화번호 재입력
조건: 유효한 휴대폰 번호 입력, 해당 번호로 주문 없음 (ARS 타입 무관)
기대결과:
  1. "휴대폰 번호를 입력 후 우물 정자를 눌러주세요" 재생
  2. 고객 전화번호 입력 (예: 01012345678#)
  3. 주문조회 API 호출 → 실패 (주문 없음)
  4. 즉시 "휴대폰 번호를 입력 후 우물 정자를 눌러주세요" 재생 (Case 1로 복귀)
  5. 고객이 다른 전화번호 입력 가능
```

### 8.4 음성 파일 누락 케이스

```
테스트: 음성 파일 누락 시 동작
조건: transfer_guide.wav 또는 intro.wav 파일 없음
기대결과:
  - 오류 없이 다음 단계로 진행
  - 시스템 로그에 파일 누락 경고 기록
```

### 8.5 삼자통화 전환 케이스

```
테스트: [ARS 타입] 삼자통화 전환 시 통화 유지
조건: ARS 타입, 3초 대기 중 상담원이 삼자통화 연결
기대결과:
  - 고객과 상담원 모두 다음 멘트 청취 가능
  - 시나리오 정상 진행
```

---

## 9. 구현 체크리스트

### 9.1 코드 수정
- [x] KICC_Common.h 수정
  - [x] Card_ResInfo 구조체에 REQUEST_TYPE 필드 추가
- [x] KICC_Travelport_Scenario.h 수정
  - [x] CKICC_Scenario 클래스에 m_szRequestType 필드 추가
- [x] ADODB.cpp 수정
  - [x] FetchMultiOrderResults()에서 request_type 컬럼 읽기
  - [x] m_szRequestType에 첫 주문의 request_type 저장
- [x] KICC_Travelport_Scenario.cpp 수정
  - [x] 생성자에서 m_szRequestType 초기화
  - [x] KICC_getMultiOrderInfo() 수정: 성공 시 request_type 분기 로직 추가
    - [x] SMS/TKT 타입: Case 12 (시스템 인사말)로 직접 분기
    - [x] ARS 타입 또는 빈값: Case 10 (삼자통화 안내)으로 분기
  - [x] KICC_getMultiOrderInfo() 수정: 실패 시 Case 1로 복귀
  - [x] KICC_ArsScenarioStart() 수정: Case 10, 11, 12 신규 추가

### 9.2 저장 프로시저(SP) 수정
- [x] sp_getKiccMultiOrderInfoHour.txt 수정 (request_type 컬럼 추가)
- [x] sp_getKiccMultiOrderInfo.txt 수정 (request_type 컬럼 추가)
- [ ] DB에 SP 적용 (SSMS에서 실행)

### 9.3 음성 파일 준비
- [x] transfer_guide.wav 녹음 (ARS 타입에만 사용)
- [x] intro.wav 녹음 (모든 타입 공통)
- [x] wait_3sec.wav 생성 (3초 대기음, ARS 타입에만 사용)

### 9.4 테스트
- [ ] 단위 테스트: [ARS 타입] 주문조회 성공 → 삼자통화 안내 → 3초 대기 → 인사말 흐름 확인
- [ ] 단위 테스트: [SMS 타입] 주문조회 성공 → 인사말 (삼자통화/대기 생략) 흐름 확인
- [ ] 단위 테스트: [TKT 타입] 주문조회 성공 → 인사말 (삼자통화/대기 생략) 흐름 확인
- [ ] 단위 테스트: 주문조회 실패 → 전화번호 재입력 흐름 확인
- [ ] 통합 테스트: ARS/SMS/TKT 타입별 전체 시나리오 흐름 확인
- [ ] 음성 테스트: 모든 멘트 정상 재생 확인

---

## 10. 변경 이력

| 버전 | 날짜 | 작성자 | 변경 내용 |
|------|------|--------|----------|
| 1.0 | 2025-12-20 | Claude | 최초 작성 |
| 1.1 | 2025-12-21 | Claude | 흐름 변경: 주문조회 성공 후 삼자통화 안내, 실패 시 전화번호 재입력 |
| 1.2 | 2025-12-21 | Claude | ARS 타입 분기 추가: ARS만 삼자통화 안내/3초 대기 적용, SMS/TKT는 직접 인사말로 진행 |
| 1.3 | 2025-12-21 | Claude | 분기 기준 변경: szArsType(시나리오 타입) → m_szRequestType(DB 주문의 request_type) |
| 1.4 | 2025-12-22 | Claude | 구현 완료 업데이트: 데이터 구조 변경(KICC_Common.h, KICC_Travelport_Scenario.h, ADODB.cpp), SP 수정 내용 추가, 분기 로직 수정(빈값=ARS 기본 동작), 체크리스트 업데이트 |
