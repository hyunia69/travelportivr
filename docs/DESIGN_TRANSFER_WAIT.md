# 삼자통화/호전환 대기 안내 멘트 추가 설계

## 1. 개요

### 1.1 목적
주문조회 성공 후, 삼자통화/호전환을 위한 안내 멘트와 대기 시간을 추가하여 상담원이 통화에 참여할 수 있는 시간을 확보합니다.

### 1.2 변경 요구사항
| 항목 | 현재 | 변경 후 |
|------|------|---------|
| 휴대폰 번호 입력 후 | 즉시 주문조회 → 결제안내 진행 | 주문조회 → (성공 시) 삼자통화 안내 → 3초 대기 → 인사말 → 결제안내 |
| 주문조회 실패 시 | 오류 안내 후 종료 | 휴대폰 번호 입력 화면으로 즉시 복귀 |

### 1.3 신규 음성 파일
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

### 3.1 변경된 흐름도

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
    │   [Case 10: 삼자통화 안내] [NEW]
    │       → new_guide()
    │       → set_guide(VOC_WAVE_ID, "ment/Travelport/transfer_guide")
    │         "삼초 이내에 삼자 통화 또는 호전환하시기 바랍니다"
    │       → setPostfunc(POST_PLAY, KICC_ArsScenarioStart, 11, 0)
    │       → send_guide(NODTMF)
    │       │
    │       ▼
    │   [Case 11: 3초 대기] [NEW]
    │       → new_guide()
    │       → set_guide(VOC_WAVE_ID, "ment/Travelport/wait_3sec")
    │       → setPostfunc(POST_PLAY, KICC_ArsScenarioStart, 12, 0)
    │       → send_guide(NODTMF)
    │       │
    │       ▼
    │   [Case 12: 시스템 인사말] [NEW]
    │       → new_guide()
    │       → set_guide(VOC_WAVE_ID, "ment/Travelport/intro")
    │         "안녕하세요? 항공권 인증 결재 시스템입니다"
    │       → setPostfunc(POST_PLAY, 결제안내함수, 0, 0)
    │       → send_guide(NODTMF)
    │       │
    │       ▼
    │   [결제 안내 진행] (기존 흐름)
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

| State | 역할 | 비고 |
|-------|------|------|
| 0 | 초기 인사말 | 기존 유지 |
| 1 | 전화번호 입력 안내 | 기존 유지 |
| 2 | 전화번호 검증 및 주문조회 | 기존 유지 (주문조회 API 호출) |
| 3 | (기존 TTS 확인) | SKIP_PHONE_CONFIRM=FALSE 시에만 사용 |
| 4 | (기존 확인 처리) | SKIP_PHONE_CONFIRM=FALSE 시에만 사용 |
| **10** | 삼자통화 안내 | **신규** - 주문조회 성공 후 진입 |
| **11** | 3초 대기 | **신규** |
| **12** | 시스템 인사말 | **신규** |
| 0xffff | 종료 | 기존 유지 |

---

## 4. 코드 변경 상세

### 4.1 KICC_getMultiOrderInfo() 함수 수정

주문조회 결과 처리 부분에서 성공/실패에 따른 분기 추가

#### 4.1.1 주문조회 성공 시 - 삼자통화 안내로 이동

**현재 코드 (주문조회 성공 시):**
```cpp
// 주문조회 성공 - 바로 결제 안내 멘트로 진행
info_printf(localCh, "KICC_getMultiOrderInfo 주문조회 성공, 결제 안내 진행");
// ... 결제 안내 로직 ...
```

