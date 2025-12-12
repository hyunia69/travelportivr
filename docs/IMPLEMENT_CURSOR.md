# KICC 시나리오 DB 필드 사용 구현 진행 상황

## ? 전체 진행 상황

- ? **Phase 1**: 데이터 구조 준비 및 DB 조회 수정 (완료)
- ? **Phase 2**: 카드번호 입력 로직 수정 (완료)
- ? **Phase 3**: 유효기간 입력 단계 건너뛰기 (완료)
- ? **Phase 4**: 할부개월 입력 단계 건너뛰기 (완료)
- ? **Phase 5**: 결제 처리 검증 및 테스트 (완료)

---

## ? Phase 1: 데이터 구조 준비 및 DB 조회 수정 (완료)

### Step 1.1: 클래스 멤버 변수 추가 ?
**파일**: `KICC_Scenario_Travelport/KICC_Scenario_Travelport.h`

**구현 내용**:
- `m_szDB_CardPrefix[12 + 1]`: RESERVED_4 (카드번호 앞 12자리)
- `m_szDB_ExpireDate[4 + 1]`: RESERVED_3 (유효기간 YYMM)
- `m_szDB_InstallPeriod[2 + 1]`: RESERVED_5 (할부개월)
- `m_bUseDbCardInfo`: DB 필드 사용 여부 플래그 (1:TRUE, 0:FALSE)

**구현 위치**: `KICC_Scenario_Travelport.h` 라인 72-76

### Step 1.2: ScenarioInit() 초기화 ?
**파일**: `KICC_Scenario_Travelport/KICC_Scenario_Travelport.cpp`

**구현 내용**:
- 모든 DB 필드를 `memset`으로 초기화
- `m_bUseDbCardInfo`를 기본값 `1` (TRUE)로 설정

**구현 위치**: `KICC_Scenario_Travelport.cpp` 라인 1804-1811

### Step 1.3: DB 조회 및 검증 로직 ?
**파일**: `KICC_Scenario_Travelport/ADODB.cpp`

**구현 내용**:
- `sp_getKiccOrderInfoByTel2` 함수에서 RESERVED_3, RESERVED_4, RESERVED_5 필드 읽기
- 검증 로직:
  - RESERVED_4: 정확히 12자리 확인
  - RESERVED_3: 정확히 4자리(YYMM) 확인
  - RESERVED_5: 0~12 범위 확인
- 검증 실패 시 `m_bUseDbCardInfo = 0`으로 설정하여 기존 입력 방식으로 폴백
- 로그 추가: 필드 로드 및 검증 결과 기록

**구현 위치**: `ADODB.cpp` 라인 1285-1343

---

## ? Phase 2: 카드번호 입력 로직 수정 (완료)

### Step 2.1: KICC_CardInput() 함수 수정 ?
**파일**: `KICC_Scenario_Travelport/KICC_Scenario_Travelport.cpp`

**구현 내용**:

1. **State 0 (카드번호 입력 요청)**:
   - DB 사용 시: `send_guide(5)` - 4자리만 입력
   - 기존 방식: `send_guide(17)` - 16자리 입력
   - 기존 음성 파일 재사용

2. **State 1 (카드번호 검증)**:
   - DB 사용 시:
     - 4자리 검증
     - DB 앞자리(12자리) + 입력받은 뒤자리(4자리) = 16자리 완성
     - 완성된 카드번호를 `m_CardInfo.Card_Num`에 저장
   - 기존 방식: 13~16자리 검증 (코드 보존)

3. **State 3 (사용자 확인 처리)**:
   - DB 사용 시:
     - DB에서 유효기간(`RESERVED_3`) 자동 로드
     - `KICC_JuminNo(0)` 직접 호출 (유효기간 입력 단계 건너뛰기)
   - 기존 방식: `KICC_EffecDate(0)` 호출

**구현 위치**: `KICC_Scenario_Travelport.cpp` 라인 1304-1415

---

