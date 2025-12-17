# DESIGN_FAIL_CASE.md

## 다중 주문 결제 전부 실패 시 카드번호 재입력 설계

**문서 작성일**: 2025-12-17
**대상 프로젝트**: KICC_Scenario_Travelport
**목적**: 복수건 주문 결제가 **전부 실패**한 경우 사용자에게 선택권 제공 (1번: 재결제, 2번: 담당직원 연결)

---

## 1. 현재 구현 상태 분석

### 1.1 결제 결과 처리 흐름

```
[현재 전체 흐름]

KICC_ProcessMultiPayments(0)
    └── case 0: 초기화 (nProcessedCount=0, nFailedCount=0)
    └── case 1: 주문 처리 루프 (주문별 결제 요청)
    └── case 2: 결제 결과 확인 (성공/실패 카운터 업데이트)
    └── case 3~5: DB 저장 및 다음 주문 이동
            │
            ▼ (모든 주문 처리 완료 시)
    KICC_MultiPaymentSummary(0)
        └── case 0: 결과 요약 TTS 재생
                ├── 모두 성공 → "N건 결제 완료" 안내 → 종료
                └── 일부 실패 → "N건 완료, M건 실패" 안내 → 종료 ◀ 전부 실패도 여기!
        └── case 1: TTS 파일 재생 후 종료
```

### 1.2 현재 구현의 문제점 (2025-12-17 발견)

**위치**: `KICC_Travelport_Scenario.cpp` - `KICC_MultiPaymentSummary` 함수

**현재 구현된 흐름**:
```
case 0: TTS_Play("N건 실패... 다시 결제를 진행하시려면 1번, 담당직원을 연결하시려면 2번...") → state 2
case 2: szTTSFile 재생 (실제 TTS 음성) → state 3
case 3: "맞으면 1번, 틀리면 2번" (input_confirm) 멘트 재생 + DTMF 대기 → state 4  ◀ 문제!
case 4: DTMF 입력 처리
```

**문제점**:
1. **불필요한 확인 멘트**: state 3에서 `input_confirm` 멘트("맞으면 1번, 틀리면 2번")가 재생됨
2. **사용자 혼란**: 이미 case 0에서 "1번: 재결제, 2번: 담당직원" 안내를 들었는데, 다시 "맞으면 1번..."이 나감
3. **의미 불일치**: "맞으면/틀리면"은 확인용이고, "재결제/담당직원"은 선택용으로 맥락이 다름

### 1.3 결제 실패 원인 분석

일반적인 결제 실패 원인:

| 분류 | 실패 원인 | 재시도 가치 |
|------|----------|--------------|
| **카드 정보 오류** | 카드번호 오류, 유효기간 오류, 비밀번호 오류 | ✅ 높음 |
| **한도 초과** | 일일 한도, 월 한도 초과 | ✕ 낮음 |
| **시스템 오류** | 가맹점 ID 오류(E106), 통신 장애 | ✕ 낮음 |
| **카드 상태** | 분실/도난 카드, 유효기간 만료 | ✕ 낮음 |

**핵심 인사이트**:
- 카드 정보 입력 오류는 재입력으로 해결 가능
- 복수건 **전부 실패**는 대부분 카드 정보 오류 가능성이 높음
- 부분 실패 (일부 성공, 일부 실패)는 개별 주문 문제일 가능성이 높음

---

## 2. 수정 목표

### 2.1 목표 흐름

```
[수정 후 흐름]

KICC_ProcessMultiPayments
    │
    ▼ (모든 주문 처리 완료 시)
KICC_MultiPaymentSummary(0)
    └── case 0: 결과 요약
            ├── 모두 성공 (nProcessedCount > 0, nFailedCount == 0)
            │       → "N건의 결제가 완료되었습니다. 담당직원을 연결해 드리겠습니다."
            │       → 연결 끊기 (종료)
            │
            ├── [전부 실패] (nProcessedCount == 0, nFailedCount > 0)
            │       → "N건 결제가 모두 실패하였습니다. 다시 결제를 진행하시려면 1번,
            │          담당 직원을 연결하시려면 2번을 눌러 주시기 바랍니다."
            │       → TTS 재생 완료 후 **바로** DTMF 입력 대기 (추가 멘트 없음)
            │           ├── 1번 입력 → 카드번호 재입력으로 복귀
            │           └── 2번 입력 → 연결 끊기 (종료)
            │
            └── 부분 실패 (nProcessedCount > 0, nFailedCount > 0)
                    → "N건은 결제가 완료되었으며 M건은 결제 실패하였습니다.
                       담당직원을 연결해 드리겠습니다."
                    → 연결 끊기 (종료)
```

