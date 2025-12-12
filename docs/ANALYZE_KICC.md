# KICC Travelport ARS 시나리오 흐름 분석

## 개요

이 문서는 KICC_Scenario_Travelport 플러그인의 ARS(Automatic Response System) 시나리오 흐름을 상세히 분석합니다.
이 플러그인은 KICC 결제 게이트웨이를 통한 신용카드 결제 처리를 위한 IVR(Interactive Voice Response) 시스템입니다.

## 아키텍처 개요

### 플러그인 구조
- **DLL 형태**: 동적 로드 가능한 시나리오 플러그인
- **인터페이스**: `IScenario` 기본 클래스 상속
- **주요 클래스**: `CKICC_Scenario`
- **결제 처리**: KICC ep_cli_dll.dll 라이브러리 사용

### 핵심 컴포넌트

```
CKICC_Scenario (메인 시나리오 클래스)
├── ScenarioInit()      - 초기화
├── jobArs()            - ARS 진입점
├── KICC_ArsScenarioStart() - ARS 시나리오 시작
├── KICC_SMSScenarioStart() - SMS 시나리오 시작
└── 결제 처리 함수들
    ├── KICC_CardInput()          - 카드번호 입력
    ├── KICC_EffecDate()          - 유효기간 입력
    ├── KICC_JuminNo()            - 주민/사업자번호 입력
    ├── KICC_InstallmentCConfrim()- 할부개월 확인
    ├── KICC_CardPw()             - 비밀번호 입력
    └── KICC_payARS()             - 결제 처리
```

## ARS 흐름 상세 분석

### 1. 시스템 초기화 (DLL 로딩 시)

#### DllMain (DLL_PROCESS_ATTACH)
```cpp
// DllMain:1853-1869
Kicc_Install()  // KICC 결제 DLL 초기화
├── LoadLibrary("ep_cli_dll.dll")
└── GetProcAddress로 함수 포인터 획득
    ├── lfEP_CLI_DLL__init
    ├── lfEP_CLI_DLL__set_plan_data
    ├── lfEP_CLI_DLL__set_entry
    ├── lfEP_CLI_DLL__set_value
    ├── lfEP_CLI_DLL__proc
    └── lfEP_CLI_DLL__get_value
```

#### ScenarioInit (통화별 초기화)
```cpp
// KICC_Scenario_Travelport.cpp:1792-1803
int CKICC_Scenario::ScenarioInit(LPMTP *Port, char *ArsType)
{
    (*lpmt)->pScenario = (void *)this;
    nChan = Port->chanID;
    m_Myport = Port;

    // 채널 정보 저장
    strncpy(szDnis, m_Myport->dnis, ...);      // 착신번호
    strncpy(szAni, m_Myport->ani, ...);        // 발신번호
    strncpy(szArsType, ArsType, ...);          // "ARS" or "SMS"
    strncpy(szSessionKey, m_Myport->Session_Key, ...);

    return 0;
}
```

### 2. ARS 시나리오 진입점

#### jobArs() - 메인 진입점
```cpp
// KICC_Scenario_Travelport.cpp:1808-1825
int CKICC_Scenario::jobArs(int state)
{
    (*lpmt)->pScenario = (void *)this;
    (*lpmt)->Myexit_service = KICC_ExitSvc;    // 종료 핸들러 설정
    (*lpmt)->MyStartState = 10;
    memset(&m_CardInfo, 0x00, sizeof(m_CardInfo));

    // 시나리오 타입별 분기
    if (strcmp(szArsType, "ARS") == 0)
        return KICC_ArsScenarioStart(0);        // ARS 시작
    else if (strcmp(szArsType, "SMS") == 0)
        return KICC_SMSScenarioStart(0);        // SMS 시작

    return KICC_jobArs(0);                      // 기본 흐름
}
```

### 3. ARS 시나리오 상세 흐름

#### 3.1 KICC_ArsScenarioStart - 전화번호 입력 단계

```
[State 0] 인사멘트 재생
KICC_ArsScenarioStart.cpp:1504-1514
┌────────────────────────────────────────┐
│ 음성안내: "안녕하세요..."              │
│ 파일: audio/shop_intro/{DNIS}          │
└────────────────────────────────────────┘
         ↓

[State 1] 전화번호 입력 요청
KICC_ArsScenarioStart.cpp:1516-1523
┌────────────────────────────────────────┐
│ 음성안내: "전화번호를 입력하세요"      │
│ 입력대기: 13자리 DTMF                  │
└────────────────────────────────────────┘
         ↓

[State 2] 전화번호 검증 및 확인
KICC_ArsScenarioStart.cpp:1524-1566
┌────────────────────────────────────────┐
│ 검증규칙:                              │
│  - 길이: 7~12자리 (*#으로 종료 가능)   │
│  - 형식: 010/011/012/016/017/018/019  │
│                                        │
│ TTS 재생: "입력하신 전화번호는..."    │
│ 변수저장: m_szInputTel에 저장          │
└────────────────────────────────────────┘
         ↓

[State 3] TTS 재생 완료 대기
KICC_ArsScenarioStart.cpp:1567-1588
┌────────────────────────────────────────┐
│ TTS 타임아웃 체크 (m_TTSAccess)        │
│ 확인멘트: "맞으면 1번, 아니면 2번"     │
└────────────────────────────────────────┘
         ↓

[State 4] 사용자 확인 처리
KICC_ArsScenarioStart.cpp:1589-1629
┌────────────────────────────────────────┐
│ DTMF='1' (맞습니다)                    │
│   → getOrderInfo_host(90) 호출         │
│   → DB에서 주문정보 조회               │
│   → POST_NET 완료 후 State 0으로       │
│                                        │
│ DTMF='2' (아니오)                      │
│   → KICC_ArsScenarioStart(1) 재시도    │
└────────────────────────────────────────┘
```

