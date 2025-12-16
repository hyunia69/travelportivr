# 카드번호 확인 단계 스킵 설계 문서

**작성일**: 2025-12-16
**대상 파일**: `KICC_Travelport_Scenario.cpp`
**대상 함수**: `KICC_CardInput()`

---

## 1. 현재 상태 분석

### 1.1 현재 흐름 (KICC_CardInput 함수)

```
┌─────────────────────────────────────────────────────────────────────┐
│ [State 0] 카드번호 입력 요청                                        │
│ ──────────────────────────────────────                              │
│ - DB 모드: "카드번호 뒤 4자리" 안내 → 4자리 DTMF 대기              │
│ - 기존 모드: "카드번호 13~16자리" 안내 → 17자리 DTMF 대기           │
└─────────────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────────────┐
│ [State 1] 카드번호 검증 (라인 1893-1985)                            │
│ ──────────────────────────────────────                              │
│ - DB 모드: 4자리 길이 검증 → DB 앞자리 + 입력값 조합                │
│ - 기존 모드: 13~16자리 형식 검증                                    │
│                                                                     │
│ ※ 검증 통과 시:                                                    │
│   → TTS_Play()로 "고객님께서 누르신 카드번호는 X,X,X... 번입니다"  │
│   → setPostfunc(POST_NET, KICC_CardInput, 2, 0)                    │
│   → TTS 재생 완료 후 State 2로 이동                                │
└─────────────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────────────┐
│ [State 2] TTS 재생 완료 및 확인 멘트 (라인 1987-2009)               │
│ ──────────────────────────────────────                              │
│ - TTS 타임아웃 체크                                                 │
│ - TTS 파일 재생 (pScenario->szTTSFile)                             │
│ - "ment\_common\common_audio\input_confirm" 안내 재생              │
│ - setPostfunc(POST_DTMF, KICC_CardInput, 3, 0) → 1자리 DTMF 대기   │
└─────────────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────────────┐
│ [State 3] 사용자 확인 (라인 2010-2057)                              │
│ ──────────────────────────────────────                              │
│ - DTMF '1' (맞습니다):                                              │
│   - DB 모드: KICC_CardPw(0) 호출                                   │
│   - 기존 모드: KICC_EffecDate(0) 호출                              │
│ - DTMF '2' (아니오): KICC_CardInput(0) 재입력                       │
└─────────────────────────────────────────────────────────────────────┘
```

### 1.2 문제점

현재 흐름에서 카드번호가 **형식오류 없이 정상 입력**되더라도:
1. **TTS 재생** (State 1 → State 2): 카드번호를 음성으로 읽어줌
2. **확인 멘트 재생** (State 2): "맞으면 1번, 틀리면 2번" 안내
3. **사용자 입력 대기** (State 3): 1 또는 2번 입력 대기

이 **3단계의 확인 과정**이 불필요하게 시간을 소요함.

---

## 2. 요구사항

### 2.1 목표
카드번호 입력 후 **형식오류가 없으면** 확인 단계(State 2, 3)를 **스킵**하고 바로 다음 단계로 진행

### 2.2 적용 범위

| 모드 | 현재 다음 단계 | 변경 후 동작 |
|------|---------------|-------------|
| DB 모드 (`m_bUseDbCardInfo = true`) | KICC_CardPw(0) | **동일** - 카드 비밀번호 입력 |
| 기존 모드 (`m_bUseDbCardInfo = false`) | KICC_EffecDate(0) | **동일** - 유효기간 입력 |

### 2.3 제약 조건
- 기존 기능(오류 시 재입력)은 유지
- TTS 재생/확인 로직은 코드 내 보존 (향후 활성화 가능)
- State 2, 3 코드는 삭제하지 않고 조건부 스킵

---

## 3. 설계

### 3.1 변경 전/후 흐름 비교

```
[변경 전]
State 0 → State 1 (검증) → State 2 (TTS+확인) → State 3 (1/2 입력) → 다음 단계

[변경 후]
State 0 → State 1 (검증) → 다음 단계 (바로 이동)
```

### 3.2 수정 위치

**파일**: `KICC_Travelport_Scenario.cpp`
**함수**: `KICC_CardInput(int state)`
**수정 지점**: State 1 내부, 검증 성공 후 처리 부분 (라인 1961-1985)

### 3.3 수정 내용

#### 현재 코드 (라인 1961-1985)
```cpp
// State 1 검증 성공 후
new_guide();

info_printf(localCh, "KICC_CardInput [%d]  Card 번호 입력부>확인 부(TTS)", state);
eprintf("KICC_CardInput [%d]  Card 번호 입력부>확인 부(TTS)", state);
if (TTS_Play)
{
    char TTSBuf[1024 + 1] = { 0x00, };
    int TTsLen = strlen(pScenario->m_CardInfo.Card_Num);
    for (int nRep = 0, nRep2 = 0;; nRep++)
    {
        if (TTsLen < 1) break;
        TTSBuf[nRep2++] = pScenario->m_CardInfo.Card_Num[nRep];
        TTSBuf[nRep2++] = ',';
        TTsLen--;
    }

    setPostfunc(POST_NET, KICC_CardInput, 2, 0);
    return TTS_Play((*lpmt)->chanID, 92, "고객님께서 누르신 카드번호는, %s 번 입니다.", TTSBuf);
}
else
{
    set_guide(399);
    setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
    return send_guide(NODTMF);
}
```