### 2.2 기대 효과

| 항목 | 수정 전 | 수정 후 |
|------|---------|---------|
| 전부 실패 처리 | 종료 | **사용자 선택 (1번: 재결제, 2번: 종료)** |
| 전부 실패 멘트 | - | "N건 실패... 1번: 재결제, 2번: 담당직원" → **바로 DTMF 대기** |
| 추가 확인 멘트 | "맞으면 1번..." 재생됨 | **재생 안됨** (불필요한 멘트 제거) |
| 사용자 경험 | 카드 정보 오류 시 재전화 필요 | **선택권 제공** (재결제 or 상담원 연결) |

### 2.3 재입력 제한

**무한 루프 방지**: 재입력 시도 횟수를 제한하여 무한 루프 방지

| 항목 | 값 | 설명 |
|------|-----|------|
| `MAX_CARD_RETRY` | 3 | 최대 카드번호 재입력 횟수 |
| `m_nCardRetryCount` | 0~3 | 현재 재입력 횟수 |

```
[재입력 제한 흐름 - 사용자 선택 기반]

전부 실패 발생
    │
    ├── m_nCardRetryCount < MAX_CARD_RETRY (3)
    │       → 재입력 횟수 증가
    │       → "N건 결제가 모두 실패하였습니다. 다시 결제를 진행하시려면 1번,
    │          담당 직원을 연결하시려면 2번을 눌러 주시기 바랍니다." 안내
    │       → **바로** DTMF 입력 대기 (추가 멘트 없음)
    │           ├── 1번 → KICC_CardInput(0)으로 복귀
    │           ├── 2번 → KICC_ExitSvc(0) (종료)
    │           └── 타임아웃/잘못된 입력 → 안내 멘트만 재생 후 DTMF 대기 (최대 3회)
    │
    └── m_nCardRetryCount >= MAX_CARD_RETRY (3)
            → "결제에 실패하였습니다. 다시 전화해 주시기 바랍니다." 안내
            → KICC_ExitSvc(0) (종료)
```

---

## 3. 상세 수정 계획

### 3.1 수정 대상 파일

| 파일 | 함수/구조체 | 수정 내용 |
|------|------------|----------|
| `KICC_Travelport_Scenario.h` | `CKICC_Scenario` 클래스 | 재입력 횟수 변수 추가 (이미 완료) |
| `KICC_Travelport_Scenario.cpp` | `KICC_MultiPaymentSummary` | state 2, 3 수정 - **불필요한 확인 멘트 제거** |

---

### 3.2 핵심 수정: state 2, 3 흐름 변경

**현재 (문제있는) 코드**:

```cpp
// state 2: TTS 파일 재생 후 state 3으로 이동
case 2:
    if (strlen(pScenario->szTTSFile) > 0) {
        // TTS 파일 재생
        setPostfunc(POST_PLAY, KICC_MultiPaymentSummary, 3, 0);  // state 3으로
        return send_guide(NODTMF);
    }

// state 3: input_confirm 멘트 재생 + DTMF 대기 ← 문제!
case 3:
    set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\input_confirm");  // "맞으면 1번, 틀리면 2번"
    setPostfunc(POST_DTMF, KICC_MultiPaymentSummary, 4, 10);
    return send_guide(1);
```

**수정 후 코드**:

```cpp
// state 2: TTS 파일 재생 후 **바로** DTMF 대기 (state 3으로)
case 2:
    if (strlen(pScenario->szTTSFile) > 0) {
        // TTS 파일 재생 + DTMF 1자리 입력 대기
        set_guide(VOC_TTS_ID, TTSFile);
        setPostfunc(POST_DTMF, KICC_MultiPaymentSummary, 4, 10);  // 바로 state 4로 (DTMF 처리)
        return send_guide(1);  // 1자리 DTMF 입력 대기
    }
    // fall through to state 3

// state 3: TTS 파일 없는 경우 대기음만 재생 + DTMF 대기 (확인 멘트 없음)
case 3:
    // [MODIFIED] 불필요한 input_confirm 멘트 제거
    // 대기음만 재생하고 DTMF 입력 대기
    set_guide(VOC_WAVE_ID, "ment\\wait_sound");  // 대기음만
    setPostfunc(POST_DTMF, KICC_MultiPaymentSummary, 4, 10);
    return send_guide(1);
```

### 3.3 수정 방안 비교

| 방안 | 설명 | 장점 | 단점 |
|------|------|------|------|
| **방안 A** | state 2에서 TTS+DTMF 동시 처리, state 3 단순화 | 간단, 불필요한 멘트 제거 | state 3 역할 변경 |
| **방안 B** | state 3에서 멘트만 변경 (대기음으로) | 구조 유지 | 여전히 불필요한 state |
| **방안 C** | state 2에서 바로 state 4로 이동 | 최소 수정 | state 3 스킵 |

**선택: 방안 A** - state 2에서 TTS 재생과 동시에 DTMF 대기, state 3은 fallback용으로 단순화

---

### 3.4 전체 수정 코드

**파일**: `KICC_Travelport_Scenario.cpp`
**함수**: `KICC_MultiPaymentSummary`
**위치**: case 2, case 3

```cpp
// [NEW] 전부 실패 TTS 재생 완료 후 DTMF 입력 대기
case 2:
    eprintf("[전부실패] TTS 재생 완료 - DTMF 입력 대기");

    // TTS 서버 오류 체크
    if (pScenario->m_TTSAccess == -1) {
        new_guide();
        eprintf("[전부실패] TTS 서버 오류, 타임아웃 멘트 재생 후 DTMF 대기");
        set_guide(VOC_WAVE_ID, "ment\\TTS_TimeOut");
        setPostfunc(POST_DTMF, KICC_MultiPaymentSummary, 4, 10);  // 바로 state 4로
        return send_guide(1);  // 1자리 DTMF 입력 대기
    }

    // [MODIFIED] szTTSFile을 사용하여 실제 TTS 파일 재생 + 바로 DTMF 입력 대기
    // 추가 확인 멘트("맞으면 1번...") 없이 바로 사용자 입력 대기
    if (strlen(pScenario->szTTSFile) > 0) {
        new_guide();
        char TTSFile[2048 + 1] = { 0x00, };
        sprintf(TTSFile, "%s", pScenario->szTTSFile);
        eprintf("[전부실패] TTS 파일 재생 후 바로 DTMF 대기: %s", TTSFile);
        set_guide(VOC_TTS_ID, TTSFile);
        memset(pScenario->szTTSFile, 0x00, sizeof(pScenario->szTTSFile));

        // [MODIFIED] 바로 state 4로 이동하여 DTMF 처리
        // 기존: POST_PLAY → state 3 (input_confirm 재생) → state 4
        // 수정: POST_DTMF → state 4 (바로 DTMF 대기)
        setPostfunc(POST_DTMF, KICC_MultiPaymentSummary, 4, 10);  // 10초 타임아웃
        return send_guide(1);  // 1자리 DTMF 입력 대기
    }

    // szTTSFile이 비어있으면 state 3으로 (대기음 재생)
    eprintf("[전부실패] szTTSFile 비어있음 - state 3으로 이동");
    // fall through to state 3

// [MODIFIED] DTMF 입력 대기 (확인 멘트 없이 대기음만)
case 3:
    eprintf("[전부실패] DTMF 입력 대기 - 1번: 재결제, 2번: 종료");

    new_guide();
    // [MODIFIED] 불필요한 input_confirm 멘트 제거
    // 기존: set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\input_confirm");
    // 수정: 대기음만 재생 (또는 아예 멘트 없이 DTMF 대기)
    set_guide(VOC_WAVE_ID, "ment\\wait_sound");  // 대기음만 재생
    setPostfunc(POST_DTMF, KICC_MultiPaymentSummary, 4, 10);  // 10초 타임아웃
    return send_guide(1);  // 1자리 DTMF 입력 대기

// [EXISTING] DTMF 입력 결과 처리 (수정 없음)
case 4:
    // ... (기존 코드 유지)
```