#### 3.2 KICC_getOrderInfo - 주문정보 확인 단계

```
[State 0] 주문정보 안내
KICC_getOrderInfo.cpp:1378-1418
┌────────────────────────────────────────┐
│ DB 조회 결과 검증:                     │
│  - m_DBAccess == -1: 시스템 오류       │
│  - m_bDnisInfo == -1: 시스템 오류      │
│  - m_bDnisInfo == 0: 주문없음          │
│                                        │
│ TTS 재생:                              │
│ "{고객명}님, {가맹점명}에서"           │
│ "주문하신 금액은 {금액}원입니다"       │
│                                        │
│ 데이터:                                │
│  - m_szcust_nm: 고객명                 │
│  - m_szterminal_nm: 가맹점명           │
│  - m_szgood_nm: 상품명                 │
│  - m_namount: 결제금액                 │
└────────────────────────────────────────┘
         ↓

[State 1] 주문정보 확인
KICC_getOrderInfo.cpp:1419-1449
┌────────────────────────────────────────┐
│ TTS 타임아웃 체크                      │
│ 확인멘트: "맞으면 1번, 아니면 2번"     │
└────────────────────────────────────────┘
         ↓

[State 2] 사용자 응답 처리
KICC_getOrderInfo.cpp:1450-1483
┌────────────────────────────────────────┐
│ DTMF='1' (맞습니다)                    │
│   → KICC_CardInput(0)                  │
│   → 카드정보 입력 시작                 │
│                                        │
│ DTMF='2' (아니오)                      │
│   → 시나리오 처음으로 복귀             │
│      - ARS: KICC_ArsScenarioStart(1)   │
│      - SMS: KICC_SMSScenarioStart(1)   │
└────────────────────────────────────────┘
```

#### 3.3 KICC_CardInput - 카드번호 입력 단계

```
[State 0] 카드번호 입력 요청
KICC_CardInput.cpp:1264-1271
┌────────────────────────────────────────┐
│ 음성안내: "카드번호 13~16자리를..."   │
│ 입력대기: 17자리 DTMF                  │
└────────────────────────────────────────┘
         ↓

[State 1] 카드번호 검증
KICC_CardInput.cpp:1272-1303
┌────────────────────────────────────────┐
│ 검증규칙:                              │
│  - 길이: 13~16자리 (*#으로 종료)       │
│  - 숫자만 허용                         │
│                                        │
│ TTS 재생: "입력하신 카드번호는..."    │
│ (각 숫자를 콤마로 구분하여 재생)       │
└────────────────────────────────────────┘
         ↓

[State 2] TTS 재생 완료 및 확인
KICC_CardInput.cpp:1304-1326
┌────────────────────────────────────────┐
│ TTS 타임아웃 체크                      │
│ 확인멘트 재생                          │
└────────────────────────────────────────┘
         ↓

[State 3] 카드번호 확정
KICC_CardInput.cpp:1327-1360
┌────────────────────────────────────────┐
│ DTMF='1' (맞습니다)                    │
│   → m_CardInfo.Card_Num에 저장         │
│   → KICC_EffecDate(0) 호출             │
│                                        │
│ DTMF='2' (아니오)                      │
│   → KICC_CardInput(0) 재시도           │
└────────────────────────────────────────┘
```

#### 3.4 KICC_EffecDate - 유효기간 입력 단계

```
[State 0] 유효기간 입력 요청
KICC_EffecDate.cpp:1131-1138
┌────────────────────────────────────────┐
│ 음성안내: "유효기간 연도 2자리..."    │
│ 입력대기: 4자리 DTMF (YYMM)            │
└────────────────────────────────────────┘
         ↓

[State 1] 유효기간 검증
KICC_EffecDate.cpp:1139-1187
┌────────────────────────────────────────┐
│ 검증규칙:                              │
│  - 월: 1~12                            │
│  - 년도: 현재년도 이상                 │
│  - 현재년도의 경우 현재월 이상         │
│                                        │
│ 형식변환: YY+2000년으로 처리           │
│ TTS 재생: "20XX년 YY월입니다"         │
│ 변수저장: m_CardInfo.ExpireDt          │
└────────────────────────────────────────┘
         ↓

[State 2~3] 확인 및 다음 단계
KICC_EffecDate.cpp:1188-1241
┌────────────────────────────────────────┐
│ DTMF='1' → KICC_JuminNo(0)             │
│ DTMF='2' → KICC_EffecDate(0) 재시도    │
└────────────────────────────────────────┘
```