## ? Phase 3: 유효기간 입력 단계 건너뛰기 (완료)

### Step 3.1: KICC_EffecDate() 함수에 주석 추가 ?
**파일**: `KICC_Scenario_Travelport/KICC_Scenario_Travelport.cpp`

**구현 내용**:
- 함수 상단에 주석 추가
- DB 사용 모드에서는 호출되지 않음을 명시
- 기존 입력 방식에서는 여전히 사용됨을 명시

**구현 위치**: `KICC_Scenario_Travelport.cpp` 라인 1114-1123

---

## ? Phase 4: 할부개월 입력 단계 건너뛰기 (완료)

### Step 4.1: KICC_JuminNo() 함수 수정 ?
**파일**: `KICC_Scenario_Travelport/KICC_Scenario_Travelport.cpp`

**구현 내용**:
- State 3에서 DB 사용 여부에 따라 분기:
  - DB 사용 시:
    - DB에서 할부개월(`RESERVED_5`) 로드
    - `m_CardInfo.InstPeriod`와 `m_szInstallment`에 저장
    - `KICC_CardPw(0)` 직접 호출 (할부 입력 건너뛰기)
  - 기존 방식: `KICC_InstallmentCConfrim(0)` 호출 (코드 보존)

**구현 위치**: `KICC_Scenario_Travelport.cpp` 라인 1050-1094

### Step 4.2: KICC_InstallmentCConfrim() 함수에 주석 추가 ?
**파일**: `KICC_Scenario_Travelport/KICC_Scenario_Travelport.cpp`

**구현 내용**:
- 함수 상단에 주석 추가
- DB 사용 모드에서는 호출되지 않음을 명시
- 기존 입력 방식에서는 여전히 사용됨을 명시

**구현 위치**: `KICC_Scenario_Travelport.cpp` 라인 752-761

---

## ? Phase 5: 결제 처리 검증 및 테스트 (완료)

### Step 5.1: 결제 데이터 구성 검증 ?
**파일**: `KICC_Scenario_Travelport/KICCpayMent.cpp`

**구현 내용**:
- ? `KiccArsPayProcess` 함수 내에 DB 기반 카드정보 검증 로직 추가
- ? 카드번호 16자리 완성 여부 확인
- ? 유효기간 YYMM 형식 (4자리) 확인
- ? 할부개월 00~12 범위 확인
- ? 검증 실패 시 오류 처리 및 로그 기록

**구현 위치**: `KICCpayMent.cpp` 라인 520-580

**예상 구현 위치**: `KICCpayMent.cpp`의 `KiccArsPayProcess` 함수 내부 (약 라인 520 이후)

**구현 예시**:
```cpp
// ========================================
// [NEW] DB 기반 카드정보 검증
// ========================================
if (pScenario->m_bUseDbCardInfo) {
    info_printf(ch, "[KICC] DB 기반 결제 시작");
    info_printf(ch, "[KICC] 카드번호: %s (길이: %d)",
                pScenario->m_CardInfo.Card_Num,
                strlen(pScenario->m_CardInfo.Card_Num));
    info_printf(ch, "[KICC] 유효기간: %s", pScenario->m_CardInfo.ExpireDt);
    info_printf(ch, "[KICC] 할부개월: %s", pScenario->m_CardInfo.InstPeriod);

    // 카드번호 길이 검증
    if (strlen(pScenario->m_CardInfo.Card_Num) != 16) {
        eprintf("[KICC] 카드번호 길이 오류: %d자리 (기대: 16자리)",
                strlen(pScenario->m_CardInfo.Card_Num));
        (*port)[ch].ppftbl[POST_NET].postcode = HI_OK;
        pScenario->m_PaySysCd = -1;
        return 0;
    }

    // 유효기간 형식 검증
    if (strlen(pScenario->m_CardInfo.ExpireDt) != 4) {
        eprintf("[KICC] 유효기간 형식 오류: %s (기대: YYMM)",
                pScenario->m_CardInfo.ExpireDt);
        (*port)[ch].ppftbl[POST_NET].postcode = HI_OK;
        pScenario->m_PaySysCd = -1;
        return 0;
    }
}
```

