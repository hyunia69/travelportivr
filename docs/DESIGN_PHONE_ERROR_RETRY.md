# DESIGN_PHONE_ERROR_RETRY.md

## 휴대폰 번호 입력 오류 시 3회 재시도 후 종료 설계

**문서 작성일**: 2025-12-15
**대상 프로젝트**: KICC_Scenario_Travelport
**목적**: 주문정보 조회 실패 시 휴대폰 번호 재입력을 3회까지만 허용하고, 3회 모두 실패 시 서비스를 종료하도록 수정

---

## 1. 현재 구현 상태 분석

### 1.1 현재 흐름 (DESIGN_RETURN_PHONE_INPUT.md 구현 후)

```
[현재 구현된 흐름 - 무한 재시도 가능]

KICC_ArsScenarioStart(1) ◀─────────────────────────────────┐
    └── state 1: 휴대폰 번호 입력 안내                     │
    └── state 2: 번호 유효성 검증                          │
                    │                                      │
                    ▼                                      │
            KICC_getMultiOrderInfo(0)                      │
                └── state 0: DB 조회 시작                  │
                └── state 1: 조회 결과 확인                │
                        ├── DB오류 → 안내 멘트 → state 2 ──┤
                        ├── 주문 없음 → 안내 멘트 → state 2 ┤
                        └── 주문 있음 → KICC_AnnounceMultiOrders(0)
                └── state 2: 전화번호 재입력 ──────────────┘
                                   (무한 반복 가능!)
```

### 1.2 문제점

| 항목 | 현재 상태 | 문제점 |
|------|-----------|--------|
| 재시도 횟수 | 무제한 | 잘못된 번호로 무한 반복 가능 |
| 시스템 리소스 | 점유 지속 | 회선 점유로 인한 서비스 품질 저하 |
| 사용자 경험 | 무한 루프 | 명확한 종료 조건 없음 |

---

## 2. 수정 목표

### 2.1 목표 흐름

```
[수정 후 흐름 - 3회 재시도 제한]

KICC_ArsScenarioStart(1) ◀────────────────────────────────────┐
    └── state 1: 휴대폰 번호 입력 안내                        │
    └── state 2: 번호 유효성 검증                             │
                    │                                         │
                    ▼                                         │
            KICC_getMultiOrderInfo(0)                         │
                └── state 0: DB 조회 시작                     │
                └── state 1: 조회 결과 확인                   │
                        ├── DB오류 또는 주문 없음:            │
                        │       m_nPhoneRetryCount++          │
                        │       3회 미만? → state 2 ──────────┤
                        │       3회 이상? → state 3 (종료) ───┼──▶ KICC_ExitSvc
                        └── 주문 있음 →                       │
                                m_nPhoneRetryCount = 0        │
                                KICC_AnnounceMultiOrders(0)   │
                └── state 2: 전화번호 재입력 (3회 미만) ──────┘
                └── state 3: [NEW] 3회 초과 종료 안내
```

### 2.2 기대 효과

| 항목 | 수정 전 | 수정 후 |
|------|---------|---------|
| 재시도 횟수 | 무제한 | 최대 3회 |
| 3회 실패 시 | 무한 반복 | 안내 멘트 후 종료 |
| 시스템 리소스 | 무한 점유 가능 | 최대 점유 시간 제한 |
| 사용자 안내 | 없음 | "3회 실패로 종료" 안내 |

---

## 3. 상세 수정 계획

### 3.1 수정 대상

| 파일 | 수정 내용 |
|------|----------|
| `KICC_Travelport_Scenario.h` | `m_nPhoneRetryCount` 멤버 변수 추가 |
| `KICC_Travelport_Scenario.cpp` | 생성자에서 초기화, 재시도 로직 구현 |

### 3.2 상수 정의

```cpp
// KICC_Travelport_Scenario.cpp 상단 또는 KICC_Common.h
#define MAX_PHONE_RETRY_COUNT  3   // 휴대폰 번호 입력 최대 재시도 횟수
```

---

## 4. 멤버 변수 추가

### 4.1 헤더 파일 수정

