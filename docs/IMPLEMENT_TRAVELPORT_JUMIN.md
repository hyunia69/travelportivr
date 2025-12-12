# IMPLEMENT_TRAVELPORT_JUMIN.md

## Travelport 시나리오 주민번호(생년월일) 입력 생략 구현 완료

**구현일**: 2025-12-05
**대상 프로젝트**: KICC_Scenario_Travelport
**목적**: 주민번호(생년월일) 입력 단계를 생략하도록 시나리오 수정

---

## 1. 구현 개요

KICC Travelport 결제 시나리오에서 주민번호(생년월일) 입력 단계를 생략하도록 코드를 수정하였습니다. 또한 할부개월 처리 로직을 개선하여 DB에서 읽어온 값을 우선 사용하고, 할부개월이 비어있거나 잘못된 경우에도 기본값으로 처리하여 결제가 정상적으로 진행되도록 개선하였습니다.

### 수정된 파일

| 파일 | 수정 내용 |
|------|----------|
| `KICC_Scenario_Travelport.cpp` | 흐름 변경: `KICC_JuminNo` 건너뛰기, DB 할부개월 설정 추가 |
| `KICCpayMent.cpp` | `auth_value` 빈 값 처리 로깅 추가, 할부개월 DB 값 우선 사용 로직 추가 |

---

## 2. 상세 수정 내역

### 2.1 KICC_Scenario_Travelport.cpp 수정

#### 수정 1: KICC_EffecDate 함수 (라인 1727-1736)

**변경 전**:
```cpp
if (c == '1') //예
{
    info_printf(localCh, "KICC_EffecDate[%d] 유효 기간 입력 후>확인 후>맞습니다.", state);
    eprintf("KICC_EffecDate[%d] 유효 기간 입력 후>확인 후>맞습니다.", state);

    return KICC_JuminNo(0);
}
```

**변경 후**:
```cpp
if (c == '1') //예
{
    info_printf(localCh, "KICC_EffecDate[%d] 유효 기간 입력 부>확인 부>맞습니다.", state);
    eprintf("KICC_EffecDate[%d] 유효 기간 입력 부>확인 부>맞습니다.", state);

    // [MODIFIED] 주민번호 입력 생략 - 바로 할부 확인으로 진행
    // SecretNo 초기화 (빈 값으로 설정)
    memset(pScenario->m_CardInfo.SecretNo, 0x00, sizeof(pScenario->m_CardInfo.SecretNo));
    
    // [MODIFIED] DB에서 할부개월 읽어온 경우 설정
    if (strlen(pScenario->m_szDB_InstallPeriod) > 0) {
        int nInstall = atoi(pScenario->m_szDB_InstallPeriod);
        if (nInstall >= 0 && nInstall <= 12) {
            memset(pScenario->m_CardInfo.InstPeriod, 0x00, sizeof(pScenario->m_CardInfo.InstPeriod));
            sprintf(pScenario->m_CardInfo.InstPeriod, "%02d", nInstall);
            sprintf(pScenario->m_szInstallment, "%02d", nInstall);
            info_printf(localCh, "[KICC] DB 할부개월 적용: %s개월", pScenario->m_CardInfo.InstPeriod);
        }
    }
    
    info_printf(localCh, "[KICC] 주민번호 입력 생략 - 할부 확인으로 이동");
    return KICC_InstallmentCConfrim(0);
}
```

**변경 사항**:
- `KICC_JuminNo(0)` → `KICC_InstallmentCConfrim(0)` 호출 변경
- `SecretNo` 필드 초기화 추가
- DB에서 할부개월을 읽어온 경우 `InstPeriod`에 설정하는 로직 추가
- 디버그 로그 추가

---

#### 수정 2: KICC_CardInput 함수 - DB 카드 모드 (라인 1931-1948)