#### 3.5 KICC_JuminNo - 주민/사업자번호 입력 단계

```
[State 0] 주민/사업자번호 입력
KICC_JuminNo.cpp:958-965
┌────────────────────────────────────────┐
│ 음성안내: "주민번호 앞자리 또는..."   │
│ 입력대기: 11자리 DTMF                  │
└────────────────────────────────────────┘
         ↓

[State 1] 번호 검증
KICC_JuminNo.cpp:966-1016
┌────────────────────────────────────────┐
│ 검증규칙:                              │
│  - 6자리: 생년월일 (주민번호 앞자리)   │
│    * 일자: 1~31 범위 체크              │
│  - 10자리: 사업자번호                  │
│  - 그 외: 오류                         │
│                                        │
│ TTS 재생: 각 숫자를 콤마로 구분        │
│ 변수저장: m_CardInfo.SecretNo          │
└────────────────────────────────────────┘
         ↓

[State 2~3] 확인 및 할부 처리
KICC_JuminNo.cpp:1017-1110
┌────────────────────────────────────────┐
│ DTMF='1' (맞습니다)                    │
│   → 할부개월 설정 확인                 │
│   → m_szInstallment > 0 이면           │
│      TTS: "할부 XX개월입니다"          │
│      → KICC_InstallmentCConfrim(3)     │
│   → 그외 KICC_InstallmentCConfrim(0)   │
│                                        │
│ DTMF='2' → 재시도                      │
└────────────────────────────────────────┘
```

#### 3.6 KICC_InstallmentCConfrim - 할부개월 확인 단계

```
[State 0] 할부개월 입력
KICC_InstallmentCConfrim.cpp:769-776
┌────────────────────────────────────────┐
│ 음성안내: "할부개월수를 입력..."       │
│ 입력대기: 3자리 DTMF                   │
└────────────────────────────────────────┘
         ↓

[State 1] 할부개월 검증
KICC_InstallmentCConfrim.cpp:777-833
┌────────────────────────────────────────┐
│ 검증규칙:                              │
│  - 0 또는 1: 일시불                    │
│    음성: "일시불로 선택하셨습니다"     │
│  - 2~12개월: 할부                      │
│    TTS: "할부 XX개월입니다"            │
│  - 그 외: 오류 재입력                  │
│                                        │
│ 변수저장: m_szInstallment              │
└────────────────────────────────────────┘
         ↓

[State 3/30] 할부정보 확인
KICC_InstallmentCConfrim.cpp:834-929
┌────────────────────────────────────────┐
│ TTS 타임아웃 체크                      │
│ TTS 재생: 할부개월 정보                │
│ 확인멘트: "맞으면 1번, 아니면 2번"     │
└────────────────────────────────────────┘
         ↓

[State 4] 최종 확인
KICC_InstallmentCConfrim.cpp:880-936
┌────────────────────────────────────────┐
│ DTMF='1' (맞습니다)                    │
│   → m_CardInfo.InstPeriod 설정         │
│   → KICC_CardPw(0) 호출                │
│                                        │
│ DTMF='2' (아니오)                      │
│   → KICC_InstallmentCConfrim(0) 재시도 │
└────────────────────────────────────────┘
```

#### 3.7 KICC_CardPw - 비밀번호 입력 단계

```
[State 0] 비밀번호 입력
KICC_CardPw.cpp:719-725
┌────────────────────────────────────────┐
│ 음성안내: "카드 비밀번호 앞 2자리..."  │
│ 입력대기: 2자리 DTMF                   │
└────────────────────────────────────────┘
         ↓

[State 1] 비밀번호 저장 및 결제 시작
KICC_CardPw.cpp:726-748
┌────────────────────────────────────────┐
│ 검증: 숫자만 허용                      │
│ 변수저장: m_CardInfo.Password          │
│                                        │
│ 결제 프로세스 시작:                    │
│  setPostfunc(POST_NET, KICC_payARS, 0) │
│  KiccPaymemt_host(90) 호출             │
└────────────────────────────────────────┘
```

### 4. 결제 처리 흐름

#### 4.1 KiccPaymemt_host - 결제 요청 준비

```cpp
// KICCpayMent.cpp:844-866
int KiccPaymemt_host(int holdm)
{
    CKICC_Scenario *pScenario = (CKICC_Scenario *)((*lpmt)->pScenario);

    // 대기 음악 재생
    if (holdm != 0) {
        new_guide();
        (*lpmt)->trials = 0;
        (*lpmt)->Hmusic = HM_LOOP;
        set_guide(holdm);
        send_guide(NODTMF);
    }

    // 기존 결제 스레드 대기
    if (pScenario->m_hPayThread) {
        WaitForSingleObject(pScenario->m_hPayThread, INFINITE);
        CloseHandle(pScenario->m_hPayThread);
        pScenario->m_hPayThread = NULL;
    }

    (*lpmt)->ppftbl[POST_NET].postcode = HI_NCMPLT;

    // 결제 처리 스레드 생성
    pScenario->m_hPayThread = (HANDLE)_beginthreadex(
        NULL, 0,
        KiccArsPayProcess,      // 결제 처리 함수
        (LPVOID)(*lpmt),
        0,
        &(pScenario->paythreadID)
    );

    return 0;
}
```

