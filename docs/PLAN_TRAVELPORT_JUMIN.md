# PLAN_TRAVELPORT_JUMIN.md

## Travelport 시나리오 주민번호(생년월일) 입력 생략 수정 계획

**문서 작성일**: 2025-12-05
**대상 프로젝트**: KICC_Scenario_Travelport
**목적**: 주민번호(생년월일) 입력 단계를 생략하도록 시나리오 수정

---

## 1. 현재 구현 상태 분석

### 1.1 주민번호 입력 흐름

현재 시나리오에서 주민번호/생년월일 입력은 `KICC_JuminNo` 함수(1421-1613 라인)에서 처리됩니다.

**호출 지점**:
1. `KICC_EffecDate` → `KICC_JuminNo` (라인 1732)
   - 유효기간 입력 후 주민번호 입력으로 진행
2. `KICC_CardInput` (DB 카드 모드) → `KICC_JuminNo` (라인 1940)
   - DB에서 카드정보 조회 후 주민번호 입력으로 진행

**진행 지점**:
1. `KICC_JuminNo` → `KICC_InstallmentCConfrim` (라인 1594)
   - 일반 모드: 주민번호 입력 후 할부 확인으로 진행
2. `KICC_JuminNo` → `KICC_CardPw` (라인 1551)
   - DB 카드 모드: 주민번호 입력 후 카드 비밀번호 입력으로 진행

```
[현재 흐름]

일반 모드:
KICC_CardInput → KICC_EffecDate → KICC_JuminNo → KICC_InstallmentCConfrim → ...

DB 카드 모드:
KICC_CardInput → KICC_JuminNo → KICC_CardPw → KICC_InstallmentCConfrim → ...
```

### 1.2 관련 데이터 구조

**CARDINFO 구조체** (`KICC_Common.h`):
```cpp
typedef struct stCardInfo
{
    char Card_Num[20];      // 카드번호
    char ExpireDt[5];       // 유효기간
    char SecretNo[10+1];    // 주민번호(6자리) 또는 사업자번호(10자리)
    char Password[3];       // 카드 비밀번호 앞 2자리
    char InstPeriod[3];     // 할부 개월
    ...
} CARDINFO;
```

**KICC API 연동** (`KICCpayMent.cpp`):
- `SecretNo`는 `auth_value` 파라미터로 KICC API에 전송됨 (라인 647)
- `user_type` 결정: `SecretNo` 길이가 10자리면 사업자(02), 6자리면 개인(01) (라인 612-618)

```cpp
// KICCpayMent.cpp 라인 612-618
if (strlen(pScenario->m_CardInfo.SecretNo) >= 10)
    user_type = "02";  // 사업자
else
    user_type = "01";  // 개인

// 라인 647
lplfEP_CLI_DLL__set_value("auth_value", pScenario->m_CardInfo.SecretNo, ...);
```

---

## 2. 수정 범위

### 2.1 수정 대상 파일

| 파일 | 수정 내용 |
|------|----------|
| `KICC_Scenario_Travelport.cpp` | 흐름 변경: `KICC_JuminNo` 건너뛰기 |
| `KICCpayMent.cpp` | `auth_value` 빈 값 처리 (필요시) |
| `KICC_Common.h` | 변경 없음 (구조체 유지) |
| `KICC_Scenario_Travelport.h` | 변경 없음 |

### 2.2 영향 범위

1. **일반 카드 입력 모드**: `KICC_EffecDate` → 바로 `KICC_InstallmentCConfrim`으로 이동
2. **DB 카드 정보 모드**: `KICC_CardInput` → 바로 `KICC_CardPw`로 이동
3. **KICC API**: `auth_value`가 빈 값으로 전송됨
4. **결제 승인**: KICC 측에서 주민번호 없이 승인 가능한지 확인 필요

---

## 3. 상세 수정 계획

### 3.1 KICC_Scenario_Travelport.cpp 수정

#### 수정 1: KICC_EffecDate 함수 (라인 ~1732)

**현재 코드**:
```cpp
// 라인 1732 근처
LPMT->NextSubJob = KICC_JuminNo;
```

**수정 후**:
```cpp
// 주민번호 입력 생략 - 바로 할부 확인으로 진행
LPMT->NextSubJob = KICC_InstallmentCConfrim;

// SecretNo 초기화 (빈 값으로 설정)
memset(m_CardInfo.SecretNo, 0, sizeof(m_CardInfo.SecretNo));
```

#### 수정 2: KICC_CardInput 함수 - DB 카드 모드 (라인 ~1940)

**현재 코드**:
```cpp
// 라인 1940 근처 (DB 카드 모드 분기)
LPMT->NextSubJob = KICC_JuminNo;
```