**변경 전**:
```cpp
// [MODIFIED] DB 모드 시 유효기간 건너뛰고 주민번호로 이동
if (pScenario->m_bUseDbCardInfo) {
    // [NEW] DB에서 유효기간 가져오기
    strncpy(pScenario->m_CardInfo.ExpireDt,
        pScenario->m_szDB_ExpireDate,
        sizeof(pScenario->m_CardInfo.ExpireDt) - 1);

    info_printf(localCh, "[KICC] DB 유효기간 적용: %s", pScenario->m_CardInfo.ExpireDt);
    info_printf(localCh, "[KICC] 유효기간 입력 건너뛰기 (DB 모드)");

    // 유효기간 입력 단계 건너뛰고 주민번호 입력으로 이동
    return KICC_JuminNo(0);
}
```

**변경 후**:
```cpp
// [MODIFIED] DB 모드 시 유효기간 건너뛰고 카드 비밀번호로 이동
if (pScenario->m_bUseDbCardInfo) {
    // [NEW] DB에서 유효기간 가져오기
    strncpy(pScenario->m_CardInfo.ExpireDt,
        pScenario->m_szDB_ExpireDate,
        sizeof(pScenario->m_CardInfo.ExpireDt) - 1);

    info_printf(localCh, "[KICC] DB 유효기간 적용: %s", pScenario->m_CardInfo.ExpireDt);
    info_printf(localCh, "[KICC] 유효기간 입력 건너뛰기 (DB 모드)");

    // [MODIFIED] 주민번호 입력 생략 - 바로 카드 비밀번호 입력으로 진행
    // SecretNo 초기화 (빈 값으로 설정)
    memset(pScenario->m_CardInfo.SecretNo, 0x00, sizeof(pScenario->m_CardInfo.SecretNo));
    info_printf(localCh, "[KICC] 주민번호 입력 생략 - 카드 비밀번호 입력으로 이동");
    return KICC_CardPw(0);
}
```

**변경 사항**:
- `KICC_JuminNo(0)` → `KICC_CardPw(0)` 호출 변경
- `SecretNo` 필드 초기화 추가
- 디버그 로그 추가

---

### 2.2 KICCpayMent.cpp 수정

#### 수정 1: 주민번호/사업자번호 처리 개선 (라인 612-626)

**변경 전**:
```cpp
if (strlen(pScenario->m_CardInfo.SecretNo) >= 10)
{// 사업자 번호이면
    memset(szUserType, 0x00, sizeof(szUserType));
    strncpy(szUserType, "1", sizeof(szUserType) - 1);
}
```

**변경 후**:
```cpp
// [MODIFIED] 주민번호/사업자번호 처리 개선
if (strlen(pScenario->m_CardInfo.SecretNo) >= 10)
{// 사업자 번호이면 (10자리 이상)
    memset(szUserType, 0x00, sizeof(szUserType));
    strncpy(szUserType, "1", sizeof(szUserType) - 1);
    xprintf("[CH:%03d] user_type=1 (사업자번호: %d자리)", ch, strlen(pScenario->m_CardInfo.SecretNo));
}
else if (strlen(pScenario->m_CardInfo.SecretNo) == 6)
{// 개인 (생년월일 6자리)
    xprintf("[CH:%03d] user_type=0 (개인/생년월일: 6자리)", ch);
}
else
{// 주민번호 없음 - 개인으로 기본 설정
    xprintf("[CH:%03d] user_type=0 (주민번호 생략 - 개인 기본값)", ch);
}
```

**변경 사항**:
- 주민번호 없는 경우를 명시적으로 처리
- 각 경우에 대해 로그 출력 추가
- `user_type`은 기본값 "0"(개인)으로 유지

#### 수정 2: 할부개월 처리 개선 (라인 572-599)

**변경 전**:
```cpp
// 할부개월 범위 검증
int nInstallPeriod = atoi(pScenario->m_CardInfo.InstPeriod);
if (nInstallPeriod < 0 || nInstallPeriod > 12) {
    eprintf("[KICC] 할부개월 범위 오류: %s (범위: 00~12)",
        pScenario->m_CardInfo.InstPeriod);
    // 오류 처리 및 종료
    return 0;
}
```

