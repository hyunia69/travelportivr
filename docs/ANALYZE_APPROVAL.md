# 복수건 결제 결과별 로직 분석

## 문서 개요

이 문서는 Travelport IVR 시나리오에서 **복수건 결제 처리 후** 결과에 따라 실행되는 로직과 안내 멘트를 분석합니다.

**분석 케이스**:
- **Case A**: 모두 성공 (3건 중 3건 승인)
- **Case B**: 부분 실패 (3건 중 2건 승인, 1건 실패)
- **Case C**: 전부 실패 (3건 중 0건 승인, 3건 실패)

---

## 1. 결제 처리 흐름 개요

```
KICC_CardPw (state 1)
    │
    ▼ (다중 주문 모드)
KICC_ProcessMultiPayments(0)
    │
    ▼
[state 0] 결제 루프 초기화
    │
    ▼
"결제 요청 중입니다" 멘트 재생 (한 번만)
    │
    ▼
[state 7] 결제 루프 시작 ─────────────────────────────┐
    │                                                  │
    ▼                                                  │
[state 1] 주문 데이터 준비 ◄──────────────────────────┤
    │                                                  │
    ▼                                                  │ (다음 주문)
KiccPaymemt_host() - KICC 승인 요청                    │
    │                                                  │
    ▼                                                  │
[state 2] 결제 결과 확인                               │
    │                                                  │
    ▼                                                  │
setPayLog_host() - 결제 로그 DB 저장                   │
    │                                                  │
    ▼                                                  │
[state 3] 로그 저장 결과 확인                          │
    │                                                  │
    ▼ (REPLY_CODE == "0000" 성공 시)                   │
upOrderPayState_host() - 주문 상태 업데이트            │
    │                                                  │
    ▼                                                  │
[state 4] 다음 주문으로 이동 ─────────────────────────┘
    │
    ▼ (모든 주문 처리 완료)
KICC_MultiPaymentSummary(0)
    │
    ▼
[state 0] 최종 요약 TTS 재생
    │
    ▼
[state 1] TTS 파일 재생 후 종료
    │
    ▼
KICC_ExitSvc(0) - 서비스 종료
```

---

## 2. Case A: 모두 성공 (3건 중 3건 승인)

### 2.1 결제 루프 초기화 [KICC_ProcessMultiPayments state 0]

**소스 위치**: `KICC_Travelport_Scenario.cpp:565-574`

```cpp
case 0:
    // 결제 루프 초기화
    pScenario->m_nCurrentOrderIdx = 0;
    pScenario->m_MultiOrders.nProcessedCount = 0;
    pScenario->m_MultiOrders.nFailedCount = 0;
    memset(pScenario->m_MultiOrders.szFailedOrders, 0x00, ...);

    info_printf(localCh, "%d건 주문 처리 시작", nTotalCount);
    return KICC_ProcessMultiPayments(1);
```

**안내 멘트**: 없음 (내부 로그만)

---

### 2.2 첫 번째 주문 처리 [state 1 → state 2 → state 3 → state 4 → state 5]

#### Step 1: 주문 데이터 준비 및 대기 안내 [state 1 → state 6]

**소스 위치**: `KICC_Travelport_Scenario.cpp:576-647`

- 현재 주문(인덱스 0)을 `m_CardResInfo`에 복사
- 가맹점ID, 주문번호, 상품명, 고객명, 금액 설정
- 카드 정보(카드번호, 유효기간, 비밀번호) 복사
- **대기 안내 멘트 재생** → state 6으로 이동
- [state 6] `KiccPaymemt_host()` 호출하여 KICC 승인 요청

**안내 멘트**:
> **"결제 요청 중입니다. 잠시만 기다려 주시기 바랍니다."**
> - 음성 파일: `ment/Travelport/pay_request_wait`

#### Step 2: 결제 결과 확인 [state 2]

**소스 위치**: `KICC_Travelport_Scenario.cpp:649-713`

- `REPLY_CODE == "0000"` 확인 → 성공
- `nProcessedCount++` (성공 카운트 증가)
- `setPayLog_host()` 호출하여 결제 로그 DB 저장