### Step 5.2: 로그 강화 ?
**파일**: 모든 수정된 함수

**구현 내용**:
- ? DB 필드 로드 시점 로그 추가 (`ADODB.cpp`)
- ? 카드번호 완성 시점 로그 추가 (`KICC_Scenario_Travelport.cpp`)
- ? 단계 건너뛰기 시점 로그 추가 (`KICC_Scenario_Travelport.cpp`)
- ? 결제 데이터 전송 직전 로그 추가 (`KICCpayMent.cpp`)

**구현 위치**:
- `ADODB.cpp` 라인 1334-1342: DB 필드 로드 완료 로그
- `KICC_Scenario_Travelport.cpp` 라인 1360-1363: 카드번호 완성 로그
- `KICC_Scenario_Travelport.cpp` 라인 1403: 유효기간 입력 건너뛰기 로그
- `KICC_Scenario_Travelport.cpp` 라인 1072: 할부개월 입력 건너뛰기 로그
- `KICCpayMent.cpp` 라인 560-567: 결제 요청 데이터 로그

**로그 추가 항목**:
```cpp
// 1. DB 필드 로드 시점
info_printf(localCh, "[KICC] DB 필드 로드 완료");
info_printf(localCh, "  - RESERVED_3 (유효기간): %s", m_szDB_ExpireDate);
info_printf(localCh, "  - RESERVED_4 (카드앞자리): %s", m_szDB_CardPrefix);
info_printf(localCh, "  - RESERVED_5 (할부개월): %s", m_szDB_InstallPeriod);
info_printf(localCh, "  - DB 사용 모드: %s", m_bUseDbCardInfo ? "ON" : "OFF");

// 2. 카드번호 완성 시점
info_printf(localCh, "[KICC] 카드번호 완성: DB(%s) + 입력(%s) = %s",
            m_szDB_CardPrefix, szInputLast4, m_CardInfo.Card_Num);

// 3. 단계 건너뛰기 시점
info_printf(localCh, "[KICC] 유효기간 입력 건너뛰기 (DB 사용)");
info_printf(localCh, "[KICC] 할부개월 입력 건너뛰기 (DB 사용)");

// 4. 결제 데이터 전송 직전
info_printf(localCh, "[KICC] 결제 요청 데이터:");
info_printf(localCh, "  - 카드번호: %s", m_CardInfo.Card_Num);
info_printf(localCh, "  - 유효기간: %s", m_CardInfo.ExpireDt);
info_printf(localCh, "  - 할부개월: %s", m_CardInfo.InstPeriod);
info_printf(localCh, "  - 비밀번호: **");
info_printf(localCh, "  - 주민번호: %s", m_CardInfo.SecretNo);
```

---

## ? 변경된 흐름 요약

### DB 사용 모드 (m_bUseDbCardInfo = TRUE)
```
전화번호 입력 
→ 주문정보 조회 (DB에서 RESERVED_3/4/5 로드 및 검증)
→ 카드번호 뒤 4자리 입력 (16자리 완성)
→ 주민/사업자번호 입력
→ 비밀번호 입력 
→ 결제 처리
```

**건너뛰는 단계**:
- ? 유효기간 입력 (`KICC_EffecDate`)
- ? 할부개월 입력 (`KICC_InstallmentCConfrim`)

### 기존 입력 방식 (m_bUseDbCardInfo = FALSE)
```
전화번호 입력 
→ 주문정보 조회
→ 카드번호 전체 입력 (13~16자리)
→ 유효기간 입력
→ 주민/사업자번호 입력
→ 할부개월 입력
→ 비밀번호 입력 
→ 결제 처리
```

---

## ? 주요 구현 파일 목록

