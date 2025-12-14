# DESIGN_RETURN_PHONE_INPUT.md

## 주문 정보 안내 후 휴대폰 번호 입력으로 복귀 설계

**문서 작성일**: 2025-12-14
**대상 프로젝트**: KICC_Scenario_Travelport
**목적**:
1. 주문 정보 안내 후 "틀리면 2번" 입력 시 휴대폰 번호 입력 단계로 돌아가도록 수정
2. 주문 조회 결과가 없을 때도 휴대폰 번호 입력 단계로 돌아가도록 수정

---

## 1. 현재 구현 상태 분석

### 1.1 전체 ARS 흐름

```
[현재 전체 흐름]

KICC_ArsScenarioStart(0)
    └── state 0: 인사말 (생략됨)
    └── state 1: 휴대폰 번호 입력 안내 ("전화번호를 입력해주세요")
    └── state 2: 번호 유효성 검증 → 바로 주문 조회로 진행 (SKIP_PHONE_CONFIRM 활성화)
                    │
                    ▼
            KICC_getMultiOrderInfo(0)
                └── state 0: DB에서 다중 주문 조회 시작
                └── state 1: 조회 결과 확인
                        ├── 주문 없음 → 안내 멘트 → KICC_ExitSvc(0) (종료) ◀ 문제점 #2!
                        └── 주문 있음 → KICC_AnnounceMultiOrders(0)
                    │
                    ▼
            KICC_AnnounceMultiOrders(0)
                └── state 0: TTS로 주문 건수/금액 안내
                └── state 1: 확인 멘트 재생 ("맞으면 1번, 틀리면 2번")
                └── state 2: 사용자 입력 처리
                        ├── 1번 → KICC_CardInput(0) (카드 입력으로 진행)
                        └── 2번 → KICC_ExitSvc(0) (종료) ◀ 문제점 #1!
```

### 1.2 문제 발생 지점

#### 문제점 #1: 주문 안내 후 2번 입력 시 종료

**위치**: `KICC_Travelport_Scenario.cpp` - `KICC_AnnounceMultiOrders` 함수 state 2

**현재 코드** (라인 459-464):
```cpp
else {
    // 취소 및 종료
    info_printf(localCh, "사용자가 다중 주문 결제 취소");
    return KICC_ExitSvc(0);
}
```

**문제점**: 사용자가 2번(아니오)을 누르면 서비스가 종료되어버림.
휴대폰 번호를 잘못 입력한 경우 재입력 기회가 없음.

#### 문제점 #2: 주문 조회 결과가 없을 때 종료

**위치**: `KICC_Travelport_Scenario.cpp` - `KICC_getMultiOrderInfo` 함수 state 1

**현재 코드** (라인 364-369):
```cpp
if (pScenario->m_MultiOrders.nOrderCount == 0) {
    // 주문 없음
    info_printf(localCh, "주문 조회되지 않음");
    set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\no_order_msg");
    setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
    return send_guide(NODTMF);
}
```

**문제점**: 주문이 조회되지 않으면 안내 멘트 재생 후 서비스 종료됨.
휴대폰 번호를 잘못 입력한 경우 재입력 기회가 없음.

#### 문제점 #3: DB 조회 실패 시 종료

**위치**: `KICC_Travelport_Scenario.cpp` - `KICC_getMultiOrderInfo` 함수 state 1

**현재 코드** (라인 358-362):
```cpp
if (pScenario->m_DBAccess == -1) {
    // 데이터베이스 오류
    info_printf(localCh, "다중 주문 조회 실패");
    return KICC_ExitSvc(0);
}
```

**문제점**: DB 조회가 실패하면 바로 서비스 종료됨.
휴대폰 번호를 잘못 입력하여 조회 실패한 경우 재입력 기회가 없음.

---

## 2. 수정 목표

### 2.1 목표 흐름