**안내 멘트**: 없음

#### Step 3: 로그 저장 결과 확인 [state 3]

**소스 위치**: `KICC_Travelport_Scenario.cpp:716-751`

- 결제 성공 시: `upOrderPayState_host()` 호출하여 주문 상태 업데이트

**안내 멘트**: 없음

#### Step 4: 개별 승인 완료 안내 [state 4]

**소스 위치**: `KICC_Travelport_Scenario.cpp:753-777`

```cpp
case 4:
    // 결제 성공 안내 멘트 재생
    new_guide();
    set_guide(VOC_WAVE_ID, "ment/Travelport/pay_success_msg");
    setPostfunc(POST_PLAY, KICC_ProcessMultiPayments, 5, 0);
    return send_guide(NODTMF);
```

**안내 멘트**:
> **"결제가 완료되었습니다."**
> - 음성 파일: `ment/Travelport/pay_success_msg`

#### Step 5: 다음 주문으로 이동 [state 5]

**소스 위치**: `KICC_Travelport_Scenario.cpp:779-790`

```cpp
case 5:
    // 다음 주문으로 이동
    pScenario->m_nCurrentOrderIdx++;
    return KICC_ProcessMultiPayments(1);
```

**안내 멘트**: 없음

---

### 2.3 두 번째 주문 처리 (동일 반복)

[state 1 → state 2 → state 3 → state 4 → state 5]

**안내 멘트**:
> **"결제가 완료되었습니다."**

---

### 2.4 세 번째 주문 처리 (동일 반복)

[state 1 → state 2 → state 3 → state 4 → state 5]

**안내 멘트**:
> **"결제가 완료되었습니다."**

---

### 2.5 모든 주문 완료 확인 [state 1 → KICC_MultiPaymentSummary]

**소스 위치**: `KICC_Travelport_Scenario.cpp:577-583`

```cpp
case 1:
    // 모든 주문 처리 완료 확인
    if (nCurrentIdx >= nTotalCount) {
        info_printf(localCh, "모든 주문 처리 완료: 성공=%d, 실패=%d", ...);
        return KICC_MultiPaymentSummary(0);
    }
```

**안내 멘트**: 없음

---

### 2.6 최종 요약 안내 [KICC_MultiPaymentSummary state 0]

**소스 위치**: `KICC_Travelport_Scenario.cpp:806-833`

```cpp
case 0:
    if (pScenario->m_MultiOrders.nFailedCount == 0) {
        // 모두 성공
        if (TTS_Play) {
            setPostfunc(POST_NET, KICC_MultiPaymentSummary, 1, 0);
            return TTS_Play(
                (*lpmt)->chanID, 92,
                "%d건의 결제가 완료되었습니다. 담당직원을 연결해 드리겠습니다.",
                pScenario->m_MultiOrders.nProcessedCount
            );
        }
    }
```

**안내 멘트 (TTS)**:
> **"3건의 결제가 완료되었습니다. 담당직원을 연결해 드리겠습니다."**

---

### 2.7 TTS 파일 재생 및 종료 [KICC_MultiPaymentSummary state 1]

**소스 위치**: `KICC_Travelport_Scenario.cpp:893-921`

```cpp
case 1:
    if (strlen(pScenario->szTTSFile) > 0) {
        new_guide();
        set_guide(VOC_TTS_ID, TTSFile);
        setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
        return send_guide(NODTMF);
    }
    return KICC_ExitSvc(0);
```

**안내 멘트**: TTS 생성 파일 재생 (위 TTS 멘트의 음성 파일)

---

### 2.8 서비스 종료 [KICC_ExitSvc state 0]

**소스 위치**: `KICC_Travelport_Scenario.cpp:315-321`

```cpp
case 0:
    new_guide();
    set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\service_end");
    setPostfunc(POST_PLAY, KICC_ExitSvc, 0xffff, 0);
    return send_guide(NODTMF);
```

**안내 멘트**:
> **"이용해 주셔서 감사합니다."** (또는 유사한 마지막 인사말)
> - 음성 파일: `ment\_common\common_audio\service_end`