#### 변경 후 코드
```cpp
// State 1 검증 성공 후
new_guide();

// ========================================
// [MODIFIED] 카드번호 확인 단계 스킵
// 형식오류가 없으면 바로 다음 단계로 진행
// ========================================
info_printf(localCh, "KICC_CardInput [%d]  Card 번호 검증 완료 - 확인 단계 스킵", state);
eprintf("KICC_CardInput [%d]  Card 번호 검증 완료: %s", state, pScenario->m_CardInfo.Card_Num);

// DB 사용 여부에 따라 다음 단계 결정
if (pScenario->m_bUseDbCardInfo) {
    // [DB 모드] 유효기간 DB에서 가져오기
    strncpy(pScenario->m_CardInfo.ExpireDt,
        pScenario->m_szDB_ExpireDate,
        sizeof(pScenario->m_CardInfo.ExpireDt) - 1);

    info_printf(localCh, "[KICC] DB 유효기간 사용: %s", pScenario->m_CardInfo.ExpireDt);

    // 주민번호 초기화 (빈 값)
    memset(pScenario->m_CardInfo.SecretNo, 0x00, sizeof(pScenario->m_CardInfo.SecretNo));

    info_printf(localCh, "[KICC] 카드번호 확인 스킵 → 비밀번호 입력으로 이동");
    return KICC_CardPw(0);
}
else {
    // [기존 모드] 유효기간 입력으로 이동
    info_printf(localCh, "[KICC] 카드번호 확인 스킵 → 유효기간 입력으로 이동");
    return KICC_EffecDate(0);
}

// ========================================
// [PRESERVED] 기존 TTS 확인 로직 (비활성화)
// 향후 필요시 활성화를 위해 코드 보존
// ========================================
#if 0  // 카드번호 확인 단계 비활성화
info_printf(localCh, "KICC_CardInput [%d]  Card 번호 입력부>확인 부(TTS)", state);
eprintf("KICC_CardInput [%d]  Card 번호 입력부>확인 부(TTS)", state);
if (TTS_Play)
{
    // ... 기존 TTS 로직 ...
}
#endif
```

### 3.4 State 2, 3 처리

State 2, 3 코드는 **삭제하지 않고 그대로 유지**합니다.
이유:
1. State 1에서 바로 다음 함수로 `return`하므로 State 2, 3에 도달하지 않음
2. 향후 확인 단계 복원이 필요할 경우 활용 가능
3. 코드 삭제 금지 원칙 준수 (조건부 분기로 처리)

---

## 4. 상세 구현 계획

### 4.1 수정 단계

| 단계 | 작업 | 라인 |
|------|------|------|
| 1 | State 1 내 검증 성공 후 로직 수정 | 1961-1985 |
| 2 | 기존 TTS 로직을 `#if 0` 블록으로 감싸기 | 1963-1985 |
| 3 | 새로운 스킵 로직 추가 | 1961 이후 |
| 4 | 로그 메시지 수정 | 해당 위치 |

### 4.2 영향 분석

| 항목 | 영향 |
|------|------|
| State 0 | **무관** - 입력 요청 로직 변경 없음 |
| State 1 | **수정** - 검증 후 바로 다음 단계로 이동 |
| State 2 | **무관** - 코드 유지, 도달 불가 |
| State 3 | **무관** - 코드 유지, 도달 불가 |
| 오류 처리 | **무관** - `send_error()` 동작 유지 |
| DB 모드 | **무관** - 기존 분기 로직 동일 |
| 기존 모드 | **적용** - 확인 스킵 후 유효기간 입력으로 |

### 4.3 테스트 시나리오

| 시나리오 | 입력 | 기대 결과 |
|----------|------|----------|
| DB 모드 정상 입력 | 4자리 숫자 | → KICC_CardPw(0) |
| DB 모드 형식 오류 | 3자리/5자리 | → send_error() → State 0 |
| 기존 모드 정상 입력 | 13~16자리 + *# | → KICC_EffecDate(0) |
| 기존 모드 형식 오류 | 12자리 이하 | → send_error() → State 0 |

---

## 5. 변경 전/후 시퀀스 다이어그램

### 5.1 변경 전

```
고객          IVR                          다음단계
 │            │                              │
 │──카드번호──▶│                              │
 │            │ [State 1] 검증               │
 │            │ TTS: "카드번호는 X,X,X..."   │
 │◀───TTS────│                              │
 │            │ [State 2] 확인멘트           │
 │◀─"1번/2번"─│                              │
 │───'1'────▶│                              │
 │            │ [State 3]                    │
 │            │───────────────────────────────▶│
```

### 5.2 변경 후