#### 4.2 KiccArsPayProcess - 실제 결제 처리 (별도 스레드)

```
KiccArsPayProcess 스레드 시작
KICCpayMent.cpp:446-686
┌────────────────────────────────────────┐
│ [1] 환경 설정 로드                     │
│  - KICC_GW_URL: 게이트웨이 URL         │
│  - KICC_GW_PORT: 포트                  │
│  - KICC_MAII_ID: 가맹점 ID             │
│  - KICC__CERT_FILE: 인증서 경로        │
│  - KICC_LOG: 로그 디렉터리             │
└────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────┐
│ [2] 외부 IP 주소 획득                  │
│  FindWebAddresss(ch, szClientIP)       │
│   - ipconfig.kr 접속                   │
│   - HTML 파싱하여 공인 IP 추출         │
└────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────┐
│ [3] KICC DLL 초기화                    │
│  lplfEP_CLI_DLL__init(                 │
│    szGwUrl,    // testgw.easypay.co.kr │
│    szGWport,   // 80                   │
│    CertFile,   // ./cert/pg_cert.pem   │
│    LogDir,     // ./KiccLog            │
│    LogLv,      // 1                    │
│    &obj_s_CFG_I                        │
│  )                                     │
└────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────┐
│ [4] 결제 요청 데이터 구성              │
│  A. 공통 데이터 (common)               │
│    - tot_amt: 총금액                   │
│    - currency: 통화코드 (00)           │
│    - client_ip: 고객 IP                │
│                                        │
│  B. 카드 데이터 (card)                 │
│    - card_txtype: 거래유형 (20)        │
│    - req_type: 카드정보 암호화 (0)     │
│    - card_amt: 카드금액                │
│    - noint: 무이자여부 (00)            │
│    - cert_type: 인증방식 (0)           │
│    - card_no: 카드번호                 │
│    - expire_date: 유효기간             │
│    - password: 비밀번호                │
│    - auth_value: 주민/사업자번호       │
│    - user_type: 개인(0)/법인(1)        │
│    - install_period: 할부개월          │
│                                        │
│  C. 주문 데이터 (order_data)           │
│    - order_no: 주문번호                │
│    - product_nm: 상품명                │
│    - product_amt: 상품금액             │
│    - user_define1~6: 사용자정의        │
│                                        │
│  D. 사용자 정보                        │
│    - user_nm: 사용자명                 │
│    - user_id: "ARS"                    │
│    - user_phone1: DNIS                 │
│    - user_phone2: 입력받은 전화번호    │
└────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────┐
│ [5] KICC 게이트웨이 호출               │
│  lplfEP_CLI_DLL__proc(                 │
│    szTrCd,        // "00101000" (승인) │
│    szMallid,      // 가맹점ID          │
│    szClientIP,    // 고객IP            │
│    szOrder_no,    // 주문번호          │
│    &obj_s_CFG_I,  // 환경설정          │
│    szResData,     // 응답버퍼          │
│    sizeof(szResData)                   │
│  )                                     │
│                                        │
│ → HTTPS 통신으로 KICC 서버 호출        │
└────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────┐
│ [6] 응답 데이터 파싱                   │
│  lplfEP_CLI_DLL__get_value() 사용      │
│                                        │
│  응답항목:                             │
│   - res_cd: 응답코드 (0000=성공)       │
│   - res_msg: 응답메시지                │
│   - auth_no: 승인번호                  │
│   - tran_date: 거래일시                │
│   - cno: PG거래번호                    │
│   - issuer_cd/nm: 발급사 코드/명       │
│   - uirer_cd: 매입사 코드              │
│   - acquirer_nm: 매입사명              │
│   - install_period: 할부개월           │
│                                        │
│  변수저장:                             │
│   → m_CardResInfo 구조체에 저장        │
└────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────┐
│ [7] 스레드 종료 및 결과 전달           │
│  (*port)[ch].ppftbl[POST_NET].postcode │
│    = HI_OK (성공)                      │
│                                        │
│  KiccNone_Pay_Quithostio()             │
│  _endthreadex()                        │
└────────────────────────────────────────┘
```

#### 4.3 KICC_payARS - 결제 결과 처리