---

### 2.9 통화 종료 [KICC_ExitSvc state 0xffff]

**소스 위치**: `KICC_Travelport_Scenario.cpp:329-331`

```cpp
case 0xffff:
    (*lpmt)->Myexit_service = NULL;
    return goto_hookon();
```

**안내 멘트**: 없음 (통화 종료)

---

## 3. Case B: 부분 실패 (3건 중 2건 승인, 1건 실패)

### 3.1 개별 주문 처리 과정

각 주문별로 `KICC_ProcessMultiPayments`의 state 1 → 2 → 3 과정을 거칩니다.

#### 성공한 주문 (1건째, 2건째)

- state 2: `REPLY_CODE == "0000"` → `nProcessedCount++`
- state 3: `upOrderPayState_host()` 호출
- state 4: **"결제가 완료되었습니다."** 멘트 재생
- state 5: 다음 주문으로 이동

#### 실패한 주문 (3건째)

**소스 위치**: `KICC_Travelport_Scenario.cpp:691-708`

```cpp
else {
    // 실패
    pScenario->m_MultiOrders.nFailedCount++;

    // 실패한 주문 목록에 추가
    if (strlen(pScenario->m_MultiOrders.szFailedOrders) > 0) {
        strcat_s(pScenario->m_MultiOrders.szFailedOrders, ", ");
    }
    strcat_s(pScenario->m_MultiOrders.szFailedOrders, szCurrentOrderNo);
}
```

- state 2: `REPLY_CODE != "0000"` → `nFailedCount++`, 실패 주문번호 기록
- state 3: 결제 실패 → 주문 상태 업데이트 생략, 바로 다음 주문으로 이동

**안내 멘트**: 없음 (개별 실패 시 별도 멘트 없음)

---

### 3.2 최종 요약 안내 [KICC_MultiPaymentSummary state 0]

**소스 위치**: `KICC_Travelport_Scenario.cpp:873-888`

```cpp
else {
    // 부분 실패 (일부 성공, 일부 실패)
    if (TTS_Play) {
        setPostfunc(POST_NET, KICC_MultiPaymentSummary, 1, 0);
        return TTS_Play(
            (*lpmt)->chanID, 92,
            "%d건은 결제가 완료되었으며 %d건은 결제 실패하였습니다. 담당직원을 연결해 드리겠습니다.",
            pScenario->m_MultiOrders.nProcessedCount,
            pScenario->m_MultiOrders.nFailedCount
        );
    }
}
```

**안내 멘트 (TTS)**:
> **"2건은 결제가 완료되었으며 1건은 결제 실패하였습니다. 담당직원을 연결해 드리겠습니다."**

---

### 3.3 TTS 파일 재생 및 종료 [state 1 → KICC_ExitSvc]

모두 성공 케이스와 동일하게 TTS 파일 재생 후 `KICC_ExitSvc(0)`으로 종료됩니다.

**안내 멘트**:
> **"이용해 주셔서 감사합니다."**

---

### 3.4 실제 고객 청취 시나리오 (부분 실패)

```
[1] "결제가 완료되었습니다."                    ← 첫 번째 주문 승인
[2] "결제가 완료되었습니다."                    ← 두 번째 주문 승인
    (세 번째 주문 실패 - 별도 멘트 없음)
[3] "2건은 결제가 완료되었으며                  ← 최종 요약
     1건은 결제 실패하였습니다.
     담당직원을 연결해 드리겠습니다."
[4] "이용해 주셔서 감사합니다."                  ← 종료 인사
[통화 종료]
```

---

### 3.5 변수 상태 변화 (부분 실패 시)

| 시점 | m_nCurrentOrderIdx | nProcessedCount | nFailedCount | szFailedOrders |
|------|-------------------|-----------------|--------------|----------------|
| 초기화 후 | 0 | 0 | 0 | "" |
| 1건 승인 후 | 1 | 1 | 0 | "" |
| 2건 승인 후 | 2 | 2 | 0 | "" |
| 3건 실패 후 | 3 | 2 | 1 | "ORDER003" |
| 요약 진입 시 | 3 | 2 | 1 | "ORDER003" |