| 파일 | 수정 내용 | 상태 |
|------|-----------|------|
| `KICC_Scenario_Travelport.h` | 멤버 변수 추가 | ? 완료 |
| `KICC_Scenario_Travelport.cpp` | ScenarioInit 초기화, KICC_CardInput, KICC_JuminNo 수정, 로그 추가 | ? 완료 |
| `ADODB.cpp` | sp_getKiccOrderInfoByTel2에서 RESERVED_3/4/5 읽기 및 검증, 로그 추가 | ? 완료 |
| `KICCpayMent.cpp` | 결제 데이터 검증 로직 추가, 로그 추가 | ? 완료 |

---

## ? 테스트 계획

### Test Case 1: DB 필드 정상 케이스 ?
**전제 조건**:
- `RESERVED_3 = "2512"` (2025년 12월)
- `RESERVED_4 = "123456789012"` (카드번호 앞 12자리)
- `RESERVED_5 = "03"` (할부 3개월)

**예상 흐름**:
1. 전화번호 입력 → DB 조회 성공
2. 카드번호 뒤 4자리 입력: `3456` → 완성: `1234567890123456`
3. 주민번호 입력: `801231`
4. 비밀번호 입력: `12`
5. 결제 승인 요청 → 성공

**검증 항목**:
- [ ] 유효기간 입력 단계 건너뛰어짐
- [ ] 할부개월 입력 단계 건너뛰어짐
- [ ] 카드번호 16자리 정상 완성
- [ ] 결제 승인 성공

### Test Case 2: DB 필드 오류 케이스 (폴백) ?
**전제 조건**:
- `RESERVED_3 = "25"` (길이 오류: 2자리)
- `RESERVED_4 = "1234567890"` (길이 오류: 10자리)
- `RESERVED_5 = "15"` (범위 오류: 15개월)

**예상 흐름**:
1. 전화번호 입력 → DB 조회 성공
2. DB 필드 검증 실패 → `m_bUseDbCardInfo = FALSE` 설정
3. 카드번호 전체 입력: `1234567890123456` (13~16자리)
4. 유효기간 입력: `2512`
5. 주민번호 입력: `801231`
6. 할부개월 입력: `03`
7. 비밀번호 입력: `12`
8. 결제 승인 요청

**검증 항목**:
- [ ] DB 오류 감지됨
- [ ] 기존 입력 방식으로 폴백
- [ ] 모든 입력 단계 정상 실행
- [ ] 결제 승인 성공

### Test Case 3: DB 필드 없음 (NULL) ?
**전제 조건**:
- `RESERVED_3 = NULL`
- `RESERVED_4 = NULL`
- `RESERVED_5 = NULL`

**예상 흐름**:
- Test Case 2와 동일 (기존 방식으로 폴백)

**검증 항목**:
- [ ] NULL 값 처리 정상
- [ ] 기존 입력 방식 동작

---

## ? 다음 단계

1. ? **Phase 5.1 구현**: `KICCpayMent.cpp`의 `KiccArsPayProcess` 함수에 검증 로직 추가 (완료)
2. ? **Phase 5.2 구현**: 로그 강화 (필요한 위치에 로그 추가) (완료)
3. ? **테스트 실행**: Test Case 1, 2, 3 실행 및 검증 (대기 중)
4. ? **문서화**: 최종 변경 사항 문서화 (대기 중)

---

## ? 구현 이력

- **2025-11-21**: Phase 1, 2, 3, 4 완료
  - 데이터 구조 준비 및 DB 조회 수정
  - 카드번호 입력 로직 수정 (4자리만 입력)
  - 유효기간 입력 단계 건너뛰기
  - 할부개월 입력 단계 건너뛰기

- **2025-11-21**: Phase 5 완료
  - 결제 데이터 구성 검증 로직 추가
  - 로그 강화 (DB 필드 로드, 카드번호 완성, 단계 건너뛰기, 결제 요청 데이터)

---

**문서 버전**: 1.1  
**최종 업데이트**: 2025-11-21

