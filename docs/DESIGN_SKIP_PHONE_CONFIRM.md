# DESIGN_SKIP_PHONE_CONFIRM.md

## 휴대폰 번호 확인 단계 생략 설계

**문서 작성일**: 2025-12-14
**대상 프로젝트**: KICC_Scenario_Travelport
**목적**: 휴대폰 번호 입력 후 확인 단계(1번/2번 선택)를 생략하고 바로 다음 단계로 진행

---

## 1. 현재 구현 상태 분석

### 1.1 전화번호 입력 흐름 (`KICC_ArsScenarioStart` 함수)

현재 시나리오의 전화번호 입력 흐름은 다음과 같습니다:

**함수 위치**: `KICC_Travelport_Scenario.cpp` 라인 2110-2256

| State | 동작 | 설명 |
|-------|------|------|
| 0 | 인사말 재생 | 초기 인사말 (현재 주석 처리됨) |
| 1 | 전화번호 입력 요청 | "전화 번호를 입력해 주세요" 멘트 재생 후 13자리까지 입력 대기 |
| 2 | 전화번호 유효성 검증 | 7~12자리, 010/011/012/016/017/018/019 시작 확인 |
| 3 | TTS로 번호 읽어주기 | "고객님께서 누르신 전화번호는 XXX번 입니다" + 확인 멘트 |
| 4 | 사용자 확인 | **1번: 맞음** → 다음 단계 / **2번: 다시 입력** → state 1로 복귀 |

```
[현재 흐름]
인사말(0) → 전화번호 입력(1) → 유효성 검증(2) → TTS 확인(3) → 1/2번 선택(4)
                                                                    ↓
                                                            1번: KICC_getMultiOrderInfo
                                                            2번: state 1 (재입력)
```

### 1.2 상세 코드 분석

#### State 2: 전화번호 유효성 검증 (라인 2149-2191)

```cpp
case 2:// 전화 번호 입력 처리
    if ((check_validform("*#:7:12", (*lpmt)->refinfo)) < 0)  // 7~12자리 검증
    {
        eprintf("KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>잘못 누르셨습니다.....", state);
        return send_error();
    }
    // 010/011/012/016/017/018/019 시작 검증
    if (strncmp((*lpmt)->dtmfs, "010", 3) != 0 && ...)
    {
        return send_error();
    }

    // TTS로 입력된 번호 읽어주기
    setPostfunc(POST_NET, KICC_ArsScenarioStart, 3, 0);
    return TTS_Play((*lpmt)->chanID, 92, "고객님께서 누르신 전화번호는, %s 번 입니다.", TTSBuf);
```

#### State 3: 확인 멘트 재생 (라인 2192-2213)

```cpp
case 3:
    // TTS 오류 처리
    if (pScenario->m_TTSAccess == -1) { ... }

    // "맞으면 1번, 다시 입력하시려면 2번을 눌러주세요" 멘트 재생
    set_guide(VOC_TTS_ID, TTSFile);
    set_guide(VOC_WAVE_ID, "ment/_common/common_audio/input_confirm");
    setPostfunc(POST_DTMF, KICC_ArsScenarioStart, 4, 0);
    return send_guide(1);  // 1자리 입력 대기
```

#### State 4: 사용자 확인 처리 (라인 2214-2244)

```cpp
case 4:
    if (!check_validdtmf(c, "12"))  // 1, 2만 허용
    {
        return send_error();
    }

    if (c == '1') //예
    {
        memcpy(pScenario->m_szInputTel, (*lpmt)->refinfo, ...);

        // 다중 주문 조회로 진행
        pScenario->m_bMultiOrderMode = TRUE;
        setPostfunc(POST_NET, KICC_getMultiOrderInfo, 0, 0);
        return getMultiOrderInfo_host(90);
    }
    else if (c == '2') //아니오
    {
        return KICC_ArsScenarioStart(1);  // 전화번호 재입력
    }
```

---

## 2. 수정 목표

### 2.1 목표 흐름

전화번호 입력 후 TTS 확인 및 1/2번 선택 단계를 **생략**하고, 유효성 검증 통과 즉시 다음 단계로 진행합니다.