---

## 4. Case C: 전부 실패 (3건 중 0건 승인, 3건 실패)

### 4.1 개별 주문 처리 과정

모든 주문이 실패하므로 각 주문별로:

- state 2: `REPLY_CODE != "0000"` → `nFailedCount++`, 실패 주문번호 기록
- state 3: 결제 실패 → 주문 상태 업데이트 생략, 바로 다음 주문으로 이동

**안내 멘트**: 없음 (개별 실패 시 별도 멘트 없음)

---

### 4.2 최종 요약 - 재시도 가능 [KICC_MultiPaymentSummary state 0]

**조건**: `nProcessedCount == 0` && `m_nCardRetryCount < MAX_CARD_RETRY`

**소스 위치**: `KICC_Travelport_Scenario.cpp:835-857`

```cpp
else if (pScenario->m_MultiOrders.nProcessedCount == 0) {
    // 전부 실패
    if (pScenario->m_nCardRetryCount < MAX_CARD_RETRY) {
        // 재입력 가능 - 안내 후 사용자 선택
        pScenario->m_nCardRetryCount++;

        if (TTS_Play) {
            setPostfunc(POST_NET, KICC_MultiPaymentSummary, 2, 0);
            return TTS_Play(
                (*lpmt)->chanID, 92,
                "%d건 결제가 모두 실패하였습니다. 다시 결제를 진행하시려면 1번, 담당 직원을 연결하시려면 2번을 눌러 주시기 바랍니다.",
                pScenario->m_MultiOrders.nFailedCount
            );
        }
    }
}
```

**안내 멘트 (TTS)**:
> **"3건 결제가 모두 실패하였습니다. 다시 결제를 진행하시려면 1번, 담당 직원을 연결하시려면 2번을 눌러 주시기 바랍니다."**

---

### 4.3 DTMF 입력 대기 및 처리 [state 2 → state 4]

**소스 위치**: `KICC_Travelport_Scenario.cpp:923-1017`

#### TTS 재생 완료 후 [state 2]

- TTS 파일 재생 후 DTMF 입력 대기 (1자리, 10초 타임아웃)

#### DTMF 입력 결과 처리 [state 4]

```cpp
case 4:
    if (cDTMF == '1') {
        // 1번: 다시 결제 진행 - 카드번호 입력으로 복귀
        return KICC_CardInput(0);
    }
    else if (cDTMF == '2') {
        // 2번: 담당 직원 연결 - 종료
        return KICC_ExitSvc(0);
    }
    else {
        // 잘못된 입력 또는 타임아웃 - 재안내 (최대 3회)
        if (s_nDTMFRetryCount >= 3) {
            return KICC_ExitSvc(0);  // 자동 종료
        }
        // 멘트 재안내
        return TTS_Play(..., "다시 결제를 진행하시려면 1번, 담당 직원을 연결하시려면 2번을 눌러 주시기 바랍니다.");
    }
```

**사용자 선택에 따른 분기**:
- **1번 선택**: `KICC_CardInput(0)` → 카드번호 재입력부터 다시 시작
- **2번 선택**: `KICC_ExitSvc(0)` → 종료
- **잘못된 입력/타임아웃**:
  > **"다시 결제를 진행하시려면 1번, 담당 직원을 연결하시려면 2번을 눌러 주시기 바랍니다."**
  > (최대 3회 재안내 후 자동 종료)

---

### 4.4 최종 요약 - 재시도 초과 [KICC_MultiPaymentSummary state 0]

**조건**: `nProcessedCount == 0` && `m_nCardRetryCount >= MAX_CARD_RETRY`

**소스 위치**: `KICC_Travelport_Scenario.cpp:859-871`

```cpp
else {
    // 재입력 횟수 초과 - 안내 후 종료
    if (TTS_Play) {
        setPostfunc(POST_NET, KICC_MultiPaymentSummary, 1, 0);
        return TTS_Play(
            (*lpmt)->chanID, 92,
            "결제에 실패하였습니다. 다시 전화해 주시기 바랍니다."
        );
    }
}
```