**파일**: `KICC_Travelport_Scenario.h`
**위치**: 다중 주문 지원 섹션 아래

#### 추가할 코드:

```cpp
// ========================================
// [2025-12-15 NEW] 휴대폰 번호 재시도 카운트
// ========================================
int  m_nPhoneRetryCount;            // 휴대폰 번호 입력 재시도 횟수 (0~3)
```

### 4.2 생성자 초기화

**파일**: `KICC_Travelport_Scenario.cpp`
**함수**: `CKICC_Scenario::CKICC_Scenario()` 또는 `ScenarioInit()`

#### 추가할 코드:

```cpp
m_nPhoneRetryCount = 0;  // [NEW] 휴대폰 번호 재시도 카운트 초기화
```

---

## 5. KICC_getMultiOrderInfo 함수 수정

### 5.1 State 1 수정 (조회 결과 확인)

**파일**: `KICC_Travelport_Scenario.cpp`
**함수**: `KICC_getMultiOrderInfo`
**위치**: state 1 (현재 라인 356-387)

#### 현재 코드 (DB 오류 처리):

```cpp
if (pScenario->m_DBAccess == -1) {
    // [MODIFIED] 데이터베이스 오류 - 안내 멘트 재생 후 전화번호 재입력으로 이동
    info_printf(localCh, "KICC_getMultiOrderInfo[%d] 다중 주문 조회 실패 - 전화번호 재입력 안내", state);
    eprintf("KICC_getMultiOrderInfo[%d] 다중 주문 조회 실패 - 전화번호 재입력 안내", state);
    new_guide();
    set_guide(VOC_WAVE_ID, "ment/Travelport/no_order_msg");
    setPostfunc(POST_PLAY, KICC_getMultiOrderInfo, 2, 0);  // state 2로 이동
    return send_guide(NODTMF);
}
```

#### 수정 후 코드 (DB 오류 처리):

```cpp
if (pScenario->m_DBAccess == -1) {
    // [MODIFIED] 데이터베이스 오류 - 재시도 횟수 확인 후 분기
    pScenario->m_nPhoneRetryCount++;  // 재시도 카운트 증가
    info_printf(localCh, "KICC_getMultiOrderInfo[%d] 다중 주문 조회 실패 (재시도: %d/%d)",
                state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);
    eprintf("KICC_getMultiOrderInfo[%d] 다중 주문 조회 실패 (재시도: %d/%d)",
            state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);

    new_guide();
    set_guide(VOC_WAVE_ID, "ment/Travelport/no_order_msg");

    if (pScenario->m_nPhoneRetryCount >= MAX_PHONE_RETRY_COUNT) {
        // 3회 실패 - 종료 안내로 이동
        setPostfunc(POST_PLAY, KICC_getMultiOrderInfo, 3, 0);  // state 3: 종료 안내
    } else {
        // 재시도 가능 - 전화번호 재입력으로 이동
        setPostfunc(POST_PLAY, KICC_getMultiOrderInfo, 2, 0);  // state 2: 재입력
    }
    return send_guide(NODTMF);
}
```

#### 현재 코드 (주문 없음 처리):

```cpp
if (pScenario->m_MultiOrders.nOrderCount == 0) {
    // [MODIFIED] 주문 없음 - 안내 멘트 재생 후 전화번호 재입력으로 이동
    info_printf(localCh, "KICC_getMultiOrderInfo[%d] 주문 조회되지 않음 - 전화번호 재입력 안내", state);
    eprintf("KICC_getMultiOrderInfo[%d] 주문 조회되지 않음 - 전화번호 재입력 안내", state);
    new_guide();
    set_guide(VOC_WAVE_ID, "ment/Travelport/no_order_msg");
    setPostfunc(POST_PLAY, KICC_getMultiOrderInfo, 2, 0);  // state 2로 이동
    return send_guide(NODTMF);
}
```

#### 수정 후 코드 (주문 없음 처리):