```
[State 0] 결제 완료 대기 및 로그 저장
KICC_payARS.cpp:389-403
┌────────────────────────────────────────┐
│ 결제 시스템 코드 체크:                 │
│  - m_PaySysCd < 0: 시스템 오류         │
│                                        │
│ 결제 로그 DB 저장:                     │
│  setPostfunc(POST_NET, KICC_payARS, 1) │
│  setPayLog_host(92) 호출               │
└────────────────────────────────────────┘
         ↓

[State 1] 로그 저장 결과 및 오류 처리
KICC_payARS.cpp:404-546
┌────────────────────────────────────────┐
│ 로그 DB 저장 실패 (m_PayResult < 0):   │
│  → 결제 성공했으나 로그 실패           │
│  → 자동 취소 처리 시작                 │
│  → 취소정보 구성 (m_Card_CancleInfo)   │
│  → KiccPaymemtCancle_host(92)          │
│  → State 90으로 이동                   │
│                                        │
│ 로그 DB 저장 성공 (m_PayResult == 0):  │
│  → 결제 성공했으나 로그 미저장(재시도) │
│  → 동일하게 자동 취소 시도             │
│  → State 91로 이동                     │
│                                        │
│ 정상 처리 (m_PayResult > 0):           │
│  → REPLY_CODE != "0000": 결제 실패     │
│     TTS: "죄송합니다, {에러메시지}"    │
│     → State 91                         │
│  → REPLY_CODE == "0000": 결제 성공     │
│     특정 가맹점(05111031): SMS 발송    │
│     주문상태 업데이트:                 │
│     upOrderPayState_host(92)           │
│     → State 80                         │
└────────────────────────────────────────┘
         ↓

[State 80] 주문상태 업데이트 결과
KICC_payARS.cpp:550-667
┌────────────────────────────────────────┐
│ 업데이트 실패 (m_PayResult < 0):       │
│  → 자동 취소 처리                      │
│  → State 90                            │
│                                        │
│ 업데이트 성공:                         │
│  → 결제 성공 안내                      │
│  → "결제가 완료되었습니다"             │
│  → 통화 종료 (KICC_ExitSvc)            │
└────────────────────────────────────────┘
         ↓

[State 90] 취소 처리 결과 (오류)
KICC_payARS.cpp:668-672
┌────────────────────────────────────────┐
│ 시스템 오류 안내                       │
│ 통화 종료                              │
└────────────────────────────────────────┘
         ↓

[State 91] 결제 실패 안내
KICC_payARS.cpp:673-697
┌────────────────────────────────────────┐
│ TTS 타임아웃 체크                      │
│ 실패 사유 TTS 재생 (szTTSFile)         │
│ 음성: "결제 실패" 안내                 │
│ 통화 종료                              │
└────────────────────────────────────────┘
```

### 5. 결제 취소 흐름

#### KiccArsPayCancleProcess - 결제 취소 처리 (별도 스레드)

```
취소 스레드 시작
KICCpayMent.cpp:689-842
┌────────────────────────────────────────┐
│ [1] 환경 설정 로드 (승인과 동일)       │
│  - KICC_GW_URL, PORT, CERT 등          │
└────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────┐
│ [2] KICC DLL 초기화                    │
│  lplfEP_CLI_DLL__init()                │
└────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────┐
│ [3] 취소 요청 데이터 구성              │
│  트랜잭션 코드: "00201000" (관리)      │
│                                        │
│  mgr_data 엔트리:                      │
│   - mgr_txtype: "40" (거래취소)        │
│   - org_cno: 원거래 PG거래번호         │
│   - req_ip: 요청자 IP                  │
│   - req_id: 관리자 ID (KICC_ADMIN)     │
└────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────┐
│ [4] KICC 게이트웨이 취소 호출          │
│  lplfEP_CLI_DLL__proc(                 │
│    "00201000",    // 관리 트랜잭션     │
│    szMallid,                           │
│    szClientIP,                         │
│    ORDER_NO,                           │
│    &obj_s_CFG_I,                       │
│    szResData,                          │
│    sizeof(szResData)                   │
│  )                                     │
└────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────┐
│ [5] 취소 응답 파싱                     │
│  → m_Cancel_ResInfo에 저장             │
│   - REPLY_CODE: 취소 결과 코드         │
│   - REPLY_MESSAGE: 취소 메시지         │
│   - APPROVAL_NUM: 취소 승인번호        │
│   - CONTROL_NO: PG거래번호             │
└────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────┐
│ [6] 스레드 종료                        │
│  _endthreadex()                        │
└────────────────────────────────────────┘
```

### 6. SMS 시나리오 흐름 (KICC_SMSScenarioStart)

SMS 시나리오는 전화번호 대신 SMS로 받은 주문번호로 진행됩니다:

```
[State 0] 인사멘트
KICC_SMSScenarioStart.cpp:1650-1660
┌────────────────────────────────────────┐
│ 음성안내: 가맹점별 인사멘트            │
└────────────────────────────────────────┘
         ↓

[State 1] SMS 주문번호 입력
KICC_SMSScenarioStart.cpp:1661-1668
┌────────────────────────────────────────┐
│ 음성안내: "SMS로 받은 주문번호 입력"   │
│ 입력대기: 6자리 DTMF                   │
└────────────────────────────────────────┘
         ↓

[State 2] 주문번호 검증
KICC_SMSScenarioStart.cpp:1669-1700
┌────────────────────────────────────────┐
│ 검증: 숫자 6자리                       │
│ 변수저장: m_szAuth_no                  │
│ TTS 재생: 각 숫자 읽기                 │
└────────────────────────────────────────┘
         ↓

[State 3~4] 확인 및 DB 조회
KICC_SMSScenarioStart.cpp:1701-1756
┌────────────────────────────────────────┐
│ DTMF='1' (맞습니다)                    │
│   → getSMSOrderInfo_host(90) 호출      │
│   → SMS 주문번호로 DB 조회             │
│   → KICC_getOrderInfo(0)               │
│                                        │
│ DTMF='2' → 재입력                      │
└────────────────────────────────────────┘

이후 흐름은 ARS 시나리오와 동일
(카드정보 입력 → 결제 처리)
```