```
[수정 후 흐름]
인사말(0) → 전화번호 입력(1) → 유효성 검증(2) → KICC_getMultiOrderInfo
                                    ↓
                               (TTS 확인 및 1/2번 선택 생략)
```

### 2.2 영향 범위

| 구분 | 수정 전 | 수정 후 |
|------|---------|---------|
| 사용자 경험 | 전화번호 확인 후 1번 추가 입력 필요 | 전화번호 입력만으로 진행 |
| 오류 복구 | 2번으로 재입력 가능 | 잘못 입력 시 처음부터 다시 |
| 통화 시간 | TTS + 1/2번 대기 시간 소요 | 해당 시간 단축 |

---

## 3. 상세 수정 계획

### 3.1 수정 대상 파일

| 파일 | 수정 내용 |
|------|----------|
| `KICC_Travelport_Scenario.cpp` | `KICC_ArsScenarioStart` 함수의 state 2 수정 |

### 3.2 코드 수정 상세

#### 수정 위치: State 2 (라인 2149-2191)

**현재 코드** (라인 2166-2191):
```cpp
        new_guide();

        info_printf(localCh, "KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>확인 부", state);
        eprintf("KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>확인 부", state);
        if (TTS_Play)
        {
            char TTSBuf[1024 + 1] = { 0x00, };
            int TTsLen = strlen((*lpmt)->refinfo);
            for (int nRep = 0, nRep2 = 0;; nRep++)
            {
                if (TTsLen < 1) break;
                TTSBuf[nRep2++] = (char)*((*lpmt)->refinfo + nRep);
                TTSBuf[nRep2++] = ',';
                TTsLen--;
            }

            setPostfunc(POST_NET, KICC_ArsScenarioStart, 3, 0);
            return TTS_Play((*lpmt)->chanID, 92, "고객님께서 누르신 전화번호는, %s 번 입니다.", TTSBuf);
        }
        else
        {
            set_guide(399);
            setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
            return send_guide(NODTMF);
        }
```

**수정 후 코드**:
```cpp
        new_guide();

        info_printf(localCh, "KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>확인 생략, 바로 주문조회 진행", state);
        eprintf("KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>확인 생략, 바로 주문조회 진행", state);

        // [MODIFIED] 전화번호 확인 단계 생략 - 바로 다음 단계로 진행
        // 입력된 전화번호 저장
        if (((CKICC_Scenario *)(*lpmt)->pScenario)->m_Myport == (*lpmt))
        {
            memset(pScenario->m_szInputTel, 0x00, sizeof(pScenario->m_szInputTel));
            memcpy(pScenario->m_szInputTel, (*lpmt)->refinfo, sizeof(pScenario->m_szInputTel) - 1);
        }

        // 다중 주문 조회로 바로 진행
        pScenario->m_bMultiOrderMode = TRUE;
        setPostfunc(POST_NET, KICC_getMultiOrderInfo, 0, 0);
        return getMultiOrderInfo_host(90);
```

### 3.3 삭제/비활성화 대상 코드

State 3, 4는 전화번호 확인 흐름에서만 사용되므로, 수정 후에는 **도달하지 않는 코드**가 됩니다.

**권장 처리 방법**: 기존 코드를 유지하되 주석으로 비활성화 사유 표기

```cpp
    case 3:  // [SKIP_PHONE_CONFIRM] 전화번호 확인 단계 생략으로 미사용
        // 기존 코드 유지...

    case 4:  // [SKIP_PHONE_CONFIRM] 전화번호 확인 단계 생략으로 미사용
        // 기존 코드 유지...
```

---

## 4. 조건부 컴파일 옵션 (권장)

롤백 및 A/B 테스트를 위해 조건부 컴파일 방식을 권장합니다.

### 4.1 매크로 정의

`KICC_Travelport_Scenario.h` 또는 `KICC_Common.h`에 추가:

```cpp
// 전화번호 확인 단계 생략 여부 (1: 생략, 0: 기존 유지)
#define SKIP_PHONE_CONFIRM  1
```