```cpp
if (pScenario->m_MultiOrders.nOrderCount == 0) {
    // [MODIFIED] 주문 없음 - 재시도 횟수 확인 후 분기
    pScenario->m_nPhoneRetryCount++;  // 재시도 카운트 증가
    info_printf(localCh, "KICC_getMultiOrderInfo[%d] 주문 조회되지 않음 (재시도: %d/%d)",
                state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);
    eprintf("KICC_getMultiOrderInfo[%d] 주문 조회되지 않음 (재시도: %d/%d)",
            state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);

    new_guide();
    set_guide(VOC_WAVE_ID, "ment/Travelport/no_order_msg");

    if (pScenario->m_nPhoneRetryCount >= MAX_PHONE_RETRY_COUNT) {
        // 3회 실패 - 종료 안내로 이동
        setPostfunc(POST_PLAY, KICC_getMultiOrderInfo, 3, 0);  // state 3: 종료 안내
    } else {
        // 재시도 가능 - 전화번호 재입력으로 이동
        setPostfunc(POST_PLAY, KICC_getMultiOrderInfo, 2, 0);  // state 2: 재입력
    }
    return send_guide(NODTMF);
}
```

#### 주문 조회 성공 시 카운트 초기화:

```cpp
// 조회된 주문 정보 로깅
if (pScenario->m_MultiOrders.nOrderCount > 0) {
    pScenario->m_nPhoneRetryCount = 0;  // [NEW] 성공 시 재시도 카운트 초기화
    info_printf(localCh, "조회된 주문 건수: %d, 총 금액: %d, AUTH_NO: %s",
               pScenario->m_MultiOrders.nOrderCount,
               pScenario->m_MultiOrders.nTotalAmount,
               pScenario->m_szAuthNo);
}
```

### 5.2 State 3 추가 (3회 초과 종료)

**위치**: state 2 다음에 추가

#### 추가할 코드:

```cpp
case 3:
    // [NEW] 3회 재시도 실패 - 서비스 종료
    info_printf(localCh, "KICC_getMultiOrderInfo[%d] 전화번호 입력 %d회 실패>서비스 종료",
                state, MAX_PHONE_RETRY_COUNT);
    eprintf("KICC_getMultiOrderInfo[%d] 전화번호 입력 %d회 실패>서비스 종료",
            state, MAX_PHONE_RETRY_COUNT);

    // 일반 종료 멘트 재생 후 서비스 종료
    set_guide(399);  // 일반 종료 멘트
    setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
    return send_guide(NODTMF);
```

---

## 6. 전체 수정된 KICC_getMultiOrderInfo 함수