**변경 후**:
```cpp
// [MODIFIED] 할부개월 검증 및 기본값 설정
// 할부개월이 비어있거나 잘못된 경우 DB에서 읽어온 값 또는 기본값 "00"(일시불)로 설정
if (strlen(pScenario->m_CardInfo.InstPeriod) == 0) {
    // DB에서 할부개월을 읽어온 경우 우선 사용
    if (strlen(pScenario->m_szDB_InstallPeriod) > 0) {
        int nInstall = atoi(pScenario->m_szDB_InstallPeriod);
        if (nInstall >= 0 && nInstall <= 12) {
            memset(pScenario->m_CardInfo.InstPeriod, 0x00, sizeof(pScenario->m_CardInfo.InstPeriod));
            sprintf(pScenario->m_CardInfo.InstPeriod, "%02d", nInstall);
            xprintf("[CH:%03d] [KICC] 할부개월이 비어있어 DB 값으로 설정: %s", ch, pScenario->m_CardInfo.InstPeriod);
            info_printf(ch, "[KICC] 할부개월이 비어있어 DB 값으로 설정: %s", pScenario->m_CardInfo.InstPeriod);
        }
        else {
            // DB 값이 범위를 벗어난 경우 기본값 사용
            memset(pScenario->m_CardInfo.InstPeriod, 0x00, sizeof(pScenario->m_CardInfo.InstPeriod));
            strncpy(pScenario->m_CardInfo.InstPeriod, "00", sizeof(pScenario->m_CardInfo.InstPeriod) - 1);
            xprintf("[CH:%03d] [KICC] DB 할부개월 범위 오류, 기본값(일시불)으로 설정: %s", ch, pScenario->m_CardInfo.InstPeriod);
            info_printf(ch, "[KICC] DB 할부개월 범위 오류, 기본값(일시불)으로 설정: %s", pScenario->m_CardInfo.InstPeriod);
        }
    }
    else {
        // DB 값도 없는 경우 기본값 사용
        memset(pScenario->m_CardInfo.InstPeriod, 0x00, sizeof(pScenario->m_CardInfo.InstPeriod));
        strncpy(pScenario->m_CardInfo.InstPeriod, "00", sizeof(pScenario->m_CardInfo.InstPeriod) - 1);
        xprintf("[CH:%03d] [KICC] 할부개월이 비어있어 기본값(일시불)으로 설정: %s", ch, pScenario->m_CardInfo.InstPeriod);
        info_printf(ch, "[KICC] 할부개월이 비어있어 기본값(일시불)으로 설정: %s", pScenario->m_CardInfo.InstPeriod);
    }
}

// 할부개월 범위 검증
int nInstallPeriod = atoi(pScenario->m_CardInfo.InstPeriod);
if (nInstallPeriod < 0 || nInstallPeriod > 12) {
    // 범위를 벗어난 경우 기본값(일시불)으로 설정
    memset(pScenario->m_CardInfo.InstPeriod, 0x00, sizeof(pScenario->m_CardInfo.InstPeriod));
    strncpy(pScenario->m_CardInfo.InstPeriod, "00", sizeof(pScenario->m_CardInfo.InstPeriod) - 1);
    xprintf("[CH:%03d] [KICC] 할부개월 범위 오류로 기본값(일시불)으로 설정: 원래값 범위 초과 (범위: 00~12)", ch);
    info_printf(ch, "[KICC] 할부개월 범위 오류로 기본값(일시불)으로 설정: 원래값 범위 초과 (범위: 00~12)");
}
```

**변경 사항**:
- 할부개월이 비어있을 때 DB에서 읽어온 값(`m_szDB_InstallPeriod`)을 우선 사용
- DB 값이 없거나 범위를 벗어난 경우 기본값 "00"(일시불)로 설정
- 범위 오류 시 오류 종료 대신 기본값으로 설정하여 결제 진행 가능하도록 개선
- 각 경우에 대해 상세한 로그 출력 추가

---

## 3. 할부개월 처리 개선

### 3.1 문제점

주민번호 입력을 생략하면서 할부개월이 설정되지 않아 "할부개월 없음" 오류가 발생하는 문제가 있었습니다.

### 3.2 해결 방안