---

## 4. 상태 전이 다이어그램

```
                    ┌─────────────────────────────────────────────────────────────────────┐
                    │                                                                     │
                    ▼                                                                     │
    KICC_CardInput(0)                                                                    │
        │                                                                                 │
        ▼                                                                                 │
    KICC_ProcessMultiPayments(0)                                                         │
        │                                                                                 │
        ▼                                                                                 │
    KICC_MultiPaymentSummary(0)                                                          │
        │                                                                                 │
        ├─── 모두 성공 ───────────────────> state 1 → KICC_ExitSvc(0) [연결 끊기]       │
        │   "N건 결제 완료. 담당직원                                                     │
        │    연결해 드리겠습니다"                                                        │
        │                                                                                 │
        ├─── 부분 실패 ───────────────────> state 1 → KICC_ExitSvc(0) [연결 끊기]       │
        │   "N건 완료, M건 실패.                                                         │
        │    담당직원 연결해 드리겠습니다"                                               │
        │                                                                                 │
        └─── 전부 실패 ───────────────────────────────────────────────────────┐         │
                                                                               │          │
                    ┌──────────────────────────────────────────────────────────┴─┐       │
                    │                                                             │       │
                    ▼                                                             ▼       │
           재입력 가능                                                      재입력 불가  │
        (retry < MAX)                                                    (retry >= MAX)  │
                    │                                                             │       │
                    ▼                                                             ▼       │
              state 2: TTS 재생                                    KICC_ExitSvc(0)       │
        "N건 실패. 1번: 재결제,                                  "다시 전화해주세요"      │
         2번: 담당직원 연결"                                       [연결 끊기]            │
                    │                                                                     │
                    ▼                                                                     │
           **바로 DTMF 대기** (10초)  ← 수정됨! (확인 멘트 없음)                         │
                    │                                                                     │
        ┌───────────┼───────────────────────────────────┐                               │
        │           │                                   │                                 │
        ▼           ▼                                   ▼                                 │
    1번 입력    2번 입력                        타임아웃/잘못된 입력                      │
        │           │                                   │                                 │
        │           ▼                                   ▼                                 │
        │   KICC_ExitSvc(0)                    TTS 재안내 (최대 3회)                      │
        │   [연결 끊기]                        "다시 결제를 진행하시려면 1번..."           │
        │                                              │                                  │
        │                                   ┌──────────┴────────┐                        │
        │                                   │                   │                         │
        │                               3회 미만            3회 이상                      │
        │                                   │                   │                         │
        │                                   ▼                   ▼                         │
        │                              state 2로         KICC_ExitSvc(0)                  │
        │                              (TTS+DTMF)          [자동 종료]                    │
        │                                                                                 │
        └─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 5. 구현 체크리스트

### 5.1 헤더 파일 수정 (`KICC_Travelport_Scenario.h`) - ✅ 완료

- [x] `#define MAX_CARD_RETRY 3` 추가
- [x] `int m_nCardRetryCount` 멤버 변수 추가

### 5.2 초기화 코드 추가 - ✅ 완료

- [x] `ScenarioInit` 또는 생성자에서 `m_nCardRetryCount = 0` 초기화

### 5.3 `KICC_MultiPaymentSummary` 함수 수정

- [x] case 0: 전부 실패 분기 추가
- [x] case 0: 재입력 가능 시 TTS 안내 후 state 2로 이동
- [x] **case 2: TTS 재생 후 바로 DTMF 대기** (POST_DTMF → state 4) ✅ 완료
- [x] **case 3: 불필요한 input_confirm 멘트 제거** (대기음만) ✅ 완료
- [x] case 4: DTMF 입력 결과 처리

### 5.4 테스트 시나리오