```
[수정 후 흐름]

KICC_ArsScenarioStart(1) ◀─────────────────────────────────┐
    └── state 1: 휴대폰 번호 입력 안내                     │
    └── state 2: 번호 유효성 검증                          │
                    │                                      │
                    ▼                                      │
            KICC_getMultiOrderInfo(0)                      │
                └── state 0: DB 조회 시작                  │
                └── state 1: 조회 결과 확인                │
                        ├── 주문 없음 → 안내 멘트 → state 2 │
                        └── 주문 있음 → KICC_AnnounceMultiOrders(0)
                └── state 2: [NEW] 전화번호 재입력 ────────┤
                    │                                      │
                    ▼                                      │
            KICC_AnnounceMultiOrders(0)                    │
                └── state 0~1: 주문 안내                   │
                └── state 2: 사용자 입력                   │
                        ├── 1번 → KICC_CardInput(0)        │
                        └── 2번 → KICC_ArsScenarioStart(1) ┘
                                    (휴대폰 번호 재입력)
```

### 2.2 기대 효과

| 항목 | 수정 전 | 수정 후 |
|------|---------|---------|
| 주문 안내 후 2번 입력 | 서비스 종료 | 휴대폰 번호 재입력 |
| **주문 조회 결과 없음** | **서비스 종료** | **휴대폰 번호 재입력** |
| 사용자 경험 | 잘못된 번호 입력 시 재전화 필요 | 재입력 가능 |
| 안내 멘트 의미 | "틀리면 2번" → 종료 (혼란) | "틀리면 2번" → 재입력 (명확) |

---

## 3. 상세 수정 계획

### 3.1 수정 대상 파일

| 파일 | 함수 | 수정 내용 |
|------|------|----------|
| `KICC_Travelport_Scenario.cpp` | `KICC_AnnounceMultiOrders` | state 2에서 2번 입력 시 동작 변경 |
| `KICC_Travelport_Scenario.cpp` | `KICC_getMultiOrderInfo` | 주문 없음 시 state 2 추가하여 전화번호 재입력으로 복귀 |

---

### 3.2 수정 #1: 주문 없음 시 전화번호 재입력 (`KICC_getMultiOrderInfo`)

**파일**: `KICC_Travelport_Scenario.cpp`
**함수**: `KICC_getMultiOrderInfo`
**State**: 1 수정 + state 2 신규 추가 (라인 364-385)

#### 현재 코드 (state 1):

```cpp
if (pScenario->m_MultiOrders.nOrderCount == 0) {
    // 주문 없음
    info_printf(localCh, "주문 조회되지 않음");
    set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\no_order_msg");
    setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
    return send_guide(NODTMF);
}
```

#### 수정 후 코드 (state 1):

```cpp
if (pScenario->m_MultiOrders.nOrderCount == 0) {
    // [MODIFIED] 주문 없음 - 안내 멘트 재생 후 전화번호 재입력으로 이동
    info_printf(localCh, "KICC_getMultiOrderInfo[%d] 주문 조회되지 않음 - 전화번호 재입력 안내", state);
    eprintf("KICC_getMultiOrderInfo[%d] 주문 조회되지 않음 - 전화번호 재입력 안내", state);
    new_guide();
    set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\no_order_msg");
    setPostfunc(POST_PLAY, KICC_getMultiOrderInfo, 2, 0);  // state 2로 이동
    return send_guide(NODTMF);
}
```

#### 신규 추가 코드 (state 2):

```cpp
case 2:
    // [NEW] 주문 없음 안내 후 전화번호 재입력으로 복귀
    info_printf(localCh, "KICC_getMultiOrderInfo[%d] 주문 없음>전화번호 재입력", state);
    eprintf("KICC_getMultiOrderInfo[%d] 주문 없음>전화번호 재입력", state);

    if (strcmp(pScenario->szArsType, "ARS") == 0) {
        return KICC_ArsScenarioStart(1);
    }
    else if (strcmp(pScenario->szArsType, "SMS") == 0) {
        return KICC_SMSScenarioStart(1);
    }
    else {
        return pScenario->jobArs(0);
    }
```