**수정 후**:
```cpp
// 주민번호 입력 생략 - 바로 카드 비밀번호 입력으로 진행
LPMT->NextSubJob = KICC_CardPw;

// SecretNo 초기화 (빈 값으로 설정)
memset(m_CardInfo.SecretNo, 0, sizeof(m_CardInfo.SecretNo));
```

### 3.2 KICCpayMent.cpp 수정 (필요시)

`auth_value`가 빈 값일 때의 처리를 확인하고 필요시 수정합니다.

**현재 코드** (라인 612-618):
```cpp
if (strlen(pScenario->m_CardInfo.SecretNo) >= 10)
    user_type = "02";  // 사업자
else
    user_type = "01";  // 개인
```

**수정 후** (주민번호 없을 때 기본값 처리):
```cpp
if (strlen(pScenario->m_CardInfo.SecretNo) >= 10)
    user_type = "02";  // 사업자
else if (strlen(pScenario->m_CardInfo.SecretNo) == 6)
    user_type = "01";  // 개인 (생년월일)
else
    user_type = "01";  // 주민번호 없음 - 개인으로 기본 설정
```

**auth_value 처리** (라인 647 근처):
- 빈 값 전송이 KICC API에서 허용되는지 확인 필요
- 필요시 빈 문자열("")로 명시적 설정

---

## 4. 수정 흐름도

```
[수정 후 흐름]

일반 모드:
KICC_CardInput → KICC_EffecDate → KICC_InstallmentCConfrim → KICC_CardPw → KICC_ApproveReq → ...
                                  (주민번호 생략)

DB 카드 모드:
KICC_CardInput → KICC_CardPw → KICC_InstallmentCConfrim → KICC_ApproveReq → ...
                (주민번호 생략)
```

---

## 5. 구현 체크리스트

### 5.1 코드 수정

- [ ] `KICC_EffecDate` 함수에서 `NextSubJob`을 `KICC_InstallmentCConfrim`으로 변경
- [ ] `KICC_CardInput` 함수(DB 모드)에서 `NextSubJob`을 `KICC_CardPw`로 변경
- [ ] 각 수정 지점에서 `SecretNo` 초기화 추가
- [ ] `KICCpayMent.cpp`에서 빈 `auth_value` 처리 확인/수정

### 5.2 검증

- [ ] KICC API 문서에서 `auth_value` 필수 여부 확인
- [ ] 테스트 환경에서 주민번호 없이 결제 승인 테스트
- [ ] 에러 응답 시 적절한 오류 메시지 처리 확인

### 5.3 빌드 및 배포

- [ ] Debug 구성으로 빌드 및 테스트
- [ ] Release 구성으로 빌드
- [ ] 운영 환경 배포 전 QA 테스트

---

## 6. 주의사항

### 6.1 KICC API 호환성

- **확인 필요**: KICC EasyPay API에서 `auth_value`(주민번호/사업자번호) 없이 결제 승인이 가능한지 사전 확인 필요
- 카드사별로 인증 정책이 다를 수 있음
- 일부 카드사는 본인인증을 위해 주민번호를 필수로 요구할 수 있음

### 6.2 롤백 계획

수정 전 상태로 쉽게 롤백할 수 있도록:
1. 현재 코드를 백업
2. 조건부 컴파일 또는 설정 플래그를 통한 on/off 기능 고려

```cpp
// 예시: 조건부 처리
#define SKIP_JUMIN_INPUT  1  // 주민번호 입력 생략 여부

#if SKIP_JUMIN_INPUT
    LPMT->NextSubJob = KICC_InstallmentCConfrim;
#else
    LPMT->NextSubJob = KICC_JuminNo;
#endif
```

### 6.3 로깅

- 주민번호 생략 모드로 동작할 때 로그 기록
- KICC API 응답에서 인증 관련 오류 발생 시 상세 로깅

---

## 7. 일정 (예상)

| 단계 | 작업 내용 |
|------|----------|
| 1단계 | KICC API 문서 확인 및 기술 검토 |
| 2단계 | 코드 수정 (KICC_Scenario_Travelport.cpp) |
| 3단계 | 코드 수정 (KICCpayMent.cpp - 필요시) |
| 4단계 | 빌드 및 단위 테스트 |
| 5단계 | 통합 테스트 (테스트 환경) |
| 6단계 | QA 검증 |
| 7단계 | 운영 배포 |

---

## 8. 참고 자료

- **KICC_Scenario_Travelport.cpp** - 메인 시나리오 로직
  - `KICC_JuminNo` 함수: 라인 1421-1613
  - `KICC_EffecDate` 함수: 라인 ~1700
  - `KICC_CardInput` 함수: 라인 ~1900
- **KICCpayMent.cpp** - KICC API 연동
  - `auth_value` 설정: 라인 647
  - `user_type` 결정: 라인 612-618
- **KICC_Common.h** - 데이터 구조체 정의