### 4.2 조건부 코드 적용

```cpp
case 2:// 전화 번호 입력 처리
    if ((check_validform("*#:7:12", (*lpmt)->refinfo)) < 0)
    {
        eprintf("KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>잘못 누르셨습니다.....", state);
        return send_error();
    }
    if (strncmp((*lpmt)->dtmfs, "010", 3) != 0 && ... )
    {
        return send_error();
    }
    new_guide();

#if SKIP_PHONE_CONFIRM
    // [MODIFIED] 전화번호 확인 단계 생략 - 바로 다음 단계로 진행
    info_printf(localCh, "KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>확인 생략, 바로 주문조회 진행", state);
    eprintf("KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>확인 생략, 바로 주문조회 진행", state);

    // 입력된 전화번호 저장
    if (((CKICC_Scenario *)(*lpmt)->pScenario)->m_Myport == (*lpmt))
    {
        memset(pScenario->m_szInputTel, 0x00, sizeof(pScenario->m_szInputTel));
        memcpy(pScenario->m_szInputTel, (*lpmt)->refinfo, sizeof(pScenario->m_szInputTel) - 1);
    }

    // 다중 주문 조회로 바로 진행
    pScenario->m_bMultiOrderMode = TRUE;
    setPostfunc(POST_NET, KICC_getMultiOrderInfo, 0, 0);
    return getMultiOrderInfo_host(90);
#else
    // 기존 코드: TTS 확인 후 1/2번 선택
    info_printf(localCh, "KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>확인 부", state);
    eprintf("KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>확인 부", state);
    if (TTS_Play)
    {
        // ... 기존 TTS 코드 ...
    }
#endif
```

---

## 5. 구현 체크리스트

### 5.1 코드 수정

- [ ] `SKIP_PHONE_CONFIRM` 매크로 정의 추가
- [ ] `KICC_ArsScenarioStart` state 2에서 조건부 분기 적용
- [ ] 로그 메시지에 확인 생략 표시 추가
- [ ] 전화번호 저장 로직을 state 2로 이동

### 5.2 테스트

- [ ] 전화번호 입력 후 바로 주문 조회로 진행되는지 확인
- [ ] 잘못된 전화번호 입력 시 오류 처리 정상 동작 확인
- [ ] `m_szInputTel`에 전화번호가 정상 저장되는지 확인
- [ ] 이후 결제 흐름 정상 동작 확인

### 5.3 빌드 및 배포

- [ ] Debug 구성으로 빌드 및 테스트
- [ ] Release 구성으로 빌드
- [ ] 운영 환경 배포 전 QA 테스트

---

## 6. 주의사항

### 6.1 사용자 경험 변화

- **장점**: 통화 시간 단축, 불필요한 버튼 입력 제거
- **단점**: 전화번호 오입력 시 재입력 기회 없음 (처음부터 다시 전화해야 함)

### 6.2 오류 복구 전략

현재는 2번을 눌러 재입력이 가능하지만, 수정 후에는 오입력 시 복구 방법이 없습니다.
필요 시 다른 방식의 재입력 기능을 고려할 수 있습니다:

- **옵션 A**: `#` 또는 `*` 키로 전화번호 재입력 기능 추가
- **옵션 B**: 주문 조회 실패 시 전화번호 재입력으로 복귀

### 6.3 롤백 방법

`SKIP_PHONE_CONFIRM` 매크로를 `0`으로 변경 후 재빌드하면 즉시 이전 동작으로 복귀됩니다.

```cpp
#define SKIP_PHONE_CONFIRM  0  // 기존 동작으로 복귀
```

---

## 7. 참고 자료

- **KICC_Travelport_Scenario.cpp** - 메인 시나리오 로직
  - `KICC_ArsScenarioStart` 함수: 라인 2110-2256
  - State 2 (전화번호 검증): 라인 2149-2191
  - State 3 (TTS 확인): 라인 2192-2213
  - State 4 (1/2번 선택): 라인 2214-2244
- **KICC_getMultiOrderInfo** - 다중 주문 조회 함수 (다음 단계)
