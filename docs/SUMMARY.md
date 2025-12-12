# KICC 다중 주문 결제 처리 기능 구현 요약

## 작업 개요
기존 KICC 결제 시스템에 다중 주문 결제 처리 기능을 추가했습니다. 하나의 전화번호와 인증번호로 여러 주문을 조회하고, 단일 카드 입력으로 모든 주문을 순차적으로 처리할 수 있도록 구현했습니다.

## 구현 일자
2025년 11월 22일

## 주요 변경 사항

### 1. 데이터 구조 추가 (`KICC_Common.h`)

#### `MultiOrderInfo` 구조체 추가
```cpp
#define MAX_ORDERS 10  // PHONE_NO + AUTH_NO 조합당 최대 주문 개수

typedef struct MultiOrderInfo
{
    int nOrderCount;                    // 조회된 주문 개수
    Card_ResInfo orders[MAX_ORDERS];    // 주문 정보 배열
    int nTotalAmount;                   // 모든 주문의 금액 합계
    int nProcessedCount;                // 성공적으로 처리된 주문 개수
    int nFailedCount;                   // 실패한 주문 개수
    char szFailedOrders[512];           // 실패한 주문번호 목록 (쉼표 구분)
} MultiOrderInfo;
```

### 2. 클래스 멤버 변수 추가 (`KICC_Scenario_Travelport.h`)

```cpp
// [다중 주문 지원 - 신규]
MultiOrderInfo   m_MultiOrders;        // 다중 주문 컨테이너
int              m_nCurrentOrderIdx;   // 현재 처리중인 주문 인덱스
BOOL             m_bMultiOrderMode;   // 플래그: 다중 주문 처리 모드
char             m_szAuthNo[12 + 1];  // SP에서 추출된 AUTH_NO 저장용
```

### 3. 데이터베이스 함수 추가 (`ADODB.h`, `ADODB.cpp`)

#### `sp_getKiccMultiOrderInfo` 함수
- **목적**: 전화번호로 다중 주문 조회
- **동작**:
  1. `PHONE_NO`로 가장 최근 주문 조회
  2. 해당 주문의 `AUTH_NO` 자동 추출
  3. `PHONE_NO` + `AUTH_NO`로 모든 매칭 주문 조회
- **반환**: `MultiOrderInfo` 구조체에 주문 정보 배열 저장

#### `FetchMultiOrderResults` 함수
- **목적**: 저장 프로시저 결과를 파싱하여 `MultiOrderInfo`에 저장
- **특징**:
  - 첫 번째 주문에서 DB 카드 정보(`RESERVED_3`, `RESERVED_4`, `RESERVED_5`) 로드
  - 각 주문의 상세 정보 로그 출력

#### `getMultiOrderInfoProc` 스레드 함수
- **목적**: 비동기 DB 조회 (IVR 채널 블로킹 방지)
- **동작**: 별도 스레드에서 DB 접근 후 결과를 `m_DBAccess`에 설정

### 4. 시나리오 상태 함수 추가 (`KICC_Scenario_Travelport.cpp`)

#### `KICC_getMultiOrderInfo(int state)`
- **목적**: 다중 주문 조회 시작
- **동작**:
  1. `getMultiOrderInfo_host(90)` 호출하여 비동기 DB 조회 시작
  2. 조회 결과 대기
  3. 주문이 없으면 종료, 있으면 `KICC_AnnounceMultiOrders`로 이동

#### `KICC_AnnounceMultiOrders(int state)`
- **목적**: 다중 주문 안내 및 확인
- **동작**:
  1. TTS로 주문 개수와 총 금액 안내
  2. 사용자 확인 대기 (1: 진행, 2: 취소)
  3. 확인 시 `KICC_CardInput(0)`으로 이동

#### `KICC_ProcessMultiPayments(int state)`
- **목적**: 다중 주문 순차 결제 처리
- **상태**:
  - **case 0**: 초기화 (인덱스, 카운터 초기화)
  - **case 1**: 각 주문 처리
    - 현재 주문 정보를 `m_CardResInfo`에 복사
    - 카드 정보 확인 로그 출력
    - `KiccPaymemt_host(90)` 호출하여 결제 요청
  - **case 2**: 결제 결과 확인
    - `REPLY_CODE`가 "0000"이면 성공, 아니면 실패
    - 성공/실패 카운터 업데이트
    - 실패 주문번호 목록에 추가
    - 다음 주문으로 이동 또는 `KICC_MultiPaymentSummary`로 이동