## 데이터 구조

### CARDINFO - 카드 입력 정보
```cpp
// KICC_Common.h:4-12
typedef struct CarsInfo {
    char Card_Num[16 + 1];     // 카드번호 (13~16자리)
    char ExpireDt[4 + 1];      // 유효기간 (YYMM)
    char SecretNo[10 + 1];     // 주민번호/사업자번호 (6 or 10자리)
    char Password[2 + 1];      // 비밀번호 앞 2자리
    char InstPeriod[2 + 1];    // 할부개월수 (00~12)
} CARDINFO;
```

### Card_ResInfo - 결제 응답 정보
```cpp
// KICC_Common.h:15-50
typedef struct Card_ResInfo {
    char ORDER_NO[32 + 1];         // 주문번호
    char TERMINAL_ID[32 + 1];      // 가맹점ID
    char TERMINAL_NM[50 + 1];      // 가맹점명
    char CUST_NM[64 + 1];          // 고객명
    char GOOD_NM[255 + 1];         // 상품명
    int  TOTAMOUNT;                // 총금액
    char PHONE_NO[32 + 1];         // 전화번호
    char PAYMENT_CODE;             // 결제코드 (1:성공, 2:실패)
    char REPLY_CODE[20 + 1];       // 응답코드 (0000=성공)
    char REPLY_MESSAGE[255 + 1];   // 응답메시지
    char CONTROL_NO[32 + 1];       // PG거래번호
    char APPROVAL_NUM[15 + 1];     // 승인번호
    char APPROVAL_DATE[20 + 1];    // 승인일시
    char issuer_cd[3 + 1];         // 발급사코드
    char issuer_nm[20 + 1];        // 발급사명
    char uirer_cd[3 + 1];          // 매입사코드
    char acquirer_nm[20 + 1];      // 매입사명
    char InstPeriod[2 + 1];        // 할부개월
} Card_ResInfo;
```

### Card_CancleInfo - 취소 요청 정보
```cpp
// KICC_Common.h:54-87
typedef struct Card_CancleInfo {
    // Card_ResInfo와 동일한 구조
    // 취소 시 원거래 정보를 담아서 전송
} Card_CancleInfo;
```

## 주요 함수 포인터

### IVR 시스템 함수 (메인 애플리케이션에서 제공)
```cpp
// KICC_Scenario_Travelport.cpp:17-65
void(*eprintf)(const char *str, ...);         // 에러 로그
void(*xprintf)(const char *str, ...);         // 일반 로그
void(*info_printf)(int chan, const char *str, ...);  // 채널 로그

void(*new_guide)(void);                       // 안내멘트 초기화
int(*set_guide)(int vid, ...);                // 안내멘트 설정
int(*send_guide)(int mode);                   // 안내멘트 송출
void(*setPostfunc)(int type, int(*func)(int), int poststate, int wtime);

int(*goto_hookon)(void);                      // 통화 종료
int(*check_validform)(char *form, char *data);  // DTMF 유효성 검사
int(*check_validdtmf)(int c, char *vkeys);    // DTMF 키 검증
int(*check_validkey)(char *data);             // 입력키 검증
int(*send_error)(void);                       // 오류 안내

int(*quitchan)(int chan);                     // 채널 종료
int(*in_multifunc)(int chan);                 // 멀티펑션 확인

int(*TTS_Play)(int chan, int holdm, const char *str, ...);  // TTS 재생
```

### KICC DLL 함수 포인터
```cpp
// KICCpayMent.cpp:159-187
typedef int(*lfEP_CLI_DLL__init)(char*, char*, char*, char*, int, PS_CFG_I);
typedef int(*lfEP_CLI_DLL__set_plan_data)(char*, PS_CFG_I);
typedef int(*lfEP_CLI_DLL__set_entry)(const char*, char* const, int);
typedef int(*lfEP_CLI_DLL__set_delim)(unsigned char, char*, int);
typedef int(*lfEP_CLI_DLL__set_value)(char*, char*, unsigned char, char*, int);
typedef int(*lfEP_CLI_DLL__proc)(char*, char*, char*, char*, PS_CFG_I, char*, int);
typedef int(*lfEP_CLI_DLL__get_value)(char*, char*, char*, int);

lfEP_CLI_DLL__init          lplfEP_CLI_DLL__init;
lfEP_CLI_DLL__set_plan_data lplfEP_CLI_DLL__set_plan_data;
lfEP_CLI_DLL__set_entry     lplfEP_CLI_DLL__set_entry;
lfEP_CLI_DLL__set_delim     lplfEP_CLI_DLL__set_delim;
lfEP_CLI_DLL__set_value     lplfEP_CLI_DLL__set_value;
lfEP_CLI_DLL__proc          lplfEP_CLI_DLL__proc;
lfEP_CLI_DLL__get_value     lplfEP_CLI_DLL__get_value;
```

