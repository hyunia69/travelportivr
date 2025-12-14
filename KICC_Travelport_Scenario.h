// 다음 ifdef 블록은 DLL에서 내보내기하는 작업을 쉽게 해 주는 매크로를 만드는 
// 표준 방식입니다. 이 DLL에 들어 있는 파일은 모두 명령줄에 정의된 _EXPORTS 기호로
// 컴파일되며, 다른 프로젝트에서는 이 기호를 정의할 수 없습니다.
// 이렇게 하면 소스 파일에 이 파일이 들어 있는 다른 모든 프로젝트에서는 
// NICE_SCENARIO_API 함수를 DLL에서 가져오는 것으로 보고, 이 DLL은
// 이 DLL은 해당 매크로로 정의된 기호가 내보내지는 것으로 봅니다.
#pragma once
#ifdef WIN32


#else
#define STDMETHODCALLTYPE
#endif

// 이 클래스는 Nice_Scenario.dll에서 내보낸 것입니다.
class CKICC_Scenario : public IScenario
{
public:
public:
	int  STDMETHODCALLTYPE ScenarioInit(LPMTP *Port, char *ArsType);
	int  STDMETHODCALLTYPE jobArs(/* [in] */int state);

	// 데이터 베이스 접속 성공 여부
	int m_DBAccess;
	int m_bDnisInfo;//주문 내역 존재 여부

	CADODB *m_AdoDb;

	char  m_szpaymethod[10 + 1];
	char  m_szInputTel[127 + 1];      // 고객이 입력한 전화 번호
	char  m_szAuth_no[10 + 1];       // 고객이 입력한 주문 당시의 인증 번호
	char  m_szcust_nm[64 + 1];       // 고객명
	char  m_szgood_nm[255 + 1];      // 상품명
	char  m_szorder_no[32 + 1];      // 주문번호
	char  m_szterminal_id[32 + 1];   // 가맹점 아이디  ==> mall_id와 동일
	char  m_szterminal_pw[32 + 1];   // 가맹점 비번
	int   m_namount;                 // 결제금액
	char  m_szterminal_nm[50 + 1];   // 가맹점 명
	char  m_szInstallment[4 + 1];    // 할부 개월 수
	char  m_szMx_opt[255 + 1];       // 가맹점 접근키
	char  m_szphone_no[13 + 1];      // 데이터 베이스 상의 전화 번호

	char  m_szADMIN_ID[20 + 1];      // 결제/결제 취소 시 , 고정으로 관리자 
	char  m_szSHOP_PW[10 + 1];       // 상점 별 취소 시 비번
	char  m_szCC_email[200 + 1];     // 무이자를 emaill 필드를 이용해서 추가 20141115

	int   m_nServiceAmt;             // 면세 추가 20150312
	int   m_nSupplyAmt;              // 면세 추가 20150312
	int   m_nGoodsVat;               // 면세 추가 20150312
	int   m_nTaxFreeAmt;             // 면세 추가 20150312

	//   결제 취소 요청용 정보

	char m_szCONTROL_NO[32 + 1];     // PG거래번호
	char m_szAPPROVAL_NUM[15 + 1];   // 승인번호
	char m_szAPPROVAL_DATE[20 + 1];  // 승인일시

	CKICC_Scenario(void);
	~CKICC_Scenario(void);

	CARDINFO         m_CardInfo;// 결제 요청 정보
	Card_ResInfo     m_CardResInfo;// 결제 승인 정보

	Card_CancleInfo  m_Card_CancleInfo;//결제취소 요청 정보
	Card_ResInfo     m_Cancel_ResInfo;//결제 취소 승인 정보

	int              m_PaySysCd;

	int              m_PayResult;

	// ========================================
	// [2025-11-21 NEW] DB 기반 카드정보 필드
	// ========================================
	char m_szDB_CardPrefix[12 + 1];     // RESERVED_4: 카드번호 앞 12자리
	char m_szDB_ExpireDate[4 + 1];      // RESERVED_3: 유효기간 YYMM
	char m_szDB_InstallPeriod[2 + 1];   // RESERVED_5: 할부개월
	int  m_bUseDbCardInfo;              // DB 필드 사용 여부 (1:TRUE, 0:FALSE)

	// ========================================
	// [2025-11-22 NEW] 다중 주문 지원
	// ========================================
	MultiOrderInfo   m_MultiOrders;        // 다중 주문 컨테이너
	int              m_nCurrentOrderIdx;   // 현재 처리중인 주문 인덱스
	BOOL             m_bMultiOrderMode;    // 플래그: 다중 주문 처리 모드
	char             m_szAuthNo[12 + 1];   // SP에서 추출된 AUTH_NO 저장용 (로깅/디버깅)

	// ========================================
	// [2025-12-15 NEW] 휴대폰 번호 재시도 카운트
	// ========================================
	int              m_nPhoneRetryCount;   // 휴대폰 번호 입력 재시도 횟수 (0~3)

private:


};