| # | 테스트 항목 | 입력 | 예상 결과 |
|---|------------|------|----------|
| 1 | 모두 성공 | 정상 카드 | "N건의 결제가 완료되었습니다..." → 연결 끊기 |
| 2 | 부분 실패 | 일부 주문 문제 | "N건 완료, M건 실패..." → 연결 끊기 |
| 3 | **전부 실패 + 1번 선택** | 잘못된 카드 → 1번 | "N건 실패. 1번: 재결제, 2번: 담당직원" → **바로 DTMF 대기** → 1번 → 카드번호 입력 |
| 4 | **전부 실패 + 2번 선택** | 잘못된 카드 → 2번 | "N건 실패..." → **바로 DTMF 대기** → 2번 → 연결 끊기 |
| 5 | **전부 실패 + 타임아웃** | 잘못된 카드 → 무입력 | "N건 실패..." → 10초 대기 → "다시 결제를 진행하시려면 1번..." → DTMF 대기 |
| 6 | 전부 실패 (4회차) | 잘못된 카드 (4번째) | "결제에 실패하였습니다..." → 연결 끊기 |

---

## 6. 영향도 분석

### 6.1 수정 범위

- **수정 파일**: 1개 (`KICC_Travelport_Scenario.cpp`)
- **수정 함수**: 1개 (`KICC_MultiPaymentSummary`)
- **수정 state**: 2개 (case 2, case 3)
- **수정 라인**: 약 15줄

### 6.2 기존 동작 영향

| 시나리오 | 기존 동작 | 수정 후 동작 | 변경 여부 |
|----------|----------|-------------|----------|
| 모두 성공 | 종료 | 종료 | ✕ 변경 없음 |
| 부분 실패 | 종료 | 종료 | ✕ 변경 없음 |
| 전부 실패 (TTS 후) | input_confirm 재생 → DTMF | **바로 DTMF** | ✅ 변경됨 |

### 6.3 위험도 평가

| 항목 | 평가 | 사유 |
|------|------|------|
| 기능 안정성 | **낮음** | 불필요한 멘트만 제거, 핵심 로직 유지 |
| 회귀 가능성 | **낮음** | 모두 성공, 부분 실패 동작 변경 없음 |
| 사용자 경험 | **개선** | 불필요한 확인 멘트 제거로 흐름 간소화 |

---

## 7. TTS 멘트 정의

### 7.1 전부 실패 시 멘트 (변경 없음)

| 상황 | 멘트 내용 |
|------|----------|
| 전부 실패 + 재입력 가능 | "%d건 결제가 모두 실패하였습니다. 다시 결제를 진행하시려면 1번, 담당 직원을 연결하시려면 2번을 눌러 주시기 바랍니다." |
| 전부 실패 + 재입력 불가 | "결제에 실패하였습니다. 다시 전화해 주시기 바랍니다." |
| 잘못된 입력/타임아웃 재안내 | "다시 결제를 진행하시려면 1번, 담당 직원을 연결하시려면 2번을 눌러 주시기 바랍니다." |

### 7.2 제거된 멘트

| 상황 | 기존 멘트 | 수정 후 |
|------|----------|---------|
| TTS 재생 후 DTMF 대기 | "맞으면 1번, 틀리면 2번을 눌러주세요" (input_confirm) | **제거됨** (바로 DTMF 대기) |

---

## 8. 변경 이력

| 일자 | 버전 | 내용 |
|------|------|------|
| 2025-12-17 | 1.0 | 최초 작성 - 전부 실패 시 카드번호 재입력 설계 |
| 2025-12-17 | 1.1 | TTS 멘트 변경 - "담당직원 연결" 안내 추가 |
| 2025-12-17 | 1.2 | 사용자 선택 기능 추가 - DTMF 입력 대기 추가 |
| 2025-12-17 | **1.3** | **불필요한 확인 멘트 제거** - TTS 재생 후 바로 DTMF 대기, input_confirm 멘트 제거 |

---

## 9. 참고 자료

### 9.1 관련 파일

- `KICC_Travelport_Scenario.cpp`
  - `KICC_MultiPaymentSummary`: 수정 대상 (case 2, 3)
- `KICC_Travelport_Scenario.h`
  - 클래스 멤버 변수 (이미 완료)

### 9.2 관련 문서

- `docs/PLAN_MULTI_KR.md` - 다중 주문 결제 처리 설계
- `docs/SUMMARY.md` - 다중 주문 기능 구현 요약
- `docs/DESIGN_RETURN_PHONE_INPUT.md` - 전화번호 재입력 복귀 설계 (유사 패턴 참조)