**안내 멘트 (TTS)**:
> **"결제에 실패하였습니다. 다시 전화해 주시기 바랍니다."**

이후 `KICC_ExitSvc(0)`으로 종료됩니다.

---

### 4.5 실제 고객 청취 시나리오 (전부 실패 - 1번 선택 시)

```
[1] (첫 번째 주문 실패 - 별도 멘트 없음)
[2] (두 번째 주문 실패 - 별도 멘트 없음)
[3] (세 번째 주문 실패 - 별도 멘트 없음)
[4] "3건 결제가 모두 실패하였습니다.            ← 전부 실패 안내
     다시 결제를 진행하시려면 1번,
     담당 직원을 연결하시려면 2번을
     눌러 주시기 바랍니다."
[고객: 1번 입력]
[5] → 카드번호 입력 화면으로 복귀 (KICC_CardInput)
```

---

### 4.6 실제 고객 청취 시나리오 (전부 실패 - 2번 선택 시)

```
[1] (첫 번째 주문 실패 - 별도 멘트 없음)
[2] (두 번째 주문 실패 - 별도 멘트 없음)
[3] (세 번째 주문 실패 - 별도 멘트 없음)
[4] "3건 결제가 모두 실패하였습니다.            ← 전부 실패 안내
     다시 결제를 진행하시려면 1번,
     담당 직원을 연결하시려면 2번을
     눌러 주시기 바랍니다."
[고객: 2번 입력]
[5] "이용해 주셔서 감사합니다."                  ← 종료 인사
[통화 종료]
```

---

### 4.7 실제 고객 청취 시나리오 (전부 실패 - 재시도 초과 시)

```
[재시도 1회차 실패 후]
[재시도 2회차 실패 후]
[재시도 3회차 실패 - MAX_CARD_RETRY 초과]
[1] "결제에 실패하였습니다.                      ← 재시도 초과 안내
     다시 전화해 주시기 바랍니다."
[2] "이용해 주셔서 감사합니다."                  ← 종료 인사
[통화 종료]
```

---

### 4.8 변수 상태 변화 (전부 실패 시)

| 시점 | m_nCurrentOrderIdx | nProcessedCount | nFailedCount | m_nCardRetryCount |
|------|-------------------|-----------------|--------------|-------------------|
| 초기화 후 | 0 | 0 | 0 | 0 |
| 1건 실패 후 | 1 | 0 | 1 | 0 |
| 2건 실패 후 | 2 | 0 | 2 | 0 |
| 3건 실패 후 | 3 | 0 | 3 | 0 |
| 요약 진입 시 | 3 | 0 | 3 | 1 (증가) |
| 1번 선택 후 | - | - | - | 1 (유지) |

---

## 5. 안내 멘트 요약 (모든 케이스)

### 5.1 Case A: 모두 성공 (3건 중 3건 승인)

| 순서 | 함수 | State | 안내 멘트 | 비고 |
|------|------|-------|-----------|------|
| 1 | KICC_ProcessMultiPayments | 0 | "결제 요청 중입니다. 잠시만 기다려 주시기 바랍니다." | 결제 시작 전 (한 번만) |
| 2 | KICC_MultiPaymentSummary | 0 | "3건의 결제가 완료되었습니다. 담당직원을 연결해 드리겠습니다." | 최종 요약 (TTS) |
| 3 | KICC_ExitSvc | 0 | "이용해 주셔서 감사합니다." | 종료 인사 |

### 5.2 Case B: 부분 실패 (3건 중 2건 승인, 1건 실패)

| 순서 | 함수 | State | 안내 멘트 | 비고 |
|------|------|-------|-----------|------|
| 1 | KICC_ProcessMultiPayments | 0 | "결제 요청 중입니다. 잠시만 기다려 주시기 바랍니다." | 결제 시작 전 (한 번만) |
| 2 | KICC_MultiPaymentSummary | 0 | "2건은 결제가 완료되었으며 1건은 결제 실패하였습니다. 담당직원을 연결해 드리겠습니다." | 최종 요약 (TTS) |
| 3 | KICC_ExitSvc | 0 | "이용해 주셔서 감사합니다." | 종료 인사 |