1. **DB에서 할부개월 읽기**: `ADODB.cpp`에서 `RESERVED_5` 필드를 `m_szDB_InstallPeriod`에 저장
2. **일반 모드에서 DB 할부개월 설정**: `KICC_EffecDate`에서 `KICC_InstallmentCConfrim`으로 이동 시 DB 값이 있으면 설정
3. **결제 요청 시 최종 검증**: `KICCpayMent.cpp`에서 할부개월이 비어있을 때 DB 값 우선 확인 후 기본값 사용

### 3.3 할부개월 설정 우선순위

1. 사용자 입력 값 (할부개월 입력 단계에서 입력한 값)
2. DB에서 읽어온 값 (`m_szDB_InstallPeriod`)
3. 기본값 "00" (일시불)

---

## 4. 수정 후 흐름도

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

## 5. KICC API 영향

### auth_value 필드 처리
- `auth_value` 파라미터는 빈 값("")으로 전송됨
- 기존 코드에서 이미 빈 값 전송을 지원함 (라인 657)

### user_type 필드 처리
- `user_type`은 기본값 "0"(개인)으로 설정됨
- 사업자번호(10자리)가 입력된 경우에만 "1"로 변경

### install_period 필드 처리
- 할부개월은 DB에서 읽어온 값(`RESERVED_5`)을 우선 사용
- DB 값이 없거나 범위를 벗어난 경우 기본값 "00"(일시불)로 설정
- 할부개월이 비어있어도 기본값으로 처리하여 "할부개월 없음" 오류 방지

---

## 6. 확인 사항

### 5.1 코드 수정 완료

- [x] `KICC_EffecDate` 함수에서 `NextSubJob`을 `KICC_InstallmentCConfrim`으로 변경
- [x] `KICC_CardInput` 함수(DB 모드)에서 `NextSubJob`을 `KICC_CardPw`로 변경
- [x] 각 수정 지점에서 `SecretNo` 초기화 추가
- [x] `KICCpayMent.cpp`에서 빈 `auth_value` 처리 로깅 추가
- [x] `KICC_EffecDate` 함수에서 DB 할부개월 설정 로직 추가
- [x] `KICCpayMent.cpp`에서 할부개월 빈 값 처리 및 DB 값 우선 사용 로직 추가

### 5.2 검증 필요

- [ ] KICC API 테스트 환경에서 주민번호 없이 결제 승인 테스트
- [ ] 에러 응답 시 적절한 오류 메시지 처리 확인
- [ ] Debug 구성으로 빌드 및 테스트
- [ ] Release 구성으로 빌드

---

## 7. 롤백 방법

수정 전 상태로 롤백하려면:

### KICC_Scenario_Travelport.cpp

1. 라인 1732-1748의 `KICC_InstallmentCConfrim(0)` 호출을 `KICC_JuminNo(0)`로 복원
2. 라인 1943-1947의 `KICC_CardPw(0)` 호출을 `KICC_JuminNo(0)`로 복원
3. 추가된 `memset`, `info_printf` 및 DB 할부개월 설정 로직 제거

### KICCpayMent.cpp

1. 라인 612-626의 조건문을 원래의 간단한 형태로 복원
2. 라인 572-599의 할부개월 처리 로직을 원래의 범위 검증만 하는 형태로 복원

---

## 8. 참고 파일

| 파일 | 위치 | 설명 |
|------|------|------|
| `KICC_Scenario_Travelport.cpp` | `KICC_Scenario_Travelport/` | 메인 시나리오 로직 |
| `KICCpayMent.cpp` | `KICC_Scenario_Travelport/` | KICC API 연동 |
| `KICC_Common.h` | `KICC_Scenario_Travelport/` | CARDINFO 구조체 정의 |
| `PLAN_TRAVELPORT_JUMIN.md` | 프로젝트 루트 | 원본 계획 문서 |

---

## 9. 변경 이력

| 날짜 | 버전 | 변경 내용 | 작성자 |
|------|------|----------|--------|
| 2025-12-05 | 1.0 | 최초 구현 - 주민번호 입력 생략 | Claude |
| 2025-12-05 | 1.1 | 할부개월 처리 개선 - DB 값 우선 사용, 빈 값 처리 추가 | Claude |