```cpp
int KICC_getMultiOrderInfo(int state)
{
    CKICC_Scenario* pScenario = (CKICC_Scenario*)((*lpmt)->pScenario);
    int c = *((*lpmt)->dtmfs);
    int localCh = (*lpmt)->chanID;
    (*lpmt)->PrevCall = KICC_getMultiOrderInfo;
    (*lpmt)->prevState = state;

    switch(state)
    {
    case 0:
        // 데이터베이스 조회 시작
        info_printf(localCh, "다중 주문 조회: PHONE=%s (AUTH_NO는 SP에서 자동 추출)",
                    pScenario->m_szInputTel);

        setPostfunc(POST_NET, KICC_getMultiOrderInfo, 1, 0);
        return getMultiOrderInfo_host(90);

    case 1:
        // 조회 결과 확인
        if (pScenario->m_DBAccess == -1) {
            // [MODIFIED] 데이터베이스 오류 - 재시도 횟수 확인 후 분기
            pScenario->m_nPhoneRetryCount++;
            info_printf(localCh, "KICC_getMultiOrderInfo[%d] 다중 주문 조회 실패 (재시도: %d/%d)",
                        state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);
            eprintf("KICC_getMultiOrderInfo[%d] 다중 주문 조회 실패 (재시도: %d/%d)",
                    state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);

            new_guide();
            set_guide(VOC_WAVE_ID, "ment/Travelport/no_order_msg");

            if (pScenario->m_nPhoneRetryCount >= MAX_PHONE_RETRY_COUNT) {
                setPostfunc(POST_PLAY, KICC_getMultiOrderInfo, 3, 0);
            } else {
                setPostfunc(POST_PLAY, KICC_getMultiOrderInfo, 2, 0);
            }
            return send_guide(NODTMF);
        }

        if (pScenario->m_MultiOrders.nOrderCount == 0) {
            // [MODIFIED] 주문 없음 - 재시도 횟수 확인 후 분기
            pScenario->m_nPhoneRetryCount++;
            info_printf(localCh, "KICC_getMultiOrderInfo[%d] 주문 조회되지 않음 (재시도: %d/%d)",
                        state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);
            eprintf("KICC_getMultiOrderInfo[%d] 주문 조회되지 않음 (재시도: %d/%d)",
                    state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);

            new_guide();
            set_guide(VOC_WAVE_ID, "ment/Travelport/no_order_msg");

            if (pScenario->m_nPhoneRetryCount >= MAX_PHONE_RETRY_COUNT) {
                setPostfunc(POST_PLAY, KICC_getMultiOrderInfo, 3, 0);
            } else {
                setPostfunc(POST_PLAY, KICC_getMultiOrderInfo, 2, 0);
            }
            return send_guide(NODTMF);
        }

        // 조회 성공 - 재시도 카운트 초기화
        if (pScenario->m_MultiOrders.nOrderCount > 0) {
            pScenario->m_nPhoneRetryCount = 0;  // [NEW] 성공 시 초기화
            info_printf(localCh, "조회된 주문 건수: %d, 총 금액: %d, AUTH_NO: %s",
                       pScenario->m_MultiOrders.nOrderCount,
                       pScenario->m_MultiOrders.nTotalAmount,
                       pScenario->m_szAuthNo);
        }

        // 주문 개수 및 총 금액 안내
        return KICC_AnnounceMultiOrders(0);

    case 2:
        // [EXISTING] 주문 없음 안내 후 전화번호 재입력으로 복귀
        info_printf(localCh, "KICC_getMultiOrderInfo[%d] 주문 없음>전화번호 재입력 (재시도: %d/%d)",
                    state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);
        eprintf("KICC_getMultiOrderInfo[%d] 주문 없음>전화번호 재입력 (재시도: %d/%d)",
                state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);

        if (strcmp(pScenario->szArsType, "ARS") == 0) {
            return KICC_ArsScenarioStart(1);
        }
        else if (strcmp(pScenario->szArsType, "SMS") == 0) {
            return KICC_SMSScenarioStart(1);
        }
        else {
            return pScenario->jobArs(0);
        }

    case 3:
        // [NEW] 3회 재시도 실패 - 서비스 종료
        info_printf(localCh, "KICC_getMultiOrderInfo[%d] 전화번호 입력 %d회 실패>서비스 종료",
                    state, MAX_PHONE_RETRY_COUNT);
        eprintf("KICC_getMultiOrderInfo[%d] 전화번호 입력 %d회 실패>서비스 종료",
                state, MAX_PHONE_RETRY_COUNT);

        // 일반 종료 멘트 재생 후 서비스 종료
        set_guide(399);  // 일반 종료 멘트
        setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
        return send_guide(NODTMF);
    }

    return 0;
}
```

---

## 7. 테스트 시나리오

### 7.1 정상 흐름 테스트

| # | 시나리오 | 입력 | 예상 결과 |
|---|----------|------|----------|
| 1 | 첫 번째 시도 성공 | 유효한 번호 | 주문 안내 → 카드 입력 |
| 2 | 두 번째 시도 성공 | 1차 실패 → 2차 성공 | 재입력 → 주문 안내 |
| 3 | 세 번째 시도 성공 | 2차 실패 → 3차 성공 | 재입력 → 주문 안내 |

### 7.2 실패 흐름 테스트

| # | 시나리오 | 입력 | 예상 결과 |
|---|----------|------|----------|
| 4 | 3회 연속 실패 | 3회 모두 주문 없음 | 종료 안내 멘트 → 서비스 종료 |
| 5 | 3회 연속 DB 오류 | 3회 모두 DB 오류 | 종료 안내 멘트 → 서비스 종료 |
| 6 | 혼합 실패 | 주문없음→DB오류→주문없음 | 종료 안내 멘트 → 서비스 종료 |