### 5.3 Case C: 전부 실패 (3건 중 0건 승인, 3건 실패)

#### 재시도 가능 시 (m_nCardRetryCount < MAX_CARD_RETRY)

| 순서 | 함수 | State | 안내 멘트 | 비고 |
|------|------|-------|-----------|------|
| 1 | KICC_ProcessMultiPayments | 0 | "결제 요청 중입니다. 잠시만 기다려 주시기 바랍니다." | 결제 시작 전 (한 번만) |
| 2 | KICC_MultiPaymentSummary | 0 | "3건 결제가 모두 실패하였습니다. 다시 결제를 진행하시려면 1번, 담당 직원을 연결하시려면 2번을 눌러 주시기 바랍니다." | 전부 실패 안내 (TTS) |
| 3-A | KICC_CardInput | 0 | (카드번호 입력 안내) | 1번 선택 시 |
| 3-B | KICC_ExitSvc | 0 | "이용해 주셔서 감사합니다." | 2번 선택 시 |

#### 재시도 초과 시 (m_nCardRetryCount >= MAX_CARD_RETRY)

| 순서 | 함수 | State | 안내 멘트 | 비고 |
|------|------|-------|-----------|------|
| 1 | KICC_MultiPaymentSummary | 0 | "결제에 실패하였습니다. 다시 전화해 주시기 바랍니다." | 재시도 초과 안내 (TTS) |
| 2 | KICC_ExitSvc | 0 | "이용해 주셔서 감사합니다." | 종료 인사 |

---

## 6. 관련 음성 파일

| 파일 경로 | 내용 | 사용 위치 |
|-----------|------|-----------|
| `ment/Travelport/pay_request_wait` | "결제 요청 중입니다. 잠시만 기다려 주시기 바랍니다." | 각 주문 승인 요청 전 |
| `ment/Travelport/pay_success_msg` | "결제가 완료되었습니다." | ~~개별 주문 승인 시~~ (미사용) |
| `ment\_common\common_audio\service_end` | 종료 인사말 | 서비스 종료 시 |
| `ment\wait_sound` | 대기음 | 전부 실패 시 DTMF 입력 대기 |
| `ment\TTS_TimeOut` | TTS 서버 오류 안내 | TTS 서버 오류 시 |

### TTS 생성 멘트

| 케이스 | TTS 멘트 |
|--------|----------|
| 모두 성공 | "%d건의 결제가 완료되었습니다. 담당직원을 연결해 드리겠습니다." |
| 부분 실패 | "%d건은 결제가 완료되었으며 %d건은 결제 실패하였습니다. 담당직원을 연결해 드리겠습니다." |
| 전부 실패 (재시도 가능) | "%d건 결제가 모두 실패하였습니다. 다시 결제를 진행하시려면 1번, 담당 직원을 연결하시려면 2번을 눌러 주시기 바랍니다." |
| 전부 실패 (재안내) | "다시 결제를 진행하시려면 1번, 담당 직원을 연결하시려면 2번을 눌러 주시기 바랍니다." |
| 전부 실패 (재시도 초과) | "결제에 실패하였습니다. 다시 전화해 주시기 바랍니다." |

---

## 7. 변수 상태 변화 (모두 성공 케이스)

| 시점 | m_nCurrentOrderIdx | nProcessedCount | nFailedCount |
|------|-------------------|-----------------|--------------|
| 초기화 후 | 0 | 0 | 0 |
| 1건 승인 후 | 1 | 1 | 0 |
| 2건 승인 후 | 2 | 2 | 0 |
| 3건 승인 후 | 3 | 3 | 0 |
| 요약 진입 시 | 3 | 3 | 0 |

---

## 8. 참고 사항

### 8.1 단일 주문 vs 다중 주문 분기

`KICC_CardPw(state 1)`에서 다중 주문 모드 여부를 확인합니다:

```cpp
// KICC_Travelport_Scenario.cpp:1432-1441
if (pScenario->m_bMultiOrderMode && pScenario->m_MultiOrders.nOrderCount > 1) {
    // 다중 주문 처리 모드
    return KICC_ProcessMultiPayments(0);
}
else {
    // 단일 주문 처리 모드 (기존 방식)
    setPostfunc(POST_NET, KICC_payARS, 0, 0);
    return KiccPaymemt_host(90);
}
```

### 8.2 성공 판단 기준

결제 성공 여부는 `REPLY_CODE == "0000"`으로 판단합니다:

```cpp
// KICC_Travelport_Scenario.cpp:678-681
if (strcmp(pScenario->m_CardResInfo.REPLY_CODE, "0000") == 0) {
    bSuccess = TRUE;
    pScenario->m_PayResult = 1;  // 성공 시 m_PayResult도 업데이트
}
```

### 8.3 재입력 횟수 초기화

모든 결제 성공 시 카드 재입력 횟수가 초기화됩니다:

```cpp
// KICC_Travelport_Scenario.cpp:822-823
// [MODIFIED] 재입력 횟수 초기화 (성공 시)
pScenario->m_nCardRetryCount = 0;
```

---

## 9. 수정 이력

### 9.1 [완료] 개별 승인 완료 멘트 삭제

**수정일**: 2025-12-18

**변경 내용**:
- `KICC_ProcessMultiPayments` state 4에서 개별 승인 완료 멘트 삭제
- state 5 제거 (state 4에 통합)
- 최종 요약 멘트만 유지

**수정 전**:
```
[1] "결제가 완료되었습니다."           ← 개별 멘트 (삭제됨)
[2] "결제가 완료되었습니다."           ← 개별 멘트 (삭제됨)
[3] "결제가 완료되었습니다."           ← 개별 멘트 (삭제됨)
[4] "3건의 결제가 완료되었습니다. 담당직원을 연결해 드리겠습니다."
[5] "이용해 주셔서 감사합니다."
```

**수정 후**:
```
[1] "3건의 결제가 완료되었습니다. 담당직원을 연결해 드리겠습니다."
[2] "이용해 주셔서 감사합니다."
```

---

### 9.2 [완료] 결제 요청 대기 멘트 추가

**수정일**: 2025-12-18

**변경 내용**:
- `KICC_ProcessMultiPayments` state 0에서 대기 안내 멘트 **한 번만** 재생
- state 7 신규 추가 (대기 멘트 재생 완료 후 결제 루프 시작)
- 각 주문별이 아닌, 전체 결제 시작 전 한 번만 안내

**추가된 멘트**:
> **"결제 요청 중입니다. 잠시만 기다려 주시기 바랍니다."**
> - 음성 파일: `ment/Travelport/pay_request_wait`

**수정 후 전체 흐름** (3건 모두 성공 케이스):
```
[1] "결제 요청 중입니다. 잠시만 기다려 주시기 바랍니다."  ← 결제 시작 전 (한 번만)
    (KICC 승인 요청 중... 3건 순차 처리)
[2] "3건의 결제가 완료되었습니다. 담당직원을 연결해 드리겠습니다."
[3] "이용해 주셔서 감사합니다."
```

**수정 위치**: `KICC_ProcessMultiPayments` state 0, state 7 추가

---

## 10. 문서 이력

| 일자 | 작성자 | 내용 |
|------|--------|------|
| 2025-12-18 | Claude | 최초 작성 - 복수건 결제 모두 승인 시 로직 분석 |
| 2025-12-18 | Claude | 부분 실패(Case B) 및 전부 실패(Case C) 케이스 추가 |
| 2025-12-18 | Claude | 수정 요청사항 추가 - 개별 멘트 삭제, 최종 요약 멘트만 유지 |
| 2025-12-18 | Claude | [구현완료] 개별 승인 완료 멘트 삭제 (state 4, 5 통합) |
| 2025-12-18 | Claude | [구현완료] 결제 요청 대기 멘트 추가 (state 6 신규) |