## 상태 천이 다이어그램

```
                    ┌─────────────────────┐
                    │  jobArs() 진입점    │
                    │  - ARS/SMS 분기     │
                    └──────────┬──────────┘
                               │
              ┌────────────────┼────────────────┐
              │                                 │
     ┌────────▼─────────┐            ┌─────────▼────────┐
     │ ARS 시나리오      │            │ SMS 시나리오      │
     │ (전화번호 입력)   │            │ (주문번호 입력)   │
     └────────┬─────────┘            └─────────┬────────┘
              │                                 │
              └────────────────┬────────────────┘
                               │
                    ┌──────────▼───────────┐
                    │ getOrderInfo         │
                    │ (주문정보 조회/확인) │
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │ CardInput            │
                    │ (카드번호 입력)      │
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │ EffecDate            │
                    │ (유효기간 입력)      │
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │ JuminNo              │
                    │ (주민/사업자번호)    │
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │ InstallmentCConfrim  │
                    │ (할부개월 확인)      │
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │ CardPw               │
                    │ (비밀번호 입력)      │
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │ KiccPaymemt_host     │
                    │ (결제 스레드 시작)   │
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │ KiccArsPayProcess    │
                    │ (결제 처리 - 스레드) │
                    │ - KICC GW 호출       │
                    │ - 응답 수신          │
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │ KICC_payARS          │
                    │ (결제 결과 처리)     │
                    └──────────┬───────────┘
                               │
              ┌────────────────┼────────────────┐
              │                │                │
     ┌────────▼────────┐  ┌───▼───┐  ┌────────▼────────┐
     │ setPayLog_host  │  │ 성공  │  │ 취소 처리        │
     │ (로그 DB 저장)  │  │       │  │ (KiccArsPayCancle│
     └────────┬────────┘  └───┬───┘  │  Process)        │
              │               │       └────────┬────────┘
     ┌────────▼────────┐      │                │
     │ upOrderPayState │      │                │
     │ (주문상태 업데이트)│   │                │
     └────────┬────────┘      │                │
              │               │                │
              └───────┬───────┘                │
                      │                        │
                      │      ┌─────────────────┘
                      │      │
               ┌──────▼──────▼─────┐
               │ KICC_ExitSvc      │
               │ (서비스 종료 안내) │
               └──────┬────────────┘
                      │
               ┌──────▼────────┐
               │ goto_hookon() │
               │ (통화 종료)   │
               └───────────────┘
```

## 에러 처리 및 복구 메커니즘

### 1. 입력 검증 오류
```cpp
// 모든 입력 단계에서 공통 패턴
if (check_validform(...) < 0) {
    eprintf("오류 로그");
    return send_error();  // 오류 안내 후 재입력
}
```

### 2. DB 접근 오류
```cpp
// KICC_getOrderInfo.cpp:1382-1388
if (pScenario->m_DBAccess == -1 || pScenario->m_bDnisInfo == -1) {
    // 시스템 오류 안내
    set_guide(399);
    setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
    return send_guide(NODTMF);
}
```

### 3. 결제 시스템 오류
```cpp
// KICC_payARS.cpp:393-400
if (pScenario->m_PaySysCd < 0) {
    // 결제 시스템 오류 안내
    set_guide(399);
    setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
    return send_guide(NODTMF);
}
```

### 4. TTS 타임아웃 처리
```cpp
// 모든 TTS 재생 후 공통 체크
if (pScenario->m_TTSAccess == -1) {
    // TTS 타임아웃 안내
    set_guide(VOC_WAVE_ID, "ment\\TTS_TimeOut");
    setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
    return send_guide(NODTMF);
}
```

### 5. 자동 취소 메커니즘
```cpp
// KICC_payARS.cpp:406-453
// 결제는 성공했으나 DB 로그 저장 실패 시
if (pScenario->m_PayResult < 0) {
    if (strcmp(pScenario->m_CardResInfo.REPLY_CODE, "0000") == 0) {
        // 결제 취소 정보 구성
        memset(&pScenario->m_Card_CancleInfo, 0x00, ...);
        // 승인 정보를 취소 정보로 복사
        memcpy(pScenario->m_Card_CancleInfo.*,
               pScenario->m_CardResInfo.*, ...);

        // 자동 취소 실행
        setPostfunc(POST_NET, KICC_payARS, 90, 0);
        return KiccPaymemtCancle_host(92);
    }
}
```

## 설정 파일 (KiccPay_Travelport_para.ini)