### 7.3 카운트 초기화 테스트

| # | 시나리오 | 입력 | 예상 결과 |
|---|----------|------|----------|
| 7 | 1회 실패 후 성공 | 실패 → 성공 | 카운트 0으로 초기화 |
| 8 | 2회 실패 후 성공 | 실패 → 실패 → 성공 | 카운트 0으로 초기화 |

### 7.4 경계값 테스트

| # | 시나리오 | 예상 결과 |
|---|----------|----------|
| 9 | 정확히 3회째 입력 성공 | 주문 안내 (종료 아님) |
| 10 | 정확히 3회째 입력 실패 | 종료 안내 |

---

## 8. 구현 체크리스트

### 8.1 코드 수정

- [ ] `KICC_Travelport_Scenario.h`에 `m_nPhoneRetryCount` 멤버 변수 추가
- [ ] `KICC_Travelport_Scenario.cpp` 상단에 `MAX_PHONE_RETRY_COUNT` 상수 정의
- [ ] 생성자 또는 `ScenarioInit()`에서 `m_nPhoneRetryCount = 0` 초기화
- [ ] `KICC_getMultiOrderInfo` state 1: DB 오류 시 카운트 증가 및 분기 로직
- [ ] `KICC_getMultiOrderInfo` state 1: 주문 없음 시 카운트 증가 및 분기 로직
- [ ] `KICC_getMultiOrderInfo` state 1: 조회 성공 시 카운트 초기화
- [ ] `KICC_getMultiOrderInfo` state 2: 로그에 현재 재시도 횟수 표시
- [ ] `KICC_getMultiOrderInfo` state 3: 신규 추가 (3회 초과 종료, 일반 종료 멘트 사용)

### 8.2 테스트

- [ ] Debug 빌드로 테스트 시나리오 7.1~7.4 수행
- [ ] Release 빌드 생성
- [ ] 운영 환경 배포 전 QA 테스트

---

## 9. 영향도 분석

### 9.1 수정 범위

| 파일 | 수정 내용 | 라인 수 |
|------|----------|---------|
| `KICC_Travelport_Scenario.h` | 멤버 변수 1개 추가 | +3줄 |
| `KICC_Travelport_Scenario.cpp` | 상수 정의, 초기화, 로직 수정 | +30줄 |

### 9.2 연관 함수

| 함수 | 영향 | 설명 |
|------|------|------|
| `KICC_getMultiOrderInfo` | **수정됨** | 재시도 로직 추가, state 3 추가 |
| `ScenarioInit` | **수정됨** | 카운트 초기화 추가 |
| `KICC_ArsScenarioStart` | 호출됨 | 변경 없음 |
| `KICC_ExitSvc` | 호출됨 | 변경 없음 |
| `KICC_AnnounceMultiOrders` | 무관 | 변경 없음 |

### 9.3 위험도 평가

| 항목 | 평가 | 사유 |
|------|------|------|
| 기능 안정성 | **낮음** | 기존 흐름에 조건 분기만 추가 |
| 회귀 가능성 | **낮음** | 성공 케이스 동작 변경 없음 |
| 롤백 용이성 | **높음** | 단순 코드 복원으로 롤백 가능 |

---

## 10. 변경 이력

| 일자 | 버전 | 내용 |
|------|------|------|
| 2025-12-15 | 1.0 | 최초 작성 - 3회 재시도 제한 설계 |

---

## 11. 참고 자료

### 11.1 관련 파일

- `KICC_Travelport_Scenario.h`: 멤버 변수 추가 대상
- `KICC_Travelport_Scenario.cpp`:
  - `KICC_getMultiOrderInfo`: 라인 337-406 (수정 대상)
  - `ScenarioInit`: 초기화 추가 대상

### 11.2 관련 문서

- `docs/DESIGN_RETURN_PHONE_INPUT.md` - 휴대폰 번호 재입력 복귀 설계 (이전 구현)
- `docs/DESIGN_SKIP_PHONE_CONFIRM.md` - 전화번호 확인 단계 생략 설계
- `docs/PLAN_MULTI_KR.md` - 다중 주문 결제 처리 설계