#### `KICC_MultiPaymentSummary(int state)`
- **목적**: 최종 결제 결과 요약 안내
- **동작**:
  1. 성공/실패 건수 확인
  2. TTS로 결과 안내
  3. 실패한 주문번호 목록 안내
  4. `KICC_ExitSvc(0)`로 종료

### 5. 기존 함수 수정

#### `KICC_ArsScenarioStart`
- 다중 주문 모드로 변경: `KICC_getOrderInfo(0)` → `KICC_getMultiOrderInfo(0)`

#### `KICC_CardInput`
- 다중 주문 모드에서 카드번호 입력 완료 후 `KICC_ProcessMultiPayments(0)` 호출

#### `KICC_CardPw`
- 다중 주문 모드에서 비밀번호 입력 완료 후 `KICC_ProcessMultiPayments(0)` 호출

### 6. DB 카드 정보 로드 기능

#### `FetchMultiOrderResults`에서 카드 정보 로드
- 첫 번째 주문의 `RESERVED_3` (유효기간), `RESERVED_4` (카드번호 앞자리), `RESERVED_5` (할부개월) 추출
- `m_szDB_CardPrefix`, `m_szDB_ExpireDate`, `m_szDB_InstallPeriod`에 저장
- 유효성 검증 후 `m_bUseDbCardInfo` 플래그 설정

#### `KICC_CardInput`에서 카드번호 조합
- DB 카드번호 앞자리(12자리) + 사용자 입력(4자리) = 16자리 완성
- 길이 검증 및 로그 출력

### 7. 로그 개선

#### `eprintf` 사용으로 변경
- 모든 디버그 로그를 `info_printf`에서 `eprintf`로 변경
- 로그 파일에 자동 저장되도록 개선

#### 주요 로그 포인트
1. **주문 조회**: 주문번호, 상품명, AUTH_NO, 금액
2. **카드 정보 확인**: 카드번호 길이, 유효기간, 비밀번호
3. **승인 요청**: 주문번호, 금액, 가맹점ID
4. **결제 결과 확인**: 주문번호, m_PayResult, REPLY_CODE, REPLY_MESSAGE
5. **최종 결과**: 총 주문, 성공, 실패, 실패 주문 목록

## 현재 상태 및 문제점

### ? 완료된 기능
1. 다중 주문 조회 기능
2. 다중 주문 안내 및 확인
3. 순차 결제 처리 루프
4. DB 카드 정보 로드 및 조합
5. 결제 결과 확인 및 집계
6. 최종 결과 요약 안내

### ?? 현재 문제점

#### 가맹점 ID 포맷 오류 (REPLY_CODE: E106)
- **증상**: 결제 요청 시 "가맹점 ID FORMAT 오류" 발생
- **로그 확인**:
  ```
  [승인요청] 주문번호:ORD202511210003, 금액:1000, 가맹점ID:T0021720
  [결제결과확인] REPLY_CODE:E106, REPLY_MESSAGE:가맹점 ID FORMAT 오류
  ```
- **원인 분석 필요**:
  1. `TERMINAL_ID` 값이 KICC 결제 게이트웨이에서 요구하는 형식과 일치하지 않을 수 있음
  2. `KICCpayMent.cpp`의 `KiccArsPayProcess` 함수에서 `TERMINAL_ID` 전송 방식 확인 필요
  3. DB에서 조회한 `TERMINAL_ID` 형식이 올바른지 확인 필요

### ? 다음 작업 필요 사항

1. **가맹점 ID 포맷 오류 해결**
   - `TERMINAL_ID` 형식 검증 로직 추가
   - KICC 결제 게이트웨이 요구사항 확인
   - DB의 `TERMINAL_ID` 값 검증

2. **결제 결과 처리 개선**
   - `m_PayResult` 값이 93으로 설정되는 원인 확인
   - `REPLY_CODE` 기반 성공/실패 판단 로직 검증

3. **에러 처리 강화**
   - 결제 실패 시 재시도 로직 검토
   - 부분 실패 시 사용자 안내 개선

4. **테스트**
   - 정상 케이스: 3건 모두 성공
   - 부분 실패 케이스: 일부 주문 실패
   - 전체 실패 케이스: 모든 주문 실패

## 파일 변경 목록