```ini
[KICCPAY]
; KICC 게이트웨이 설정
KICC_GW_URL=testgw.easypay.co.kr    ; 테스트: testgw, 운영: gw
KICC_GW_PORT=80

; 클라이언트 IP (ipconfig.kr에서 자동 획득)
KICC_CLIENT_IP=211.196.157.123

; 가맹점 정보
KICC_MAII_ID=T5102001                ; 가맹점 ID

; 인증서 경로
KICC__CERT_FILE=./cert/pg_cert.pem

; 로그 설정
KICC_LOG=./KiccLog
KICC_LOG_LV=1

; 관리자 ID
KICC_ADMIN=ARS

; 할부 최소 금액 (주석 처리됨 - 2016.05.19)
;KICC_MIN_AMT=50000
```

## 보안 고려사항

### 1. 카드 정보 보안
- 카드번호, 비밀번호, 주민번호는 메모리에만 보관
- HTTPS 통신으로 KICC 게이트웨이 전송
- SSL 인증서 사용 (pg_cert.pem)
- 로그에 민감정보 출력 제한

### 2. 통신 보안
```cpp
// KICCpayMent.cpp:504-519
lplfEP_CLI_DLL__init(
    szGwUrl,      // testgw.easypay.co.kr
    szGWport,     // 80 (HTTPS는 DLL 내부 처리)
    CertFile,     // ./cert/pg_cert.pem
    LogDir,       // ./KiccLog
    LogLv,        // 1
    &obj_s_CFG_I
);
```

### 3. 세션 보안
- 각 통화마다 고유 Session_Key 사용
- 스레드별 독립적인 결제 처리 (paythreadID)
- 통화 종료 시 메모리 정리

## 성능 최적화

### 1. 멀티스레딩
- 결제 처리는 별도 스레드에서 실행
- 대기 음악 재생 중 결제 처리 진행
- 메인 IVR 스레드는 블로킹 없이 계속 동작

### 2. 비동기 처리
```cpp
// setPostfunc를 통한 비동기 이벤트 처리
setPostfunc(POST_NET, KICC_payARS, 1, 0);
return KiccPaymemt_host(90);

// POST_NET 완료 시 KICC_payARS(1) 자동 호출
```

### 3. 리소스 관리
```cpp
// 스레드 완료 대기 및 정리
if (pScenario->m_hPayThread) {
    WaitForSingleObject(pScenario->m_hPayThread, INFINITE);
    CloseHandle(pScenario->m_hPayThread);
    pScenario->m_hPayThread = NULL;
}
```

## 특수 가맹점 처리

### 1. 주문번호 특수 처리
```cpp
// KICCpayMent.cpp:594-605
// 특정 가맹점(05534047)은 주문번호 12자리만 사용
if (strcmp(pScenario->m_szterminal_id, "05534047") == EQUAL) {
    memcpy(szOrder_no, pScenario->m_szorder_no, 12);
    memcpy(szDefine3, pScenario->m_szorder_no, sizeof(szDefine3) - 1);
    lplfEP_CLI_DLL__set_value("order_no", szOrder_no, ...);
}
```

### 2. SMS 발송 가맹점
```cpp
// KICC_payARS.cpp:537-541
// 특정 가맹점(05111031)은 결제 성공 시 SMS 발송
if (strcmp(pScenario->m_szterminal_id, "05111031") == EQUAL) {
    setPostfunc(POST_NET, KICC_payARS, 70, 0);
    return SMS_host(92);
}
```

## 디버깅 및 로깅

### 1. 채널별 로그
```cpp
info_printf(localCh, "KICC_CardInput [%d] 카드번호 입력", state);
eprintf("KICC_CardInput [%d] 카드번호 입력", state);
```

### 2. KICC DLL 로그
- KICC_LOG 디렉터리에 거래 로그 자동 저장
- KICC_LOG_LV로 로그 레벨 조정 (1~3)

### 3. 상태 추적
- 각 함수는 state 파라미터로 현재 단계 추적
- prevState에 이전 상태 저장
- PrevCall에 이전 함수 포인터 저장

## 결론

KICC_Scenario_Travelport는 복잡한 상태 머신 기반의 IVR 결제 시스템입니다:

**핵심 특징**:
1. **상태 기반 설계**: 각 단계를 state로 관리하는 FSM (Finite State Machine) 패턴
2. **비동기 처리**: setPostfunc를 통한 이벤트 기반 비동기 흐름
3. **멀티스레딩**: 결제 처리는 별도 스레드에서 실행하여 성능 최적화
4. **에러 복구**: 자동 취소 메커니즘으로 데이터 일관성 보장
5. **확장성**: 플러그인 아키텍처로 다양한 결제 게이트웨이 지원

**주요 흐름**:
```
전화 수신 → 전화번호 입력 → 주문조회 → 카드정보 입력
→ 결제 처리 (별도 스레드) → 로그 저장 → 주문상태 업데이트 → 완료 안내
```

**보안 및 안정성**:
- SSL/HTTPS 통신
- 민감정보 메모리 관리
- 자동 취소 메커니즘
- TTS 타임아웃 처리
- 채널별 독립적 세션 관리

이 시스템은 24시간 무중단 운영이 가능한 안정적인 IVR 결제 솔루션입니다.