```
고객          IVR                          다음단계
 │            │                              │
 │──카드번호──▶│                              │
 │            │ [State 1] 검증               │
 │            │ (TTS/확인 스킵)              │
 │            │───────────────────────────────▶│
```

---

## 6. 코드 변경 상세

### 6.1 수정 대상 코드 (라인 1961-1985)

**기존 코드 전체를 다음으로 교체:**

```cpp
		new_guide();

		// ========================================
		// [MODIFIED] 카드번호 확인 단계 스킵
		// 형식오류가 없으면 바로 다음 단계로 진행
		// ========================================
		info_printf(localCh, "KICC_CardInput [%d]  Card 번호 검증 완료 - 확인 단계 스킵", state);
		eprintf("KICC_CardInput [%d]  Card 번호 검증 완료: %s", state, pScenario->m_CardInfo.Card_Num);

		// DB 사용 여부에 따라 다음 단계 결정
		if (pScenario->m_bUseDbCardInfo) {
			// [DB 모드] 유효기간 DB에서 가져오기
			strncpy(pScenario->m_CardInfo.ExpireDt,
				pScenario->m_szDB_ExpireDate,
				sizeof(pScenario->m_CardInfo.ExpireDt) - 1);

			info_printf(localCh, "[KICC] DB 유효기간 사용: %s", pScenario->m_CardInfo.ExpireDt);

			// 주민번호 초기화 (빈 값)
			memset(pScenario->m_CardInfo.SecretNo, 0x00, sizeof(pScenario->m_CardInfo.SecretNo));

			info_printf(localCh, "[KICC] 카드번호 확인 스킵 → 비밀번호 입력으로 이동");
			return KICC_CardPw(0);
		}
		else {
			// [기존 모드] 유효기간 입력으로 이동
			info_printf(localCh, "[KICC] 카드번호 확인 스킵 → 유효기간 입력으로 이동");
			return KICC_EffecDate(0);
		}

		// ========================================
		// [PRESERVED] 기존 TTS 확인 로직 (비활성화)
		// 향후 필요시 조건부 활성화를 위해 코드 보존
		// ========================================
#if 0  // 카드번호 확인 단계 비활성화
		info_printf(localCh, "KICC_CardInput [%d]  Card 번호 입력부>확인 부(TTS)", state);
		eprintf("KICC_CardInput [%d]  Card 번호 입력부>확인 부(TTS)", state);
		if (TTS_Play)
		{
			char TTSBuf[1024 + 1] = { 0x00, };
			int TTsLen = strlen(pScenario->m_CardInfo.Card_Num);
			for (int nRep = 0, nRep2 = 0;; nRep++)
			{
				if (TTsLen < 1) break;
				TTSBuf[nRep2++] = pScenario->m_CardInfo.Card_Num[nRep];
				TTSBuf[nRep2++] = ',';
				TTsLen--;
			}

			setPostfunc(POST_NET, KICC_CardInput, 2, 0);
			return TTS_Play((*lpmt)->chanID, 92, "고객님께서 누르신 카드번호는, %s 번 입니다.", TTSBuf);
		}
		else
		{
			set_guide(399);
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		}
#endif
	}
```

---

## 7. 롤백 계획

확인 단계 복원이 필요한 경우:

1. `#if 0`을 `#if 1`로 변경
2. 새로 추가한 스킵 로직을 `#if 0`으로 감싸기

또는 조건 변수를 사용한 동적 전환:

```cpp
// INI 파일 또는 멤버 변수로 제어
bool bSkipCardConfirm = true;  // true: 스킵, false: 확인

if (bSkipCardConfirm) {
    // 스킵 로직
} else {
    // 기존 TTS 확인 로직
}
```

---

## 8. 체크리스트

### 8.1 구현 전 체크리스트
- [x] 현재 흐름 분석 완료
- [x] State 1, 2, 3 동작 이해
- [x] DB 모드/기존 모드 분기 파악
- [x] 다음 단계 함수 확인 (KICC_CardPw, KICC_EffecDate)

### 8.2 구현 체크리스트
- [ ] State 1 내 스킵 로직 추가
- [ ] 기존 TTS 로직 `#if 0` 처리
- [ ] 로그 메시지 업데이트
- [ ] 빌드 확인

### 8.3 테스트 체크리스트
- [ ] DB 모드: 4자리 정상 입력 → 비밀번호 단계 진행
- [ ] DB 모드: 형식 오류 → 재입력 안내
- [ ] 기존 모드: 16자리 정상 입력 → 유효기간 단계 진행
- [ ] 기존 모드: 형식 오류 → 재입력 안내

---

## 9. 참고 사항

### 9.1 유사 수정 사례

**휴대폰번호 확인 스킵** (`KICC_ArsScenarioStart` 함수)
- 휴대폰번호 입력 후 확인 과정 생략
- 관련 커밋: `a825c05`

### 9.2 관련 문서
- `docs/ANALYZE_KICC.md` - ARS 시나리오 흐름 분석
- `docs/PLAN_TRAVELPORT_JUMIN.md` - 주민번호 입력 생략 계획
- `docs/IMPLEMENTATION_STATUS.md` - 구현 진행 상황