**변경 코드:**
```cpp
// [MODIFIED] 주문조회 성공 - 삼자통화 안내로 이동
info_printf(localCh, "KICC_getMultiOrderInfo 주문조회 성공, 삼자통화 안내로 이동");
eprintf("KICC_getMultiOrderInfo 주문조회 성공 → 삼자통화 안내 (Case 10)");

// [NEW] 삼자통화 안내 멘트로 이동
return KICC_ArsScenarioStart(10);
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

## 6. 변경된 전체 시나리오 흐름도

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
│ Case 10: 삼자통화 안내 [NEW]     │
│ "삼초 이내에 삼자 통화 또는       │
│  호전환하시기 바랍니다"           │
└────────────────┬────────────────┘
                 │
                 ▼
┌─────────────────────────────────┐
│ Case 11: 3초 대기 [NEW]          │
│ (무음 또는 대기음)                │
└────────────────┬────────────────┘
                 │
                 ▼
┌─────────────────────────────────┐
│ Case 12: 시스템 인사말 [NEW]     │
│ "안녕하세요? 항공권 인증          │
│  결재 시스템입니다"               │
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

### 8.1 정상 케이스 - 주문조회 성공

```
테스트: 주문조회 성공 후 삼자통화 대기 흐름
조건: 유효한 휴대폰 번호 입력, 해당 번호로 주문 존재
기대결과:
  1. "휴대폰 번호를 입력 후 우물 정자를 눌러주세요" 재생
  2. 고객 전화번호 입력 (예: 01012345678#)
  3. 주문조회 API 호출 → 성공
  4. "삼초 이내에 삼자 통화 또는 호전환하시기 바랍니다" 재생
  5. 3초 대기 (대기음 재생)
  6. "안녕하세요? 항공권 인증 결재 시스템입니다" 재생
  7. "고객님의 항공권 결제 금액은 N건, 총 승인금액은 XXX원입니다" 재생
```

### 8.2 주문조회 실패 케이스 - 전화번호 재입력

```
테스트: 주문조회 실패 시 전화번호 재입력
조건: 유효한 휴대폰 번호 입력, 해당 번호로 주문 없음
기대결과:
  1. "휴대폰 번호를 입력 후 우물 정자를 눌러주세요" 재생
  2. 고객 전화번호 입력 (예: 01012345678#)
  3. 주문조회 API 호출 → 실패 (주문 없음)
  4. 즉시 "휴대폰 번호를 입력 후 우물 정자를 눌러주세요" 재생 (Case 1로 복귀)
  5. 고객이 다른 전화번호 입력 가능
```

### 8.3 음성 파일 누락 케이스

```
테스트: 음성 파일 누락 시 동작
조건: transfer_guide.wav 또는 intro.wav 파일 없음
기대결과:
  - 오류 없이 다음 단계로 진행
  - 시스템 로그에 파일 누락 경고 기록
```

### 8.4 삼자통화 전환 케이스

```
테스트: 삼자통화 전환 시 통화 유지
조건: 3초 대기 중 상담원이 삼자통화 연결
기대결과:
  - 고객과 상담원 모두 다음 멘트 청취 가능
  - 시나리오 정상 진행
```

---

## 9. 구현 체크리스트

### 9.1 코드 수정
- [ ] KICC_Travelport_Scenario.cpp 수정
  - [ ] KICC_getMultiOrderInfo() 수정: 성공 시 Case 10으로 분기
  - [ ] KICC_getMultiOrderInfo() 수정: 실패 시 Case 1로 복귀
  - [ ] KICC_ArsScenarioStart() 수정: Case 10, 11, 12 신규 추가

### 9.2 음성 파일 준비
- [ ] transfer_guide.wav 녹음
- [ ] intro.wav 녹음
- [ ] wait_3sec.wav 생성 (3초 대기음)

### 9.3 테스트
- [ ] 단위 테스트: 주문조회 성공 → 삼자통화 안내 흐름 확인
- [ ] 단위 테스트: 주문조회 실패 → 전화번호 재입력 흐름 확인
- [ ] 통합 테스트: 전체 시나리오 흐름 확인
- [ ] 음성 테스트: 모든 멘트 정상 재생 확인

---

## 10. 변경 이력

| 버전 | 날짜 | 작성자 | 변경 내용 |
|------|------|--------|----------|
| 1.0 | 2025-12-20 | Claude | 최초 작성 |
| 1.1 | 2025-12-21 | Claude | 흐름 변경: 주문조회 성공 후 삼자통화 안내, 실패 시 전화번호 재입력 |