### 수정된 파일
1. `KICC_Common.h` - `MultiOrderInfo` 구조체 추가
2. `KICC_Scenario_Travelport.h` - 다중 주문 관련 멤버 변수 추가
3. `KICC_Scenario_Travelport.cpp` - 다중 주문 처리 로직 추가
4. `ADODB.h` - `sp_getKiccMultiOrderInfo`, `FetchMultiOrderResults` 선언
5. `ADODB.cpp` - 다중 주문 조회 및 카드 정보 로드 구현

### 참고 파일
- `PLAN_MULTI_KR.md` - 원본 설계 문서
- `KICCpayMent.cpp` - 결제 처리 로직 (가맹점 ID 전송 부분 확인 필요)

## 주요 함수 호출 흐름

```
KICC_ArsScenarioStart
  └─> KICC_getMultiOrderInfo
        └─> getMultiOrderInfo_host (비동기)
              └─> sp_getKiccMultiOrderInfo
                    └─> FetchMultiOrderResults
                          └─> KICC_AnnounceMultiOrders
                                └─> KICC_CardInput
                                      └─> KICC_CardPw
                                            └─> KICC_ProcessMultiPayments
                                                  ├─> case 1: KiccPaymemt_host
                                                  └─> case 2: 결과 확인 후 다음 주문 또는
                                                        └─> KICC_MultiPaymentSummary
                                                              └─> KICC_ExitSvc
```

## 데이터베이스 요구사항

### 저장 프로시저 필요
- `dbo.sp_getKiccMultiOrderInfo`
  - **입력**: `@PHONE_NO`, `@DNIS`
  - **동작**:
    1. `PHONE_NO`로 가장 최근 주문 조회하여 `AUTH_NO` 추출
    2. `PHONE_NO` + `AUTH_NO`로 모든 매칭 주문 조회
  - **출력**: 주문 정보 레코드셋 (기존 `sp_getKiccOrderInfoByTel2`와 동일한 필드)

### 필수 필드
- `RESERVED_3`: 유효기간 (YYMM, 4자리)
- `RESERVED_4`: 카드번호 앞자리 (12자리)
- `RESERVED_5`: 할부개월 (2자리)

## 로그 예시

### 정상 흐름
```
[주문조회] 주문번호:ORD202511210003, 상품명:상품1, AUTH_NO:123456, 금액:1000
[주문조회] 주문번호:ORD202511210002, 상품명:상품2, AUTH_NO:123456, 금액:2000
[주문조회] 주문번호:ORD202511210001, 상품명:상품3, AUTH_NO:123456, 금액:3000
[다중주문 DB 카드정보 로드: 카드앞자리:123456789012, 유효기간:1226, 할부:00]
주문 처리중 [1/3]: ORD202511210003, 금액: ?1000
[카드정보확인] 카드번호길이:16, 카드번호:1234567890129999, 유효기간:1226, 비밀번호:08
[승인요청] 주문번호:ORD202511210003, 금액:1000, 가맹점ID:T0021720
[결제결과확인] 주문번호:ORD202511210003, m_PayResult:1, REPLY_CODE:0000, REPLY_MESSAGE:정상처리
주문 ORD202511210003 결제 성공 (REPLY_CODE:0000, 승인번호:12345678)
...
[최종결과] 총주문:3, 성공:3, 실패:0, 실패주문:
```

### 오류 케이스
```
[승인요청] 주문번호:ORD202511210003, 금액:1000, 가맹점ID:T0021720
[결제결과확인] 주문번호:ORD202511210003, m_PayResult:93, REPLY_CODE:E106, REPLY_MESSAGE:가맹점 ID FORMAT 오류
주문 ORD202511210003 결제 실패: 가맹점 ID FORMAT 오류 (REPLY_CODE:E106)
```

## 참고사항

1. **다중 주문 모드 플래그**: `m_bMultiOrderMode`가 TRUE일 때만 다중 주문 처리 로직 실행
2. **카드 정보 공유**: 첫 번째 입력에서 받은 카드 정보를 모든 주문에 공유
3. **순차 처리**: 주문은 순차적으로 처리되며, 한 주문이 실패해도 다음 주문 계속 처리
4. **비동기 처리**: DB 조회는 별도 스레드에서 수행하여 IVR 채널 블로킹 방지

## 다음 작업 우선순위

1. **긴급**: 가맹점 ID 포맷 오류 해결 (REPLY_CODE: E106)
2. **중요**: 결제 결과 처리 로직 검증 (`m_PayResult` 값 확인)
3. **개선**: 에러 처리 및 사용자 안내 개선
4. **테스트**: 다양한 시나리오 테스트