---

### 3.3 수정 #2: 2번 입력 시 전화번호 재입력 (`KICC_AnnounceMultiOrders`)

**파일**: `KICC_Travelport_Scenario.cpp`
**함수**: `KICC_AnnounceMultiOrders`
**State**: 2 (라인 449-465)

#### 현재 코드:

```cpp
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
```

#### 수정 후 코드:

```cpp
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
        // [MODIFIED] 2번 입력 시 휴대폰 번호 입력 단계로 복귀
        info_printf(localCh, "KICC_AnnounceMultiOrders[%d] 다중 주문 안내>확인 부> 아니오 - 전화번호 재입력", state);
        eprintf("KICC_AnnounceMultiOrders[%d] 다중 주문 안내>확인 부> 아니오 - 전화번호 재입력", state);

        // ARS 타입에 따라 분기 (KICC_getOrderInfo 함수의 패턴 참조)
        if (strcmp(pScenario->szArsType, "ARS") == 0) {
            return KICC_ArsScenarioStart(1);  // 휴대폰 번호 입력 state
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

## 4. 참조 코드 분석

### 4.1 기존 복귀 패턴 (`KICC_getOrderInfo` state 2)

이미 비슷한 패턴이 `KICC_getOrderInfo` 함수에 구현되어 있습니다:

**위치**: 라인 2090-2097

```cpp
else if (c == '2')//아니오
{
    info_printf(localCh, "KICC_getOrderInfo[%d] 고객 주문 정보 안내 부>확인 부> 아니오", state);
    eprintf("KICC_getOrderInfo[%d] 고객 주문 정보 안내 부>확인 부> 아니오", state);

    if (strcmp(pScenario->szArsType, "ARS") == 0) return KICC_ArsScenarioStart(1);
    else if (strcmp(pScenario->szArsType, "SMS") == 0) return KICC_SMSScenarioStart(1);
    else return pScenario->jobArs(0);
}
```

**특징**:
- `szArsType`에 따라 `ARS`, `SMS`, 기타 타입 분기
- `KICC_ArsScenarioStart(1)`: state 1은 휴대폰 번호 입력 안내 단계

### 4.2 `KICC_ArsScenarioStart` State 설명

| State | 기능 |
|-------|------|
| 0 | 인사말 (현재 생략됨) |
| **1** | **휴대폰 번호 입력 안내** ← 복귀 대상 |
| 2 | 번호 유효성 검증 및 다음 단계 진행 |
| 3~4 | (SKIP_PHONE_CONFIRM=0 시) TTS 확인 및 1/2번 선택 |

---

## 5. 상태 초기화 필요성 검토

### 5.1 초기화 대상 변수

휴대폰 번호 재입력 시 일부 변수 초기화가 필요할 수 있습니다:

| 변수 | 현재 값 | 초기화 필요 | 사유 |
|------|---------|-------------|------|
| `m_szInputTel` | 이전 입력값 | △ (선택적) | state 2에서 덮어쓰기됨 |
| `m_bMultiOrderMode` | TRUE | ✕ | 재조회 시 동일하게 사용 |
| `m_MultiOrders` | 조회 결과 | ✕ | 재조회 시 덮어쓰기됨 |

**결론**: 명시적 초기화 없이도 정상 동작 예상 (기존 `KICC_getOrderInfo` 패턴과 동일)

### 5.2 메모리 안전성

`KICC_ArsScenarioStart(1)`로 복귀 시:
- `lpmt->refinfo`: 새로운 DTMF 입력으로 덮어쓰기됨
- `lpmt->dtmfs`: 새로운 입력으로 덮어쓰기됨
- 이전 주문 조회 데이터: `KICC_getMultiOrderInfo` state 0에서 초기화됨

---

## 6. 구현 체크리스트

### 6.1 코드 수정

**수정 #1: `KICC_getMultiOrderInfo` (주문 없음 처리)**
- [ ] state 1에서 주문 없음 시 `setPostfunc` 대상을 state 2로 변경
- [ ] state 2 신규 추가 (전화번호 재입력 복귀)
- [ ] 로그 메시지 추가

**수정 #2: `KICC_AnnounceMultiOrders` (2번 입력 처리)**
- [ ] state 2에서 2번 입력 처리 수정
- [ ] 로그 메시지 추가 (전화번호 재입력 표시)
- [ ] `szArsType` 분기 처리 (ARS/SMS/기타)

### 6.2 테스트 시나리오

| # | 테스트 항목 | 예상 결과 |
|---|------------|----------|
| 1 | 주문 안내 후 1번 입력 | 카드 입력 진행 (기존 동작 유지) |
| 2 | 주문 안내 후 2번 입력 | 휴대폰 번호 입력 멘트 재생 |
| 3 | 2번 후 다른 번호 입력 | 새 번호로 주문 재조회 |
| 4 | 2번 후 동일 번호 입력 | 동일 주문 정보 재안내 |
| 5 | 잘못된 DTMF (3~9) | 오류 멘트 재생 |
| **6** | **존재하지 않는 번호 입력** | **"주문 없음" 안내 후 휴대폰 번호 입력 멘트 재생** |
| **7** | **주문 없음 후 다른 번호 입력** | **새 번호로 주문 재조회** |

### 6.3 빌드 및 배포

- [ ] Debug 구성으로 빌드 및 테스트
- [ ] Release 구성으로 빌드
- [ ] 운영 환경 배포 전 QA 테스트

---

## 7. 영향도 분석

### 7.1 수정 범위

- **수정 파일**: 1개 (`KICC_Travelport_Scenario.cpp`)
- **수정 함수**: 2개 (`KICC_getMultiOrderInfo`, `KICC_AnnounceMultiOrders`)
- **수정 라인**: 약 25줄 (state 2 신규 추가 포함)

### 7.2 연관 함수

| 함수 | 영향 | 설명 |
|------|------|------|
| `KICC_ArsScenarioStart` | 호출됨 | 복귀 대상 (수정 불필요) |
| `KICC_SMSScenarioStart` | 호출됨 | SMS 타입 시 복귀 대상 |
| `KICC_getMultiOrderInfo` | **수정됨** | state 1 수정 + state 2 신규 추가 |
| `KICC_AnnounceMultiOrders` | **수정됨** | state 2에서 2번 입력 처리 변경 |
| `KICC_CardInput` | 무관 | 1번 입력 시 동작 (변경 없음) |

### 7.3 위험도 평가

| 항목 | 평가 | 사유 |
|------|------|------|
| 기능 안정성 | **낮음** | 기존 패턴(`KICC_getOrderInfo`)과 동일 |
| 회귀 가능성 | **낮음** | 1번 입력 동작 변경 없음, 정상 흐름 유지 |
| 롤백 용이성 | **높음** | 단순 코드 복원으로 롤백 가능 |

---

## 8. 변경 이력

| 일자 | 버전 | 내용 |
|------|------|------|
| 2025-12-14 | 1.0 | 최초 작성 - 2번 입력 시 전화번호 재입력 복귀 |
| 2025-12-14 | 1.1 | 주문 없음 시 전화번호 재입력 복귀 추가 |

---

## 9. 참고 자료

### 9.1 관련 파일

- `KICC_Travelport_Scenario.cpp`
  - `KICC_getMultiOrderInfo`: 라인 337-385 (수정 대상 #1)
  - `KICC_AnnounceMultiOrders`: 라인 388-468 (수정 대상 #2)
  - `KICC_getOrderInfo`: 라인 1987-2108 (참조 패턴)
  - `KICC_ArsScenarioStart`: 라인 2110-2279 (복귀 대상)

### 9.2 관련 문서

- `docs/DESIGN_SKIP_PHONE_CONFIRM.md` - 전화번호 확인 단계 생략 설계
- `docs/PLAN_MULTI_KR.md` - 다중 주문 결제 처리 설계
