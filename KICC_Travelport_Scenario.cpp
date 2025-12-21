/****************************************************************************
*																			*
*	주   S E R V I C E	J O B S 											*
*																			*
****************************************************************************/
/* Head files */
#include "stdafx.h"
#include "CommonDef.H"
#include "KICC_Common.h"

#include "Scenaio.h"
#include "ADODB.h"
#include "KICC_Travelport_Scenario.h"

#define EQUAL 0

void(*eprintf)(const char *str, ...);
void(*xprintf)(const char *str, ...);
LPMTP **lpmt = NULL, **port = NULL;

//LPMTP	*curyport=NULL;
void(*info_printf)(int chan, const char *str, ...) = NULL;
void(*new_guide)(void) = NULL;
int(*set_guide)(int vid, ...);
void(*setPostfunc)(int type, int(*func)(int), int poststate, int wtime);
int(*send_guide)(int mode);
int(*goto_hookon)(void);
int(*check_validform)(char *form, char *data);
int(*send_error)(void);
int(*check_validdtmf)(int c, char *vkeys);
int(*in_multifunc)(int chan);
int(*quitchan)(int chan);
int(*CallAdd_Host)(int holdm);
//int  (*set_guide2)(int vid, char *format, ...);//파일명을 가변 조합하는 곳을 이것을 써라 강제다...

int(*atoi2)(char *p);
int(*atoi3)(char *p);
int(*atoi4)(char *p);
int(*atoiN)(char *p, int n);
long(*atolN)(char *p, int n);
int(*atox)(char *s);
int(*check_validkey)(char *data);

int(*TTS_Play)(int chan, int holdm, const char *str, ...);

extern int getSMSOrderInfo_host(int holdm);
extern int getOrderInfo_host(int holdm);
extern int getMultiOrderInfo_host(int holdm);
extern int setPayLog_host(int holdm);
extern int upOrderPayState_host(int holdm);


extern int  Kicc_Install();
extern int  Kicc_UnInstall();


extern int  KiccPaymemtCancle_host(int holdm);
extern int  KiccPaymemt_host(int holdm);

extern int  SMS_host(int holdm);

int KICC_ArsScenarioStart(/* [in] */int state);
int KICC_SMSScenarioStart(/* [in] */int state);
int KICC_getMultiOrderInfo(int state);
int KICC_AnnounceMultiOrders(int state);
int KICC_ProcessMultiPayments(int state);
int KICC_MultiPaymentSummary(int state);
int KICC_CardInput(int state);

#ifndef u_char
#define u_char	unsigned char
#define u_int	unsigned int
#define u_long	unsigned long
#endif

#define	TITLE_NAME	"ISDN PRI E1 - ARS"
#define	MAXSTRING	200
#define	MSG_SET_VIEW		WM_USER + 00
#define	MSG_INIT_LINE		WM_USER + 01
#define	MSG_SET_LINE		WM_USER + 02
#define	MSG_INBOUND_LINE	WM_USER + 03
#define	MSG_ASR_LINE	    WM_USER + 04

#define	PARAINI		".\\KiccPay_Travelport_para.ini"
#define MAXCHAN 	240		// 최대 회선 수
//#define MAXCHAN 	120		// 최대 회선 수


#define TRUE		1		// 참
#define FALSE		0		// 거짓

// [2025-12-15 NEW] 휴대폰 번호 입력 최대 재시도 횟수
#define MAX_PHONE_RETRY_COUNT  3

// [2025-12-17 NEW] 다중 주문 전부 실패 시 카드번호 재입력 최대 횟수
#define MAX_CARD_RETRY  3

#define HI_OK		0
#define HI_COMM 	98	// 통신 장애
#define HI_BADPKT	97	// BAD Packet

//////////////////////////////////////////////////////////////////////
#define VOC_MAIL_ID	500
#define VOC_MESG_ID	501
#define VOC_TEMP_ID	502
#define VOC_TTS_ID  503
#define VOC_WAVE_ID 504
#define VOC_MAIL	20		// 안내문
#define	VOC_MESG	21		// 사서함 메세지
#define VOC_TEMP	22		// Temp
#define VOC_TTS  	23		// TTS
#define VOC_WAVE  	24		// WAVE
///////////////////////////////////////////////////////////////////////

// Port 구분
#define	SERVER_PORT		(API_PORT) + 0
///////////////////////////////////////////////////////////////////////

#ifdef WIN32

// DLL 로딩 직후 반드시 위부 함수 인터페이스 포인터를 설정 해야 한다.
// 필요한 인터페이스만 설정 하도록 한다.
extern "C" {
	SCENARIO_API void Set_pinfo_printf(void(*pinfo_printf)(int chan, const char *str, ...));
	SCENARIO_API void Set_pnew_guide(void(*pnew_guide)(void));
	SCENARIO_API void Set_peprintf(void(*peprintf)(const char *str, ...));
	SCENARIO_API void Set_pxprintf(void(*pxprintf)(const char *str, ...));
	SCENARIO_API void Set_PortInfo(LPMTP **Line_lpmt, LPMTP **Tot_port);
	SCENARIO_API void Set_pset_guide(int(*pset_guide)(int vid, ...));
	SCENARIO_API void Set_psetPostfunc(void(*psetPostfunc)(int type, int(*func)(int), int poststate, int wtime));
	SCENARIO_API void Set_psend_guide(int(*psend_guide)(int mode));
	SCENARIO_API void Set_pgoto_hookon(int(*pgoto_hookon)(void));
	SCENARIO_API void Set_pcheck_validform(int(*pcheck_validform)(char *form, char *data));
	SCENARIO_API void Set_psend_error(int(*psend_error)(void));
	SCENARIO_API void Set_pcheck_validdtmf(int(*pcheck_validdtmf)(int c, char *vkeys));
	SCENARIO_API void Set_pin_multifunc(int(*pin_multifunc)(int chan));
	SCENARIO_API void Set_pquitchan(int(*pquitchan)(int chan));
	SCENARIO_API void Set_patoi2(int(*patoi2)(char *p));
	SCENARIO_API void Set_patoi3(int(*patoi3)(char *p));
	SCENARIO_API void Set_patoi4(int(*patoi4)(char *p));
	SCENARIO_API void Set_patoiN(int(*patoiN)(char *p, int n));
	SCENARIO_API void Set_patolN(long(*patolN)(char *p, int n));
	SCENARIO_API void Set_patox(int(*patox)(char *s));
	SCENARIO_API void Set_pcheck_validkey(int(*pcheck_validkey)(char *data));
	SCENARIO_API void Set_pTTS_Play(int(*pTTS_Play)(int chan, int holdm, const char *str, ...));
	SCENARIO_API void Set_CallAdd_Host(int(*pCallAdd_Host)(int holdm));
	/*
	SCENARIO_API void Set_pset_guide2(int(*pset_guide2)(int vid, char *format, ...));
	*/
	// 채널별 개체 생성자는 무조건 필요
	SCENARIO_API IScenario* CreateEngine();
	SCENARIO_API void DestroyEngine(IScenario* pComponent);

	void Set_pinfo_printf(void(*pinfo_printf)(int chan, const char *str, ...))
	{
		info_printf = pinfo_printf;
	}

	void Set_pnew_guide(void(*pnew_guide)(void))
	{
		new_guide = pnew_guide;
	}

	void Set_peprintf(void(*peprintf)(const char *str, ...))
	{
		eprintf = peprintf;
	}

	void Set_pxprintf(void(*pxprintf)(const char *str, ...))
	{
		xprintf = pxprintf;
	}

	void Set_PortInfo(LPMTP **Line_lpmt, LPMTP **Tot_port)
	{
		lpmt = Line_lpmt;
		port = Tot_port;
	}

	void Set_pset_guide(int(*pset_guide)(int vid, ...))
	{
		set_guide = pset_guide;
	}

	void Set_psetPostfunc(void(*psetPostfunc)(int type, int(*func)(int), int poststate, int wtime))
	{
		setPostfunc = psetPostfunc;
	}

	void Set_psend_guide(int(*psend_guide)(int mode))
	{
		send_guide = psend_guide;
	}

	void Set_pgoto_hookon(int(*pgoto_hookon)(void))
	{
		goto_hookon = pgoto_hookon;
	}

	void Set_pcheck_validform(int(*pcheck_validform)(char *form, char *data))
	{
		check_validform = pcheck_validform;
	}


	void Set_psend_error(int(*psend_error)(void))
	{
		send_error = psend_error;
	}

	void Set_pcheck_validdtmf(int(*pcheck_validdtmf)(int c, char *vkeys))
	{
		check_validdtmf = pcheck_validdtmf;
	}

	void Set_pin_multifunc(int(*pin_multifunc)(int chan))
	{
		in_multifunc = pin_multifunc;
	}

	void Set_pquitchan(int(*pquitchan)(int chan))
	{
		quitchan = pquitchan;
	}


	void Set_patoi2(int(*patoi2)(char *p))
	{
		atoi2 = patoi2;
	}

	void Set_patoi3(int(*patoi3)(char *p))
	{
		atoi3 = patoi3;
	}
	void Set_patoi4(int(*patoi4)(char *p))
	{
		atoi4 = patoi4;
	}
	void Set_patoiN(int(*patoiN)(char *p, int n))
	{
		atoiN = patoiN;
	}
	void Set_patolN(long(*patolN)(char *p, int n))
	{
		atolN = patolN;
	}
	void Set_patox(int(*patox)(char *s))
	{
		atox = patox;
	}

	void Set_pcheck_validkey(int(*pcheck_validkey)(char *data))
	{
		check_validkey = pcheck_validkey;
	}

	void Set_pTTS_Play(int(*pTTS_Play)(int chan, int holdm, const char *str, ...))
	{
		TTS_Play = pTTS_Play;
	}

	void Set_CallAdd_Host(int(*pCallAdd_Host)(int holdm))
	{
		CallAdd_Host = pCallAdd_Host;
	}
	/*
	void Set_pset_guide2(int(*pset_guide2)(int vid, char *format, ...))
	{
	set_guide2 = pset_guide2;
	}
	*/
	// 채널별 개체 생성자는 무조건 필요
	IScenario* CreateEngine()
	{
		return new CKICC_Scenario(); // CKICC_Scenario 시나리오 엔진의 개체 생성
	}

	void DestroyEngine(IScenario* pComponent)
	{
		delete (CKICC_Scenario *)pComponent; // CKICC_Secnario 시나리오 엔진의 개체 해제
	}

};
#else
#define HANDLE int

#define DLLEXPORT __attribute__ ((visibility("default")))
#define DLLLOCAL   __attribute__ ((visibility("hidden")))

extern "C" DLLEXPORT IScenario* CreateEngine();
extern "C" DLLEXPORT void DestroyEngine(IScenario* object);

extern DLLLOCAL FILE *FpRelease;
#endif


// ========================================
// [2025-12-22 NEW] 발신번호 유효성 검사 함수
// 휴대폰 번호 형식(010/011/012/016/017/018/019로 시작, 10~11자리)인지 검사
// ========================================
BOOL IsValidMobileNumber(const char* phoneNo)
{
	if (phoneNo == NULL) return FALSE;

	// 길이 검사: 10~11자리
	int len = strlen(phoneNo);
	if (len < 10 || len > 11) return FALSE;

	// 휴대폰 번호 prefix 검사
	if (strncmp(phoneNo, "010", 3) != 0 &&
		strncmp(phoneNo, "011", 3) != 0 &&
		strncmp(phoneNo, "012", 3) != 0 &&
		strncmp(phoneNo, "016", 3) != 0 &&
		strncmp(phoneNo, "017", 3) != 0 &&
		strncmp(phoneNo, "018", 3) != 0 &&
		strncmp(phoneNo, "019", 3) != 0)
	{
		return FALSE;
	}

	// 숫자만 포함되어 있는지 검사
	for (int i = 0; i < len; i++)
	{
		if (!isdigit(phoneNo[i])) return FALSE;
	}

	return TRUE;
}


int KICC_ExitSvc(int state)
{
	int		c = 0;
	int(*PrevCall)(int);
	int     localCh = (*lpmt)->chanID;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)((*lpmt)->pScenario);

	if (*lpmt)
	{
		c = *((*lpmt)->dtmfs);
		(*lpmt)->PrevCall = KICC_ExitSvc;
		(*lpmt)->prevState = state;
	}
	switch (state)
	{
	case 0:
		new_guide();
		info_printf(localCh, "KICC_ExitSvc [%d] 종료 서비스...", state);
		eprintf("KICC_ExitSvc [%d] 종료 서비스", state);
		set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\service_end");	 // "마지막 인사말"
		setPostfunc(POST_PLAY, KICC_ExitSvc, 0xffff, 0);
		return send_guide(NODTMF);
	case 10:
		new_guide();
		info_printf(localCh, "KICC_ExitSvc [%d] 종료 서비스...이용방법 확인...", state);
		eprintf("KICC_ExitSvc [%d] 종료 서비스:이용방법 확인...", state);
		set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\Error_end");	 // "이용방법 확인..."
		setPostfunc(POST_PLAY, KICC_ExitSvc, 0xffff, 0);
		return send_guide(NODTMF);
	case 0xffff:
		(*lpmt)->Myexit_service = NULL;
		return  goto_hookon();
		default:
			info_printf(localCh, "KICC_ExitSvc [%d] 종료 서비스...> 시나리오 아이디 오류", state);
			eprintf("KICC_ExitSvc[%d]  종료 서비스...>시나리오 아이디 오류", state);
			set_guide(399);
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
	}

}

// [다중 주문 지원 - 신규] 다중 주문 조회 상태 함수
int KICC_getMultiOrderInfo(int state)
{
	CKICC_Scenario* pScenario = (CKICC_Scenario*)((*lpmt)->pScenario);
	int c = *((*lpmt)->dtmfs);
	int localCh = (*lpmt)->chanID;
	(*lpmt)->PrevCall = KICC_getMultiOrderInfo;
	(*lpmt)->prevState = state;

	switch(state)
	{
	case 0:
		// 데이터베이스 조회 시작
		// AUTH_NO는 SP 내부에서 자동으로 추출됨
		info_printf(localCh, "다중 주문 조회: PHONE=%s (AUTH_NO는 SP에서 자동 추출)",
					pScenario->m_szInputTel);

		setPostfunc(POST_NET, KICC_getMultiOrderInfo, 1, 0);
		return getMultiOrderInfo_host(90);

	case 1:
		// 조회 결과 확인
		if (pScenario->m_DBAccess == -1) {
			// [MODIFIED] 데이터베이스 오류 - 재시도 횟수 확인 후 분기
			pScenario->m_nPhoneRetryCount++;
			info_printf(localCh, "KICC_getMultiOrderInfo[%d] 다중 주문 조회 실패 (재시도: %d/%d)",
						state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);
			eprintf("KICC_getMultiOrderInfo[%d] 다중 주문 조회 실패 (재시도: %d/%d)",
					state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);

			new_guide();
			set_guide(VOC_WAVE_ID, "ment/Travelport/no_order_msg");

			if (pScenario->m_nPhoneRetryCount >= MAX_PHONE_RETRY_COUNT) {
				// 3회 실패 - 종료 안내로 이동
				setPostfunc(POST_PLAY, KICC_getMultiOrderInfo, 3, 0);
			} else {
				// 재시도 가능 - 전화번호 재입력으로 이동
				setPostfunc(POST_PLAY, KICC_getMultiOrderInfo, 2, 0);
			}
			return send_guide(NODTMF);
		}

		if (pScenario->m_MultiOrders.nOrderCount == 0) {
			// [MODIFIED] 주문 없음 - 재시도 횟수 확인 후 분기
			pScenario->m_nPhoneRetryCount++;
			info_printf(localCh, "KICC_getMultiOrderInfo[%d] 주문 조회되지 않음 (재시도: %d/%d)",
						state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);
			eprintf("KICC_getMultiOrderInfo[%d] 주문 조회되지 않음 (재시도: %d/%d)",
					state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);

			new_guide();
			set_guide(VOC_WAVE_ID, "ment/Travelport/no_order_msg");

			if (pScenario->m_nPhoneRetryCount >= MAX_PHONE_RETRY_COUNT) {
				// 3회 실패 - 종료 안내로 이동
				setPostfunc(POST_PLAY, KICC_getMultiOrderInfo, 3, 0);
			} else {
				// 재시도 가능 - 전화번호 재입력으로 이동
				setPostfunc(POST_PLAY, KICC_getMultiOrderInfo, 2, 0);
			}
			return send_guide(NODTMF);
		}

		// 조회 성공 - 재시도 카운트 초기화
		pScenario->m_nPhoneRetryCount = 0;

		// 조회된 주문 정보 로깅
		if (pScenario->m_MultiOrders.nOrderCount > 0) {
			info_printf(localCh, "KICC_getMultiOrderInfo[%d] 주문조회 성공 [%s 타입]", state, pScenario->szArsType);
			eprintf("KICC_getMultiOrderInfo[%d] 조회된 주문 건수: %d, 총 금액: %d, AUTH_NO: %s",
					state,
					pScenario->m_MultiOrders.nOrderCount,
					pScenario->m_MultiOrders.nTotalAmount,
					pScenario->m_szAuthNo);
		}

		// [MODIFIED] 주문조회 성공 - 주문의 request_type에 따른 분기 처리
		// 디버깅: m_szRequestType 값 확인
		eprintf("KICC_getMultiOrderInfo[%d] m_szRequestType='%s', len=%d",
				state, pScenario->m_szRequestType, strlen(pScenario->m_szRequestType));

		// request_type이 "SMS" 또는 "TKT"인 경우에만 호전환 생략
		// 빈 값이거나 "ARS"이면 호전환 멘트 재생
		if (strcmp(pScenario->m_szRequestType, "SMS") == 0 || strcmp(pScenario->m_szRequestType, "TKT") == 0)
		{
			// SMS/TKT 타입: 바로 인사말 → 결제안내 (삼자통화 안내/대기 생략)
			eprintf("KICC_getMultiOrderInfo[%d] → 시스템 인사말 (Case 12) [request_type=%s]", state, pScenario->m_szRequestType);
			return KICC_ArsScenarioStart(12);
		}
		else
		{
			// ARS 타입 또는 빈값(기본값): 삼자통화 안내 → 3초 대기 → 인사말 → 결제안내
			eprintf("KICC_getMultiOrderInfo[%d] → 삼자통화 안내 (Case 10) [request_type=%s]", state,
					strlen(pScenario->m_szRequestType) > 0 ? pScenario->m_szRequestType : "ARS(default)");
			return KICC_ArsScenarioStart(10);
		}

	case 2:
		// [MODIFIED] 주문 없음 안내 후 전화번호 재입력으로 복귀
		info_printf(localCh, "KICC_getMultiOrderInfo[%d] 주문 없음>전화번호 재입력 (재시도: %d/%d)",
					state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);
		eprintf("KICC_getMultiOrderInfo[%d] 주문 없음>전화번호 재입력 (재시도: %d/%d)",
				state, pScenario->m_nPhoneRetryCount, MAX_PHONE_RETRY_COUNT);

		if (strcmp(pScenario->szArsType, "ARS") == 0) {
			return KICC_ArsScenarioStart(1);
		}
		else if (strcmp(pScenario->szArsType, "SMS") == 0) {
			return KICC_SMSScenarioStart(1);
		}
		else {
			return pScenario->jobArs(0);
		}

	case 3:
		// [NEW] 3회 재시도 실패 - 서비스 종료
		info_printf(localCh, "KICC_getMultiOrderInfo[%d] 전화번호 입력 %d회 실패>서비스 종료",
					state, MAX_PHONE_RETRY_COUNT);
		eprintf("KICC_getMultiOrderInfo[%d] 전화번호 입력 %d회 실패>서비스 종료",
				state, MAX_PHONE_RETRY_COUNT);

		// 일반 종료 멘트 재생 후 서비스 종료
		set_guide(399);
		setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
		return send_guide(NODTMF);
	}

	return 0;
}

// [다중 주문 지원 - 신규] 다중 주문 안내 상태 함수
int KICC_AnnounceMultiOrders(int state)
{
	CKICC_Scenario* pScenario = (CKICC_Scenario*)((*lpmt)->pScenario);
	int c = *((*lpmt)->dtmfs);
	int localCh = (*lpmt)->chanID;
	(*lpmt)->PrevCall = KICC_AnnounceMultiOrders;
	(*lpmt)->prevState = state;

	switch(state)
	{
	case 0:
		new_guide();
		info_printf(localCh, "안내: %d건 주문, 총 금액 ?%d",
					pScenario->m_MultiOrders.nOrderCount,
					pScenario->m_MultiOrders.nTotalAmount);

		if (TTS_Play) {
			setPostfunc(POST_NET, KICC_AnnounceMultiOrders, 1, 0);
			return TTS_Play(
				(*lpmt)->chanID, 92,
				// "%d건의 주문이 조회되었습니다. 총 결제 금액은 %d원입니다.",
				"고객님의 항공권 결제 금액은 %d건 , 총 승인금액은 %d 원입니다",
				pScenario->m_MultiOrders.nOrderCount,
				pScenario->m_MultiOrders.nTotalAmount
			);
		}
		else {
			set_guide(399);
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		}

	case 1:
		// TTS 재생 완료 후 확인 프롬프트
		info_printf(localCh, "KICC_AnnounceMultiOrders [%d] 다중 주문 안내>확인 부", state);
		eprintf("KICC_AnnounceMultiOrders [%d] 다중 주문 안내>확인 부", state);
		
		if (pScenario->m_TTSAccess == -1) {
			new_guide();
			info_printf(localCh, "KICC_AnnounceMultiOrders [%d] 현재 통화량이 많아!지연상황이 발생..", state);
			eprintf("KICC_AnnounceMultiOrders [%d] 현재 통화량이 많아!지연상황이 발생..", state);
			set_guide(VOC_WAVE_ID, "ment\\TTS_TimeOut");
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		}
		
		if (strlen(pScenario->szTTSFile) > 0) {
			new_guide();
			char TTSFile[2048 + 1] = { 0x00, };
			sprintf(TTSFile, "%s", pScenario->szTTSFile);
			set_guide(VOC_TTS_ID, TTSFile);
			set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\input_confirm");
			memset(pScenario->szTTSFile, 0x00, sizeof(pScenario->szTTSFile));
		}
		else {
			new_guide();
			set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\input_confirm");
		}
		
		setPostfunc(POST_DTMF, KICC_AnnounceMultiOrders, 2, 0);
		return send_guide(1);

	case 2:
		// 사용자 확인
		info_printf(localCh, "KICC_AnnounceMultiOrders[%d] state2 진입, c=%c(%d), szArsType=%s", state, c, c, pScenario->szArsType);
		eprintf("KICC_AnnounceMultiOrders[%d] state2 진입, c=%c(%d), szArsType=%s", state, c, c, pScenario->szArsType);

		if (!check_validdtmf(c, "12")) {
			eprintf("KICC_AnnounceMultiOrders[%d] check_validdtmf 실패", state);
			return send_error();
		}

		if (c == '1') {
			// 카드 입력으로 진행
			info_printf(localCh, "사용자가 다중 주문 결제 확인");
			return KICC_CardInput(0);
		}
		else {
			// [MODIFIED] 2번 입력 시 휴대폰 번호 입력 단계로 복귀
			info_printf(localCh, "KICC_AnnounceMultiOrders[%d] 다중 주문 안내>확인 부> 아니오 - 전화번호 재입력", state);
			eprintf("KICC_AnnounceMultiOrders[%d] 다중 주문 안내>확인 부> 아니오 - 전화번호 재입력", state);

			// ARS 타입에 따라 분기 (KICC_getOrderInfo 함수의 패턴 참조)
			if (strcmp(pScenario->szArsType, "ARS") == 0) {
				return KICC_ArsScenarioStart(1);  // 휴대폰 번호 입력 state
			}
			else if (strcmp(pScenario->szArsType, "SMS") == 0) {
				return KICC_SMSScenarioStart(1);
			}
			else {
				return pScenario->jobArs(0);
			}
		}
	}

	return 0;
}

// [다중 주문 지원 - 신규] 다중 결제 처리 루프
int KICC_ProcessMultiPayments(int state)
{
	CKICC_Scenario* pScenario = (CKICC_Scenario*)((*lpmt)->pScenario);
	int localCh = (*lpmt)->chanID;
	(*lpmt)->PrevCall = KICC_ProcessMultiPayments;
	(*lpmt)->prevState = state;

	int nCurrentIdx = pScenario->m_nCurrentOrderIdx;
	int nTotalCount = pScenario->m_MultiOrders.nOrderCount;

	switch(state)
	{
	case 0:
		// 결제 루프 초기화
		pScenario->m_nCurrentOrderIdx = 0;
		pScenario->m_MultiOrders.nProcessedCount = 0;
		pScenario->m_MultiOrders.nFailedCount = 0;
		memset(pScenario->m_MultiOrders.szFailedOrders, 0x00,
			   sizeof(pScenario->m_MultiOrders.szFailedOrders));

		info_printf(localCh, "%d건 주문 처리 시작", nTotalCount);

		// [MODIFIED] 결제 시작 전 대기 안내 멘트 한 번만 재생
		// "결제 요청 중입니다. 잠시만 기다려 주시기 바랍니다."
		new_guide();
		set_guide(VOC_WAVE_ID, "ment/Travelport/pay_request_wait");
		setPostfunc(POST_PLAY, KICC_ProcessMultiPayments, 7, 0);
		return send_guide(NODTMF);

	case 7:
		// [NEW] 대기 멘트 재생 완료 후 결제 루프 시작
		eprintf("[결제시작] 대기 멘트 재생 완료, %d건 결제 루프 시작", nTotalCount);
		return KICC_ProcessMultiPayments(1);

	case 1:
		// 모든 주문 처리 완료 확인
		if (nCurrentIdx >= nTotalCount) {
			info_printf(localCh, "모든 주문 처리 완료: 성공=%d, 실패=%d",
					   pScenario->m_MultiOrders.nProcessedCount,
					   pScenario->m_MultiOrders.nFailedCount);
			return KICC_MultiPaymentSummary(0);
		}

		// 현재 주문을 m_CardResInfo에 복사하여 처리
		memcpy(&pScenario->m_CardResInfo,
			   &pScenario->m_MultiOrders.orders[nCurrentIdx],
			   sizeof(Card_ResInfo));

		// KiccArsPayProcess에서 사용하는 필드들 설정
		// m_szterminal_id, m_szorder_no, m_szgood_nm, m_szcust_nm 등
		memset(pScenario->m_szterminal_id, 0x00, sizeof(pScenario->m_szterminal_id));
		strncpy(pScenario->m_szterminal_id, 
				pScenario->m_CardResInfo.TERMINAL_ID, 
				sizeof(pScenario->m_szterminal_id) - 1);
		
		memset(pScenario->m_szorder_no, 0x00, sizeof(pScenario->m_szorder_no));
		strncpy(pScenario->m_szorder_no, 
				pScenario->m_CardResInfo.ORDER_NO, 
				sizeof(pScenario->m_szorder_no) - 1);
		
		memset(pScenario->m_szgood_nm, 0x00, sizeof(pScenario->m_szgood_nm));
		strncpy(pScenario->m_szgood_nm, 
				pScenario->m_CardResInfo.GOOD_NM, 
				sizeof(pScenario->m_szgood_nm) - 1);
		
		memset(pScenario->m_szcust_nm, 0x00, sizeof(pScenario->m_szcust_nm));
		strncpy(pScenario->m_szcust_nm, 
				pScenario->m_CardResInfo.CUST_NM, 
				sizeof(pScenario->m_szcust_nm) - 1);
		
		// 금액 설정
		pScenario->m_namount = pScenario->m_CardResInfo.TOTAMOUNT;

		eprintf("주문 처리중 [%d/%d]: %s, 금액: ?%d",
				   nCurrentIdx + 1, nTotalCount,
				   pScenario->m_CardResInfo.ORDER_NO,
				   pScenario->m_CardResInfo.TOTAMOUNT);
		
		// 설정된 값 확인 로그
		eprintf("[주문정보설정] m_szterminal_id:%s, m_szorder_no:%s, m_szgood_nm:%s, m_namount:%d",
				   pScenario->m_szterminal_id,
				   pScenario->m_szorder_no,
				   pScenario->m_szgood_nm,
				   pScenario->m_namount);

		// 첫 입력에서 받은 카드 정보 복사 (모든 주문에 공유)
		memcpy(pScenario->m_CardResInfo.InstPeriod,
			   pScenario->m_CardInfo.InstPeriod,
			   sizeof(pScenario->m_CardInfo.InstPeriod));

		// 카드 정보 확인 로그 (디버깅용)
		eprintf("[카드정보확인] 카드번호길이:%d, 카드번호:%s, 유효기간:%s, 비밀번호:%s",
				   strlen(pScenario->m_CardInfo.Card_Num),
				   pScenario->m_CardInfo.Card_Num,
				   pScenario->m_CardInfo.ExpireDt,
				   pScenario->m_CardInfo.Password);

		// 승인 요청 로그
		eprintf("[승인요청] 주문번호:%s, 금액:%d, 가맹점ID:%s",
				   pScenario->m_CardResInfo.ORDER_NO,
				   pScenario->m_CardResInfo.TOTAMOUNT,
				   pScenario->m_CardResInfo.TERMINAL_ID);

		// [MODIFIED] 바로 승인 요청 (대기 멘트는 state 0에서 한 번만 재생)
		setPostfunc(POST_NET, KICC_ProcessMultiPayments, 2, 0);
		return KiccPaymemt_host(90);

	case 2:
	{
		// 결제 결과 확인
		// m_CardResInfo가 변경되었을 수 있으므로 원본 주문 정보에서 주문번호 가져오기
		char szCurrentOrderNo[32 + 1];
		memset(szCurrentOrderNo, 0x00, sizeof(szCurrentOrderNo));
		
		// 현재 처리 중인 주문의 주문번호 가져오기 (m_CardResInfo가 변경되었을 수 있음)
		if (nCurrentIdx < pScenario->m_MultiOrders.nOrderCount) {
			strncpy(szCurrentOrderNo, 
					pScenario->m_MultiOrders.orders[nCurrentIdx].ORDER_NO, 
					sizeof(szCurrentOrderNo) - 1);
		}
		else {
			// 백업: m_CardResInfo에서 가져오기
			strncpy(szCurrentOrderNo, 
					pScenario->m_CardResInfo.ORDER_NO, 
					sizeof(szCurrentOrderNo) - 1);
		}
		
		eprintf("[결제결과확인] 주문번호:%s, m_PayResult:%d, REPLY_CODE:%s, REPLY_MESSAGE:%s",
			   szCurrentOrderNo,
			   pScenario->m_PayResult,
			   pScenario->m_CardResInfo.REPLY_CODE,
			   pScenario->m_CardResInfo.REPLY_MESSAGE);
		
		// [MODIFIED] REPLY_CODE가 "0000"이면 성공
		// 주의: m_PayResult는 DB 작업 결과이므로 결제 성공 판단에 사용하면 안됨
		BOOL bSuccess = FALSE;
		if (strcmp(pScenario->m_CardResInfo.REPLY_CODE, "0000") == 0) {
			bSuccess = TRUE;
			pScenario->m_PayResult = 1;  // 성공 시 m_PayResult도 업데이트
		}
		
		if (bSuccess) {
			// 성공
			pScenario->m_MultiOrders.nProcessedCount++;
			eprintf("주문 %s 결제 성공 (REPLY_CODE:%s, 승인번호:%s)",
					   szCurrentOrderNo,
					   pScenario->m_CardResInfo.REPLY_CODE,
					   pScenario->m_CardResInfo.APPROVAL_NUM);
		}
		else {
			// 실패
			pScenario->m_MultiOrders.nFailedCount++;

			// 실패한 주문 목록에 추가
			if (strlen(pScenario->m_MultiOrders.szFailedOrders) > 0) {
				strcat_s(pScenario->m_MultiOrders.szFailedOrders,
						sizeof(pScenario->m_MultiOrders.szFailedOrders), ", ");
			}
			strcat_s(pScenario->m_MultiOrders.szFailedOrders,
					sizeof(pScenario->m_MultiOrders.szFailedOrders),
					szCurrentOrderNo);

			eprintf("주문 %s 결제 실패: %s (REPLY_CODE:%s)",
					   szCurrentOrderNo,
					   pScenario->m_CardResInfo.REPLY_MESSAGE,
					   pScenario->m_CardResInfo.REPLY_CODE);
		}

		// DB 저장 시작: 결제 로그 저장 (성공/실패 모두 기록)
		eprintf("[DB저장] 주문번호:%s 결제 로그 저장 시작", szCurrentOrderNo);
		setPostfunc(POST_NET, KICC_ProcessMultiPayments, 3, 0);
		return setPayLog_host(92);
	}

	case 3:
	{
		// 결제 로그 저장 결과 확인
		eprintf("[DB저장] 결제 로그 저장 결과: m_PayResult=%d", pScenario->m_PayResult);
		
		// 현재 주문번호 다시 가져오기
		char szCurrentOrderNo2[32 + 1];
		memset(szCurrentOrderNo2, 0x00, sizeof(szCurrentOrderNo2));
		if (nCurrentIdx < pScenario->m_MultiOrders.nOrderCount) {
			strncpy(szCurrentOrderNo2, 
					pScenario->m_MultiOrders.orders[nCurrentIdx].ORDER_NO, 
					sizeof(szCurrentOrderNo2) - 1);
		}
		else {
			strncpy(szCurrentOrderNo2, 
					pScenario->m_CardResInfo.ORDER_NO, 
					sizeof(szCurrentOrderNo2) - 1);
		}
		
		// [MODIFIED] REPLY_CODE가 "0000"이면 성공
		BOOL bSuccess2 = (strcmp(pScenario->m_CardResInfo.REPLY_CODE, "0000") == 0);

		if (bSuccess2) {
			// 결제 성공한 경우: 주문 상태 업데이트
			eprintf("[DB저장] 주문번호:%s 주문 상태 업데이트 시작", szCurrentOrderNo2);
			setPostfunc(POST_NET, KICC_ProcessMultiPayments, 4, 0);
			return upOrderPayState_host(92);
		}
		else {
			// 결제 실패한 경우: 로그만 저장하고 다음 주문으로
			eprintf("[DB저장] 주문번호:%s 결제 실패로 주문 상태 업데이트 생략", szCurrentOrderNo2);
			// 다음 주문으로 이동
			pScenario->m_nCurrentOrderIdx++;
			return KICC_ProcessMultiPayments(1);
		}
	}

	case 4:
	{
		// [MODIFIED] 주문 상태 업데이트 결과 확인 후 바로 다음 주문으로 이동
		// 개별 승인 완료 멘트 삭제 - 최종 요약에서만 안내 (ANALYZE_APPROVAL.md 수정 요청사항)
		eprintf("[DB저장] 주문 상태 업데이트 결과: m_PayResult=%d", pScenario->m_PayResult);

		// 현재 주문번호 로깅
		char szCurrentOrderNo3[32 + 1];
		memset(szCurrentOrderNo3, 0x00, sizeof(szCurrentOrderNo3));
		if (nCurrentIdx < pScenario->m_MultiOrders.nOrderCount) {
			strncpy(szCurrentOrderNo3,
					pScenario->m_MultiOrders.orders[nCurrentIdx].ORDER_NO,
					sizeof(szCurrentOrderNo3) - 1);
		}
		else {
			strncpy(szCurrentOrderNo3,
					pScenario->m_CardResInfo.ORDER_NO,
					sizeof(szCurrentOrderNo3) - 1);
		}

		eprintf("[승인완료] 주문번호:%s 승인 완료 (개별 멘트 생략, 최종 요약에서 안내)", szCurrentOrderNo3);

		// 다음 주문으로 바로 이동 (개별 멘트 재생 생략)
		pScenario->m_nCurrentOrderIdx++;
		return KICC_ProcessMultiPayments(1);
	}

	// [MODIFIED] state 5 제거 - state 4에서 바로 다음 주문으로 이동하므로 불필요
	// 기존 state 5 코드는 state 4에 통합됨
	}

	return 0;
}

// [다중 주문 지원 - 신규] 최종 요약 상태 함수
int KICC_MultiPaymentSummary(int state)
{
	CKICC_Scenario* pScenario = (CKICC_Scenario*)((*lpmt)->pScenario);
	int localCh = (*lpmt)->chanID;
	(*lpmt)->PrevCall = KICC_MultiPaymentSummary;
	(*lpmt)->prevState = state;

	switch(state)
	{
	case 0:
		new_guide();

		// [MODIFIED] 로그에 재입력 횟수 추가
		eprintf("[최종결과] 총주문:%d, 성공:%d, 실패:%d, 실패주문:%s, 재입력횟수:%d",
			   pScenario->m_MultiOrders.nOrderCount,
			   pScenario->m_MultiOrders.nProcessedCount,
			   pScenario->m_MultiOrders.nFailedCount,
			   pScenario->m_MultiOrders.szFailedOrders,
			   pScenario->m_nCardRetryCount);

		if (pScenario->m_MultiOrders.nFailedCount == 0) {
			// 모두 성공
			eprintf("모든 %d건 주문이 성공적으로 처리됨",
					   pScenario->m_MultiOrders.nProcessedCount);

			// [MODIFIED] 재입력 횟수 초기화 (성공 시)
			pScenario->m_nCardRetryCount = 0;

			if (TTS_Play) {
				setPostfunc(POST_NET, KICC_MultiPaymentSummary, 1, 0);
				// [MODIFIED] TTS 멘트 변경 - 담당직원 연결 안내 추가
				return TTS_Play(
					(*lpmt)->chanID, 92,
					"%d건의 결제가 완료되었습니다. 담당직원을 연결해 드리겠습니다.",
					pScenario->m_MultiOrders.nProcessedCount
				);
			}
		}
		// [NEW] 전부 실패 (nProcessedCount == 0, nFailedCount > 0)
		else if (pScenario->m_MultiOrders.nProcessedCount == 0) {
			// 전부 실패
			eprintf("전부 실패: 성공=%d, 실패=%d, 실패주문:%s",
					   pScenario->m_MultiOrders.nProcessedCount,
					   pScenario->m_MultiOrders.nFailedCount,
					   pScenario->m_MultiOrders.szFailedOrders);

			// 재입력 횟수 확인
			if (pScenario->m_nCardRetryCount < MAX_CARD_RETRY) {
				// 재입력 가능 - 안내 후 사용자 선택 (1번: 재결제, 2번: 담당직원 연결)
				pScenario->m_nCardRetryCount++;
				eprintf("[전부실패] 재입력 횟수: %d/%d - 사용자 선택 안내",
						   pScenario->m_nCardRetryCount, MAX_CARD_RETRY);

				if (TTS_Play) {
					setPostfunc(POST_NET, KICC_MultiPaymentSummary, 2, 0);  // state 2: TTS 재생 후 DTMF 입력 대기
					return TTS_Play(
						(*lpmt)->chanID, 92,
						"%d건 결제가 모두 실패하였습니다. 다시 결제를 진행하시려면 1번, 담당 직원을 연결하시려면 2번을 눌러 주시기 바랍니다.",
						pScenario->m_MultiOrders.nFailedCount
					);
				}
			}
			else {
				// 재입력 횟수 초과 - 안내 후 종료
				eprintf("[전부실패] 재입력 횟수 초과: %d/%d - 서비스 종료",
						   pScenario->m_nCardRetryCount, MAX_CARD_RETRY);

				if (TTS_Play) {
					setPostfunc(POST_NET, KICC_MultiPaymentSummary, 1, 0);  // state 1: 종료
					return TTS_Play(
						(*lpmt)->chanID, 92,
						"결제에 실패하였습니다. 다시 전화해 주시기 바랍니다."
					);
				}
			}
		}
		else {
			// 부분 실패 (일부 성공, 일부 실패)
			eprintf("부분 성공: 성공=%d, 실패=%d, 실패주문:%s",
					   pScenario->m_MultiOrders.nProcessedCount,
					   pScenario->m_MultiOrders.nFailedCount,
					   pScenario->m_MultiOrders.szFailedOrders);
			if (TTS_Play) {
				setPostfunc(POST_NET, KICC_MultiPaymentSummary, 1, 0);
				// [MODIFIED] TTS 멘트 변경 - 담당직원 연결 안내 추가
				return TTS_Play(
					(*lpmt)->chanID, 92,
					"%d건은 결제가 완료되었으며 %d건은 결제 실패하였습니다. 담당직원을 연결해 드리겠습니다.",
					pScenario->m_MultiOrders.nProcessedCount,
					pScenario->m_MultiOrders.nFailedCount
				);
			}
		}

		return send_guide(NODTMF);

	case 1:
		// TTS 재생 완료 후 실제 TTS 파일 재생
		eprintf("[최종안내] TTS 재생 완료, TTS 파일 재생 시작");

		// [MODIFIED] TTS 서버 오류 체크
		if (pScenario->m_TTSAccess == -1) {
			new_guide();
			eprintf("[최종안내] TTS 서버 오류, 타임아웃 멘트 재생");
			set_guide(VOC_WAVE_ID, "ment\\TTS_TimeOut");
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		}

		// [MODIFIED] szTTSFile을 사용하여 실제 TTS 파일 재생
		if (strlen(pScenario->szTTSFile) > 0) {
			new_guide();
			char TTSFile[2048 + 1] = { 0x00, };
			sprintf(TTSFile, "%s", pScenario->szTTSFile);
			eprintf("[최종안내] TTS 파일 재생: %s", TTSFile);
			set_guide(VOC_TTS_ID, TTSFile);
			memset(pScenario->szTTSFile, 0x00, sizeof(pScenario->szTTSFile));

			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		}

		// szTTSFile이 비어있으면 바로 종료
		eprintf("[최종안내] szTTSFile 비어있음, 바로 종료");
		return KICC_ExitSvc(0);

	// [NEW] 전부 실패 TTS 재생 완료 후 DTMF 입력 대기
	case 2:
		eprintf("[전부실패] TTS 재생 완료 - DTMF 입력 대기");

		// TTS 서버 오류 체크
		if (pScenario->m_TTSAccess == -1) {
			new_guide();
			eprintf("[전부실패] TTS 서버 오류, 타임아웃 멘트 재생 후 바로 DTMF 대기");
			set_guide(VOC_WAVE_ID, "ment\\TTS_TimeOut");
			// [MODIFIED] 바로 state 4로 이동하여 DTMF 처리
			setPostfunc(POST_DTMF, KICC_MultiPaymentSummary, 4, 10);  // 10초 타임아웃
			return send_guide(1);  // 1자리 DTMF 입력 대기
		}

		// [MODIFIED] szTTSFile을 사용하여 실제 TTS 파일 재생 + 바로 DTMF 입력 대기
		// 추가 확인 멘트("맞으면 1번...") 없이 바로 사용자 입력 대기
		if (strlen(pScenario->szTTSFile) > 0) {
			new_guide();
			char TTSFile[2048 + 1] = { 0x00, };
			sprintf(TTSFile, "%s", pScenario->szTTSFile);
			eprintf("[전부실패] TTS 파일 재생 후 바로 DTMF 대기: %s", TTSFile);
			set_guide(VOC_TTS_ID, TTSFile);
			memset(pScenario->szTTSFile, 0x00, sizeof(pScenario->szTTSFile));

			// [MODIFIED] 바로 state 4로 이동하여 DTMF 처리
			// 기존: POST_PLAY → state 3 (input_confirm 재생) → state 4
			// 수정: POST_DTMF → state 4 (바로 DTMF 대기)
			setPostfunc(POST_DTMF, KICC_MultiPaymentSummary, 4, 10);  // 10초 타임아웃
			return send_guide(1);  // 1자리 DTMF 입력 대기
		}

		// szTTSFile이 비어있으면 바로 DTMF 대기로 (state 3)
		eprintf("[전부실패] szTTSFile 비어있음 - state 3으로 이동 (대기음 재생)");
		// fall through to state 3

	// [MODIFIED] DTMF 입력 대기 - fallback용 (확인 멘트 없이 대기음만)
	case 3:
		eprintf("[전부실패] DTMF 입력 대기 (대기음) - 1번: 재결제, 2번: 종료");

		new_guide();
		// [MODIFIED] 불필요한 input_confirm 멘트 제거
		// 기존: set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\input_confirm");
		// 수정: 대기음만 재생 (확인 멘트 없음)
		set_guide(VOC_WAVE_ID, "ment\\wait_sound");  // 대기음만 재생
		setPostfunc(POST_DTMF, KICC_MultiPaymentSummary, 4, 10);  // 10초 타임아웃, DTMF 대기
		return send_guide(1);  // 1자리 DTMF 입력 대기

	// [NEW] DTMF 입력 결과 처리
	case 4:
		{
			// DTMF 입력 재안내 횟수 (static 변수로 함수 내에서 유지)
			// 유효한 입력(1번 또는 2번) 시 초기화됨
			static int s_nDTMFRetryCount = 0;

			char cDTMF = (*lpmt)->dtmfs[0];
			eprintf("[전부실패] DTMF 입력: %c (재안내 횟수: %d/3)", cDTMF, s_nDTMFRetryCount);

			if (cDTMF == '1') {
				// 1번: 다시 결제 진행 - 카드번호 입력으로 복귀
				eprintf("[전부실패] 1번 선택 - 카드번호 재입력으로 복귀");
				s_nDTMFRetryCount = 0;  // 재안내 횟수 초기화
				return KICC_CardInput(0);
			}
			else if (cDTMF == '2') {
				// 2번: 담당 직원 연결 - 종료
				eprintf("[전부실패] 2번 선택 - 담당 직원 연결 (종료)");
				s_nDTMFRetryCount = 0;  // 재안내 횟수 초기화
				return KICC_ExitSvc(0);
			}
			else {
				// 잘못된 입력 또는 타임아웃 - 재안내
				s_nDTMFRetryCount++;

				if (s_nDTMFRetryCount >= 3) {
					// 3회 재시도 후 자동 종료
					eprintf("[전부실패] 입력 재시도 초과 (%d회) - 자동 종료", s_nDTMFRetryCount);
					s_nDTMFRetryCount = 0;  // 초기화
					return KICC_ExitSvc(0);
				}

				eprintf("[전부실패] 잘못된 입력/타임아웃 - 재안내 (%d/3)", s_nDTMFRetryCount);

				// 멘트 재안내
				if (TTS_Play) {
					setPostfunc(POST_NET, KICC_MultiPaymentSummary, 2, 0);
					return TTS_Play(
						(*lpmt)->chanID, 92,
						"다시 결제를 진행하시려면 1번, 담당 직원을 연결하시려면 2번을 눌러 주시기 바랍니다."
					);
				}

				// TTS 실패 시 바로 DTMF 대기
				return KICC_MultiPaymentSummary(3);
			}
		}
	}

	return 0;
}

int KICC_jobArs(/* [in] */int state)
{
	int		c = 0;
	int(*PrevCall)(int);
	//int     localCh = curyport->chanID;
	int     localCh = (*lpmt)->chanID;

	//if (curyport)
	if (*lpmt)
	{
		//c = *curyport->dtmfs;
		//curyport->PrevCall = KICC_jobArs;
		//curyport->prevState = state;
		c = *((*lpmt)->dtmfs);
		(*lpmt)->PrevCall = KICC_jobArs;
		(*lpmt)->prevState = state;
	}

	switch (state)
	{
		case 0:
			new_guide();
			info_printf(localCh, "KICC_jobArs [%d] 서비스 준비 중...", state);
			eprintf("KICC_jobArs [%d] 서비스 준비", state);
			set_guide(VOC_WAVE_ID, "ment\\Svc");	 // "서비스 준비 중"
			setPostfunc(POST_PLAY, KICC_jobArs, 0xffff, 0);
			return send_guide(NODTMF);
			break;
		case 0xffff:
			return  goto_hookon();
		default:
			info_printf(localCh, "KICC_jobArs [%d] 서비스 준비 중...> 시나리오 아이디 오류", state);
			eprintf("KICC_jobArs[%d] 서비스 준비 중...>시나리오 아이디 오류", state);
			set_guide(399);
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);

	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////
// KICC_payARS : KICC 연동 후 처리 입력부
////////////////////////////////////////////////////////////////////////////////
int KICC_payARS(int state)
{
	int		c = 0;
	int(*PrevCall)(int);
	int     localCh = (*lpmt)->chanID;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)((*lpmt)->pScenario);

	if (*lpmt)
	{
		c = *((*lpmt)->dtmfs);
		(*lpmt)->PrevCall = KICC_payARS;
		(*lpmt)->prevState = state;
	}
	switch (state)
	{
		case 0:
			info_printf(localCh, "KICC_payARS [%d]Kicc 결제 모듈  연동 후 처리  부", state);
			eprintf("KICC_payARS [%d]Kicc 결제 모듈  연동 후 처리  부", state);
			new_guide();
			if (pScenario->m_PaySysCd < 0)
			{// 연동을 아예 하지 못함
				info_printf(localCh, "KICC_payARS [%d]  결제 연동 시스템 장애", state);
				eprintf("KICC_payARS [%d]  결제 연동 시스템 장애", state);
				set_guide(399);
				setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
				return send_guide(NODTMF);
			}
			//연동 성공 시 로그 쌓고
			setPostfunc(POST_NET, KICC_payARS, 1, 0);
			return setPayLog_host(92);
		case 1://연동 성공 시 로그 쌓기 후 처리부
			new_guide();
			if (pScenario->m_PayResult < 0)
			{
				info_printf(localCh, "KICC_payARS [%d]  결제 연동 후 로그 DB 기록 부 >  로그 DB 시스템 장애", state);
				eprintf("KICC_payARS [%d]  결제 연동 후 로그 DB 기록 부 >  로그 DB 시스템 장애", state);
				// 자동으로 결제 취소
				// 결제 연동 성공시 
				if (strcmp(pScenario->m_CardResInfo.REPLY_CODE, "0000") == 0)
				{
					memset(&pScenario->m_Card_CancleInfo, 0x00, sizeof(Card_CancleInfo));
					//DB로부터 획득 시 해당 하는 값으로 한다.
					memcpy(pScenario->m_Card_CancleInfo.acquirer_nm, pScenario->m_CardResInfo.acquirer_nm, sizeof(pScenario->m_Card_CancleInfo.acquirer_nm) - 1);
					memcpy(pScenario->m_Card_CancleInfo.ADMIN_ID, pScenario->m_CardResInfo.ADMIN_ID, sizeof(pScenario->m_Card_CancleInfo.ADMIN_ID) - 1);
					memcpy(pScenario->m_Card_CancleInfo.ADMIN_NAME, pScenario->m_CardResInfo.ADMIN_NAME, sizeof(pScenario->m_Card_CancleInfo.ADMIN_NAME) - 1);
					pScenario->m_Card_CancleInfo.AMOUNT = pScenario->m_CardResInfo.AMOUNT;
					memcpy(pScenario->m_Card_CancleInfo.APPROVAL_DATE, pScenario->m_CardResInfo.APPROVAL_DATE, sizeof(pScenario->m_Card_CancleInfo.APPROVAL_DATE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.APPROVAL_NUM, pScenario->m_CardResInfo.APPROVAL_NUM, sizeof(pScenario->m_Card_CancleInfo.APPROVAL_NUM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.CONTROL_NO, pScenario->m_CardResInfo.CONTROL_NO, sizeof(pScenario->m_Card_CancleInfo.CONTROL_NO) - 1);
					memcpy(pScenario->m_Card_CancleInfo.CUST_NM, pScenario->m_CardResInfo.CUST_NM, sizeof(pScenario->m_Card_CancleInfo.CUST_NM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.DNIS, pScenario->m_CardResInfo.DNIS, sizeof(pScenario->m_Card_CancleInfo.DNIS) - 1);
					memcpy(pScenario->m_Card_CancleInfo.GOOD_CD, pScenario->m_CardResInfo.GOOD_CD, sizeof(pScenario->m_Card_CancleInfo.GOOD_CD) - 1);
					memcpy(pScenario->m_Card_CancleInfo.GOOD_NM, pScenario->m_CardResInfo.GOOD_NM, sizeof(pScenario->m_Card_CancleInfo.GOOD_NM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.issuer_cd, pScenario->m_CardResInfo.issuer_cd, sizeof(pScenario->m_Card_CancleInfo.issuer_cd) - 1);
					memcpy(pScenario->m_Card_CancleInfo.issuer_nm, pScenario->m_CardResInfo.issuer_nm, sizeof(pScenario->m_Card_CancleInfo.issuer_nm) - 1);
					pScenario->m_Card_CancleInfo.NUMBER = pScenario->m_CardResInfo.NUMBER;
					if (strcmp(pScenario->m_szterminal_id, /*"05112158"*/"05534047") == EQUAL)
					{
						memcpy(pScenario->m_Card_CancleInfo.ORDER_NO, pScenario->m_CardResInfo.ORDER_NO, 12);
					}
					else
					{
						memcpy(pScenario->m_Card_CancleInfo.ORDER_NO, pScenario->m_CardResInfo.ORDER_NO, sizeof(pScenario->m_Card_CancleInfo.ORDER_NO) - 1);
					}
					pScenario->m_Card_CancleInfo.PAYMENT_CODE = pScenario->m_CardResInfo.PAYMENT_CODE;
					memcpy(pScenario->m_Card_CancleInfo.PHONE_NO, pScenario->m_CardResInfo.PHONE_NO, sizeof(pScenario->m_Card_CancleInfo.PHONE_NO) - 1);
					memcpy(pScenario->m_Card_CancleInfo.REPLY_CODE, pScenario->m_CardResInfo.REPLY_CODE, sizeof(pScenario->m_Card_CancleInfo.REPLY_CODE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.REPLY_DATE, pScenario->m_CardResInfo.REPLY_DATE, sizeof(pScenario->m_Card_CancleInfo.REPLY_DATE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.REPLY_MESSAGE, pScenario->m_CardResInfo.REPLY_MESSAGE, sizeof(pScenario->m_Card_CancleInfo.REPLY_MESSAGE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.RESERVED_2, pScenario->m_CardResInfo.RESERVED_2, sizeof(pScenario->m_Card_CancleInfo.RESERVED_2) - 1);
					memcpy(pScenario->m_Card_CancleInfo.RESERVED_3, pScenario->m_CardResInfo.RESERVED_3, sizeof(pScenario->m_Card_CancleInfo.RESERVED_3) - 1);
					memcpy(pScenario->m_Card_CancleInfo.TERMINAL_ID, pScenario->m_CardResInfo.TERMINAL_ID, sizeof(pScenario->m_Card_CancleInfo.TERMINAL_ID) - 1);
					memcpy(pScenario->m_Card_CancleInfo.TERMINAL_NM, pScenario->m_CardResInfo.TERMINAL_NM, sizeof(pScenario->m_Card_CancleInfo.TERMINAL_NM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.TERMINAL_PW, pScenario->m_CardResInfo.TERMINAL_PW, sizeof(pScenario->m_Card_CancleInfo.TERMINAL_PW) - 1);
					pScenario->m_Card_CancleInfo.TOTAMOUNT = pScenario->m_CardResInfo.TOTAMOUNT;
					memcpy(pScenario->m_Card_CancleInfo.uirer_cd, pScenario->m_CardResInfo.uirer_cd, sizeof(pScenario->m_Card_CancleInfo.uirer_cd) - 1);

					setPostfunc(POST_NET, KICC_payARS, 90, 0);
					return KiccPaymemtCancle_host(92);
				}
				else
				{
					set_guide(399);
					setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
					return send_guide(NODTMF);
	
				}
			}
			else if (pScenario->m_PayResult == 0)
			{  // 결제 연동 실패
				info_printf(localCh, "KICC_payARS [%d]  결제 연동 후 로그 DB 기록 부 >  로그 DB 적재 실패", state);
				eprintf("KICC_payARS [%d]  결제 연동 후 로그 DB 기록 부 >  로그 DB 적재 실패", state);
				// 자동으로 결제 취소
				if (strcmp(pScenario->m_CardResInfo.REPLY_CODE, "0000") == 0)
				{
					memset(&pScenario->m_Card_CancleInfo, 0x00, sizeof(Card_CancleInfo));
					//DB로부터 획득 시 해당 하는 값으로 한다.
					memcpy(pScenario->m_Card_CancleInfo.acquirer_nm, pScenario->m_CardResInfo.acquirer_nm, sizeof(pScenario->m_Card_CancleInfo.acquirer_nm) - 1);
					memcpy(pScenario->m_Card_CancleInfo.ADMIN_ID, pScenario->m_CardResInfo.ADMIN_ID, sizeof(pScenario->m_Card_CancleInfo.ADMIN_ID) - 1);
					memcpy(pScenario->m_Card_CancleInfo.ADMIN_NAME, pScenario->m_CardResInfo.ADMIN_NAME, sizeof(pScenario->m_Card_CancleInfo.ADMIN_NAME) - 1);
					pScenario->m_Card_CancleInfo.AMOUNT = pScenario->m_CardResInfo.AMOUNT;
					memcpy(pScenario->m_Card_CancleInfo.APPROVAL_DATE, pScenario->m_CardResInfo.APPROVAL_DATE, sizeof(pScenario->m_Card_CancleInfo.APPROVAL_DATE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.APPROVAL_NUM, pScenario->m_CardResInfo.APPROVAL_NUM, sizeof(pScenario->m_Card_CancleInfo.APPROVAL_NUM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.CONTROL_NO, pScenario->m_CardResInfo.CONTROL_NO, sizeof(pScenario->m_Card_CancleInfo.CONTROL_NO) - 1);
					memcpy(pScenario->m_Card_CancleInfo.CUST_NM, pScenario->m_CardResInfo.CUST_NM, sizeof(pScenario->m_Card_CancleInfo.CUST_NM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.DNIS, pScenario->m_CardResInfo.DNIS, sizeof(pScenario->m_Card_CancleInfo.DNIS) - 1);
					memcpy(pScenario->m_Card_CancleInfo.GOOD_CD, pScenario->m_CardResInfo.GOOD_CD, sizeof(pScenario->m_Card_CancleInfo.GOOD_CD) - 1);
					memcpy(pScenario->m_Card_CancleInfo.GOOD_NM, pScenario->m_CardResInfo.GOOD_NM, sizeof(pScenario->m_Card_CancleInfo.GOOD_NM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.issuer_cd, pScenario->m_CardResInfo.issuer_cd, sizeof(pScenario->m_Card_CancleInfo.issuer_cd) - 1);
					memcpy(pScenario->m_Card_CancleInfo.issuer_nm, pScenario->m_CardResInfo.issuer_nm, sizeof(pScenario->m_Card_CancleInfo.issuer_nm) - 1);
					pScenario->m_Card_CancleInfo.NUMBER = pScenario->m_CardResInfo.NUMBER;
					if (strcmp(pScenario->m_szterminal_id, /*"05112158"*/"05534047") == EQUAL)
					{
						memcpy(pScenario->m_Card_CancleInfo.ORDER_NO, pScenario->m_CardResInfo.ORDER_NO, 12);
					}
					else
					{
						memcpy(pScenario->m_Card_CancleInfo.ORDER_NO, pScenario->m_CardResInfo.ORDER_NO, sizeof(pScenario->m_Card_CancleInfo.ORDER_NO) - 1);
					}
					pScenario->m_Card_CancleInfo.PAYMENT_CODE = pScenario->m_CardResInfo.PAYMENT_CODE;
					memcpy(pScenario->m_Card_CancleInfo.PHONE_NO, pScenario->m_CardResInfo.PHONE_NO, sizeof(pScenario->m_Card_CancleInfo.PHONE_NO) - 1);
					memcpy(pScenario->m_Card_CancleInfo.REPLY_CODE, pScenario->m_CardResInfo.REPLY_CODE, sizeof(pScenario->m_Card_CancleInfo.REPLY_CODE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.REPLY_DATE, pScenario->m_CardResInfo.REPLY_DATE, sizeof(pScenario->m_Card_CancleInfo.REPLY_DATE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.REPLY_MESSAGE, pScenario->m_CardResInfo.REPLY_MESSAGE, sizeof(pScenario->m_Card_CancleInfo.REPLY_MESSAGE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.RESERVED_2, pScenario->m_CardResInfo.RESERVED_2, sizeof(pScenario->m_Card_CancleInfo.RESERVED_2) - 1);
					memcpy(pScenario->m_Card_CancleInfo.RESERVED_3, pScenario->m_CardResInfo.RESERVED_3, sizeof(pScenario->m_Card_CancleInfo.RESERVED_3) - 1);
					memcpy(pScenario->m_Card_CancleInfo.TERMINAL_ID, pScenario->m_CardResInfo.TERMINAL_ID, sizeof(pScenario->m_Card_CancleInfo.TERMINAL_ID) - 1);
					memcpy(pScenario->m_Card_CancleInfo.TERMINAL_NM, pScenario->m_CardResInfo.TERMINAL_NM, sizeof(pScenario->m_Card_CancleInfo.TERMINAL_NM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.TERMINAL_PW, pScenario->m_CardResInfo.TERMINAL_PW, sizeof(pScenario->m_Card_CancleInfo.TERMINAL_PW) - 1);
					pScenario->m_Card_CancleInfo.TOTAMOUNT = pScenario->m_CardResInfo.TOTAMOUNT;
					memcpy(pScenario->m_Card_CancleInfo.uirer_cd, pScenario->m_CardResInfo.uirer_cd, sizeof(pScenario->m_Card_CancleInfo.uirer_cd) - 1);

					setPostfunc(POST_NET, KICC_payARS, 91, 0);
					return KiccPaymemtCancle_host(92);
				}
				else
				{
					set_guide(399);
					setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
					return send_guide(NODTMF);
				}
			}
			else
			{//로그 쌓기 성공 시
				new_guide();
				if (strcmp(pScenario->m_CardResInfo.REPLY_CODE, "0000") != 0)
				{// 연동을 성공 다만 결제가 실제로 실패 되었으므로 자동 취소할 이유 없다.
					if (TTS_Play)
					{
						setPostfunc(POST_NET, KICC_payARS, 91, 0);
						return TTS_Play((*lpmt)->chanID, 92, "고객님, %s 이유로 인해 ", pScenario->m_CardResInfo.REPLY_MESSAGE);
					}
					else
					{
						set_guide(399);
						setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
						return send_guide(NODTMF);
					}
				}
				else
				{// 결제 성공 시
					// 주문 정보 SMS 발송
					// 홈플러스이면
					if (strcmp(pScenario->m_szterminal_id, "05111031") == EQUAL)
					{
						setPostfunc(POST_NET, KICC_payARS, 70, 0);
						return SMS_host(92);
					}
					setPostfunc(POST_NET, KICC_payARS, 80, 0);
					return upOrderPayState_host(92);
				}

			}
		case 70:
			setPostfunc(POST_NET, KICC_payARS, 80, 0);
			return upOrderPayState_host(92); 
		case 80:
			if (pScenario->m_PayResult < 0)
			{
				info_printf(localCh, "KICC_payARS [%d] 결제 연동 후 로그 DB 기록 부 >  주문 정보 결제 상태 기록 시스템 장애", state);
				eprintf("KICC_payARS [%d] 결제 연동 후 로그 DB 기록 부 >  주문 정보 결제 상태 기록 시스템 장애", state);
				// 자동으로 결제 취소
				// 결제믐 
				if (strcmp(pScenario->m_CardResInfo.REPLY_CODE, "0000") == 0)
				{
					memset(&pScenario->m_Card_CancleInfo, 0x00, sizeof(Card_CancleInfo));
					//DB로부터 획득 시 해당 하는 값으로 한다.
					memcpy(pScenario->m_Card_CancleInfo.acquirer_nm, pScenario->m_CardResInfo.acquirer_nm, sizeof(pScenario->m_Card_CancleInfo.acquirer_nm) - 1);
					memcpy(pScenario->m_Card_CancleInfo.ADMIN_ID, pScenario->m_CardResInfo.ADMIN_ID, sizeof(pScenario->m_Card_CancleInfo.ADMIN_ID) - 1);
					memcpy(pScenario->m_Card_CancleInfo.ADMIN_NAME, pScenario->m_CardResInfo.ADMIN_NAME, sizeof(pScenario->m_Card_CancleInfo.ADMIN_NAME) - 1);
					pScenario->m_Card_CancleInfo.AMOUNT = pScenario->m_CardResInfo.AMOUNT;
					memcpy(pScenario->m_Card_CancleInfo.APPROVAL_DATE, pScenario->m_CardResInfo.APPROVAL_DATE, sizeof(pScenario->m_Card_CancleInfo.APPROVAL_DATE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.APPROVAL_NUM, pScenario->m_CardResInfo.APPROVAL_NUM, sizeof(pScenario->m_Card_CancleInfo.APPROVAL_NUM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.CONTROL_NO, pScenario->m_CardResInfo.CONTROL_NO, sizeof(pScenario->m_Card_CancleInfo.CONTROL_NO) - 1);
					memcpy(pScenario->m_Card_CancleInfo.CUST_NM, pScenario->m_CardResInfo.CUST_NM, sizeof(pScenario->m_Card_CancleInfo.CUST_NM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.DNIS, pScenario->m_CardResInfo.DNIS, sizeof(pScenario->m_Card_CancleInfo.DNIS) - 1);
					memcpy(pScenario->m_Card_CancleInfo.GOOD_CD, pScenario->m_CardResInfo.GOOD_CD, sizeof(pScenario->m_Card_CancleInfo.GOOD_CD) - 1);
					memcpy(pScenario->m_Card_CancleInfo.GOOD_NM, pScenario->m_CardResInfo.GOOD_NM, sizeof(pScenario->m_Card_CancleInfo.GOOD_NM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.issuer_cd, pScenario->m_CardResInfo.issuer_cd, sizeof(pScenario->m_Card_CancleInfo.issuer_cd) - 1);
					memcpy(pScenario->m_Card_CancleInfo.issuer_nm, pScenario->m_CardResInfo.issuer_nm, sizeof(pScenario->m_Card_CancleInfo.issuer_nm) - 1);
					pScenario->m_Card_CancleInfo.NUMBER = pScenario->m_CardResInfo.NUMBER;
					if (strcmp(pScenario->m_szterminal_id, /*"05112158"*/"05534047") == EQUAL)
					{
						memcpy(pScenario->m_Card_CancleInfo.ORDER_NO, pScenario->m_CardResInfo.ORDER_NO, 12);
					}
					else
					{
						memcpy(pScenario->m_Card_CancleInfo.ORDER_NO, pScenario->m_CardResInfo.ORDER_NO, sizeof(pScenario->m_Card_CancleInfo.ORDER_NO) - 1);
					}
					pScenario->m_Card_CancleInfo.PAYMENT_CODE = pScenario->m_CardResInfo.PAYMENT_CODE;
					memcpy(pScenario->m_Card_CancleInfo.PHONE_NO, pScenario->m_CardResInfo.PHONE_NO, sizeof(pScenario->m_Card_CancleInfo.PHONE_NO) - 1);
					memcpy(pScenario->m_Card_CancleInfo.REPLY_CODE, pScenario->m_CardResInfo.REPLY_CODE, sizeof(pScenario->m_Card_CancleInfo.REPLY_CODE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.REPLY_DATE, pScenario->m_CardResInfo.REPLY_DATE, sizeof(pScenario->m_Card_CancleInfo.REPLY_DATE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.REPLY_MESSAGE, pScenario->m_CardResInfo.REPLY_MESSAGE, sizeof(pScenario->m_Card_CancleInfo.REPLY_MESSAGE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.RESERVED_2, pScenario->m_CardResInfo.RESERVED_2, sizeof(pScenario->m_Card_CancleInfo.RESERVED_2) - 1);
					memcpy(pScenario->m_Card_CancleInfo.RESERVED_3, pScenario->m_CardResInfo.RESERVED_3, sizeof(pScenario->m_Card_CancleInfo.RESERVED_3) - 1);
					memcpy(pScenario->m_Card_CancleInfo.TERMINAL_ID, pScenario->m_CardResInfo.TERMINAL_ID, sizeof(pScenario->m_Card_CancleInfo.TERMINAL_ID) - 1);
					memcpy(pScenario->m_Card_CancleInfo.TERMINAL_NM, pScenario->m_CardResInfo.TERMINAL_NM, sizeof(pScenario->m_Card_CancleInfo.TERMINAL_NM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.TERMINAL_PW, pScenario->m_CardResInfo.TERMINAL_PW, sizeof(pScenario->m_Card_CancleInfo.TERMINAL_PW) - 1);
					pScenario->m_Card_CancleInfo.TOTAMOUNT = pScenario->m_CardResInfo.TOTAMOUNT;
					memcpy(pScenario->m_Card_CancleInfo.uirer_cd, pScenario->m_CardResInfo.uirer_cd, sizeof(pScenario->m_Card_CancleInfo.uirer_cd) - 1);

					setPostfunc(POST_NET, KICC_payARS, 90, 0);
					return KiccPaymemtCancle_host(92);
				}
				else
				{
					set_guide(399);
					setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
					return send_guide(NODTMF);

				}
			}
			else if (pScenario->m_PayResult == 0)
			{  // 결제 연동 실패
				info_printf(localCh, "KICC_payARS [%d] 결제 연동 후 로그 DB 기록 부 >  주문 정보 결제 상태 실패", state);
				eprintf("KICC_payARS [%d] 결제 연동 후 로그 DB 기록 부 > 주문 정보 결제 상태 실패", state);
				// 자동으로 결제 취소
				if (strcmp(pScenario->m_CardResInfo.REPLY_CODE, "0000") == 0)
				{
					memset(&pScenario->m_Card_CancleInfo, 0x00, sizeof(Card_CancleInfo));
					//DB로부터 획득 시 해당 하는 값으로 한다.
					memcpy(pScenario->m_Card_CancleInfo.acquirer_nm, pScenario->m_CardResInfo.acquirer_nm, sizeof(pScenario->m_Card_CancleInfo.acquirer_nm) - 1);
					memcpy(pScenario->m_Card_CancleInfo.ADMIN_ID, pScenario->m_CardResInfo.ADMIN_ID, sizeof(pScenario->m_Card_CancleInfo.ADMIN_ID) - 1);
					memcpy(pScenario->m_Card_CancleInfo.ADMIN_NAME, pScenario->m_CardResInfo.ADMIN_NAME, sizeof(pScenario->m_Card_CancleInfo.ADMIN_NAME) - 1);
					pScenario->m_Card_CancleInfo.AMOUNT = pScenario->m_CardResInfo.AMOUNT;
					memcpy(pScenario->m_Card_CancleInfo.APPROVAL_DATE, pScenario->m_CardResInfo.APPROVAL_DATE, sizeof(pScenario->m_Card_CancleInfo.APPROVAL_DATE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.APPROVAL_NUM, pScenario->m_CardResInfo.APPROVAL_NUM, sizeof(pScenario->m_Card_CancleInfo.APPROVAL_NUM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.CONTROL_NO, pScenario->m_CardResInfo.CONTROL_NO, sizeof(pScenario->m_Card_CancleInfo.CONTROL_NO) - 1);
					memcpy(pScenario->m_Card_CancleInfo.CUST_NM, pScenario->m_CardResInfo.CUST_NM, sizeof(pScenario->m_Card_CancleInfo.CUST_NM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.DNIS, pScenario->m_CardResInfo.DNIS, sizeof(pScenario->m_Card_CancleInfo.DNIS) - 1);
					memcpy(pScenario->m_Card_CancleInfo.GOOD_CD, pScenario->m_CardResInfo.GOOD_CD, sizeof(pScenario->m_Card_CancleInfo.GOOD_CD) - 1);
					memcpy(pScenario->m_Card_CancleInfo.GOOD_NM, pScenario->m_CardResInfo.GOOD_NM, sizeof(pScenario->m_Card_CancleInfo.GOOD_NM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.issuer_cd, pScenario->m_CardResInfo.issuer_cd, sizeof(pScenario->m_Card_CancleInfo.issuer_cd) - 1);
					memcpy(pScenario->m_Card_CancleInfo.issuer_nm, pScenario->m_CardResInfo.issuer_nm, sizeof(pScenario->m_Card_CancleInfo.issuer_nm) - 1);
					pScenario->m_Card_CancleInfo.NUMBER = pScenario->m_CardResInfo.NUMBER;
					if (strcmp(pScenario->m_szterminal_id, /*"05112158"*/"05534047") == EQUAL)
					{
						memcpy(pScenario->m_Card_CancleInfo.ORDER_NO, pScenario->m_CardResInfo.ORDER_NO, 12);
					}
					else
					{
						memcpy(pScenario->m_Card_CancleInfo.ORDER_NO, pScenario->m_CardResInfo.ORDER_NO, sizeof(pScenario->m_Card_CancleInfo.ORDER_NO) - 1);
					}
					pScenario->m_Card_CancleInfo.PAYMENT_CODE = pScenario->m_CardResInfo.PAYMENT_CODE;
					memcpy(pScenario->m_Card_CancleInfo.PHONE_NO, pScenario->m_CardResInfo.PHONE_NO, sizeof(pScenario->m_Card_CancleInfo.PHONE_NO) - 1);
					memcpy(pScenario->m_Card_CancleInfo.REPLY_CODE, pScenario->m_CardResInfo.REPLY_CODE, sizeof(pScenario->m_Card_CancleInfo.REPLY_CODE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.REPLY_DATE, pScenario->m_CardResInfo.REPLY_DATE, sizeof(pScenario->m_Card_CancleInfo.REPLY_DATE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.REPLY_MESSAGE, pScenario->m_CardResInfo.REPLY_MESSAGE, sizeof(pScenario->m_Card_CancleInfo.REPLY_MESSAGE) - 1);
					memcpy(pScenario->m_Card_CancleInfo.RESERVED_2, pScenario->m_CardResInfo.RESERVED_2, sizeof(pScenario->m_Card_CancleInfo.RESERVED_2) - 1);
					memcpy(pScenario->m_Card_CancleInfo.RESERVED_3, pScenario->m_CardResInfo.RESERVED_3, sizeof(pScenario->m_Card_CancleInfo.RESERVED_3) - 1);
					memcpy(pScenario->m_Card_CancleInfo.TERMINAL_ID, pScenario->m_CardResInfo.TERMINAL_ID, sizeof(pScenario->m_Card_CancleInfo.TERMINAL_ID) - 1);
					memcpy(pScenario->m_Card_CancleInfo.TERMINAL_NM, pScenario->m_CardResInfo.TERMINAL_NM, sizeof(pScenario->m_Card_CancleInfo.TERMINAL_NM) - 1);
					memcpy(pScenario->m_Card_CancleInfo.TERMINAL_PW, pScenario->m_CardResInfo.TERMINAL_PW, sizeof(pScenario->m_Card_CancleInfo.TERMINAL_PW) - 1);
					pScenario->m_Card_CancleInfo.TOTAMOUNT = pScenario->m_CardResInfo.TOTAMOUNT;
					memcpy(pScenario->m_Card_CancleInfo.uirer_cd, pScenario->m_CardResInfo.uirer_cd, sizeof(pScenario->m_Card_CancleInfo.uirer_cd) - 1);

					setPostfunc(POST_NET, KICC_payARS, 91, 0);
					return KiccPaymemtCancle_host(92);
				}
				else
				{
					set_guide(399);
					setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
					return send_guide(NODTMF);
				}
			}
			else
			{
				new_guide();
				set_guide(VOC_WAVE_ID, "ment/Travelport/pay_success_msg");// 결제가 완료 되었습니다.
				setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
				return send_guide(NODTMF);
			}
		case 90:
			new_guide();
			set_guide(399);
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		case 91: //로그 DB 적재 실패 치
		{
			new_guide();
			if (pScenario->m_TTSAccess == -1)//201701.04
			{
				info_printf(localCh, "KICC_payARS [%d] 현재 통화량이 많아!지연상황이 발생..", state);
				eprintf("KICC_payARS [%d] 현재 통화량이 많아!지연상황이 발생..", state);
				set_guide(VOC_WAVE_ID, "ment\\TTS_TimeOut");	 // "현재 통화량이 많아!지연상황이 발생..."
				setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
				return send_guide(NODTMF);
			}
			if (strlen(pScenario->szTTSFile) > 0)
			{
				char TTSFile[2048 + 1] = { 0x00, };
				sprintf(TTSFile, "%s", pScenario->szTTSFile);
				set_guide(VOC_TTS_ID, TTSFile);
				memset(pScenario->szTTSFile, 0x00, sizeof(pScenario->szTTSFile));// 플래이 직후 삭제 처리(TTS)
			}
			set_guide(VOC_WAVE_ID, "ment/_common/common_audio/pay_fail_msg");// 결제 실패
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		}
	}
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////
// CardPw : 비밀 번호 입력부
////////////////////////////////////////////////////////////////////////////////
int KICC_CardPw(int state)
{
	int		c = 0;
	int(*PrevCall)(int);
	int     localCh = (*lpmt)->chanID;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)((*lpmt)->pScenario);

	if (*lpmt)
	{
		c = *((*lpmt)->dtmfs);
		(*lpmt)->PrevCall = KICC_CardPw;
		(*lpmt)->prevState = state;
	}

	switch (state)
	{
	case 0:
		info_printf(localCh, "KICC_CardPw [%d] 비밀번호 입력 부", state);
		eprintf("KICC_CardPw [%d] 비밀번호 입력 부", state);
		new_guide();
		set_guide(VOC_WAVE_ID, "ment/Travelport/input_pass_start");	 // "카드 비밀번호 네자리중, 앞, 두자리를 입력하여 주시기 바랍니다.
		setPostfunc(POST_DTMF, KICC_CardPw, 1, 0);
		return send_guide(2);
	case 1:
		if ((check_validkey("1234567890")) < 0)
		{
			eprintf("KICC_CardPw [%d] 비밀번호 입력 부>잘못 누르셨습니다.....", state);
			return send_error();
		}
		new_guide();
		memset(pScenario->m_CardInfo.Password, 0x00, sizeof(pScenario->m_CardInfo.Password));
		strncpy(pScenario->m_CardInfo.Password, (*lpmt)->dtmfs, sizeof(pScenario->m_CardInfo.Password) - 1);
		info_printf(localCh, "KICC_CardPw [%d]  비밀번호 입력부>결제 연동 부", state);
		eprintf("KICC_CardPw[%d]  비밀번호 입력부>결제 연동 부", state);

		// [다중 주문 지원] 다중 주문 모드인지 확인
		if (pScenario->m_bMultiOrderMode && pScenario->m_MultiOrders.nOrderCount > 1) {
			// 다중 주문 처리 모드
			info_printf(localCh, "다중 주문 결제 처리 시작: %d건", pScenario->m_MultiOrders.nOrderCount);
			return KICC_ProcessMultiPayments(0);
		}
		else {
			// [MODIFIED] 단일 주문 처리 모드 - 대기 멘트 재생 후 승인 요청
			// "결제 요청 중입니다. 잠시만 기다려 주시기 바랍니다."

			// [FIX] 단건일 때도 m_MultiOrders.orders[0]의 정보를 복사
			if (pScenario->m_bMultiOrderMode && pScenario->m_MultiOrders.nOrderCount == 1) {
				memcpy(&pScenario->m_CardResInfo,
					   &pScenario->m_MultiOrders.orders[0],
					   sizeof(Card_ResInfo));

				memset(pScenario->m_szterminal_id, 0x00, sizeof(pScenario->m_szterminal_id));
				strncpy(pScenario->m_szterminal_id,
						pScenario->m_CardResInfo.TERMINAL_ID,
						sizeof(pScenario->m_szterminal_id) - 1);

				memset(pScenario->m_szorder_no, 0x00, sizeof(pScenario->m_szorder_no));
				strncpy(pScenario->m_szorder_no,
						pScenario->m_CardResInfo.ORDER_NO,
						sizeof(pScenario->m_szorder_no) - 1);

				memset(pScenario->m_szgood_nm, 0x00, sizeof(pScenario->m_szgood_nm));
				strncpy(pScenario->m_szgood_nm,
						pScenario->m_CardResInfo.GOOD_NM,
						sizeof(pScenario->m_szgood_nm) - 1);

				memset(pScenario->m_szcust_nm, 0x00, sizeof(pScenario->m_szcust_nm));
				strncpy(pScenario->m_szcust_nm,
						pScenario->m_CardResInfo.CUST_NM,
						sizeof(pScenario->m_szcust_nm) - 1);

				pScenario->m_namount = pScenario->m_CardResInfo.TOTAMOUNT;

				eprintf("[단건주문정보설정] m_szterminal_id:%s, m_szorder_no:%s, m_namount:%d",
						pScenario->m_szterminal_id, pScenario->m_szorder_no, pScenario->m_namount);
			}

			set_guide(VOC_WAVE_ID, "ment/Travelport/pay_request_wait");
			setPostfunc(POST_PLAY, KICC_CardPw, 2, 0);
			return send_guide(NODTMF);
		}
	case 2:
	{
		// [NEW] 대기 멘트 재생 완료 후 단일 주문 승인 요청
		eprintf("KICC_CardPw[%d] 대기 멘트 재생 완료, 승인 요청 시작", state);
		setPostfunc(POST_NET, KICC_payARS, 0, 0);
		return KiccPaymemt_host(90);
	}
	default:
		info_printf(localCh, "KICC_CardPw [%d]  비밀번호 입력부>시나리오 아이디 오류", state);
		eprintf("KICC_CardPw[%d]  비밀번호 입력부>시나리오 아이디 오류", state);
		set_guide(399);
		setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
		return send_guide(NODTMF);
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////
// KICC_InstallmentCConfrim : 할부 개월수 확인 및 입력 부
//
// [2025-11-21 NOTE]
// DB 사용 모드(m_bUseDbCardInfo = TRUE)에서는 이 함수가 호출되지 않습니다.
// 할부개월은 DB의 RESERVED_5 필드에서 자동으로 로드됩니다.
//
// 기존 입력 방식(m_bUseDbCardInfo = FALSE)에서는 여전히 사용됩니다.
////////////////////////////////////////////////////////////////////////////////

int KICC_InstallmentCConfrim(int state)
{
	int		c = 0;
	int(*PrevCall)(int);
	int     localCh = (*lpmt)->chanID;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)((*lpmt)->pScenario);

	if (*lpmt)
	{
		c = *((*lpmt)->dtmfs);
		(*lpmt)->PrevCall = KICC_InstallmentCConfrim;
		(*lpmt)->prevState = state;
	}
	switch (state)
	{
		case 0:
			info_printf(localCh, "KICC_InstallmentCConfrim [%d] > 할부 개월 수  입력 부", state);
			eprintf("KICC_InstallmentCConfrim [%d] > 할부 개월 수  입력 부", state);

			new_guide();
			set_guide(VOC_WAVE_ID, "ment/_common/common_audio/input_halbu_start");	 // "요청하실 할부 개월수를 ... 일시불은 0번...."
			setPostfunc(POST_DTMF, KICC_InstallmentCConfrim, 1, 0);
			return send_guide(3);
		case 1:
			if ((check_validform("*#:1:2", (*lpmt)->refinfo)) < 0)
			{
				eprintf("KICC_InstallmentCConfrim[%d] > 할부 개월 수  입력 부>잘못 누르셨습니다.....", state);
				return send_error();
			}

			new_guide();
			// 2016.05.19
			// KICC 경우 해당 조건이 없다.
			/*if (pScenario->m_namount < GetPrivateProfileInt("KICC_PAYMEMT", "KICC_MIN_AMT", 50000, PARAINI))
			{
				if (TTS_Play)
				{
					memset(pScenario->m_szInstallment, 0x00, sizeof(pScenario->m_szInstallment));
					memcpy(pScenario->m_szInstallment, "00", sizeof(pScenario->m_szInstallment) - 1);
					setPostfunc(POST_NET, KICC_InstallmentCConfrim, 9, 0);
					return TTS_Play((*lpmt)->chanID, 92, "고객님께서 결제하실, 금액은 <vtml_speed value=\"90\">오만원미만으로</vtml_speed>, 일시불만 결제 하실수 있습니다.");
				}
				else
				{
					set_guide(399);
					setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
					return send_guide(NODTMF);
				}
			}
			else */if ((atoi((*lpmt)->refinfo) == 0) || (atoi((*lpmt)->refinfo) == 1))
			{//일시불을 요청하셨습니다.
				memset(pScenario->m_szInstallment, 0x00, sizeof(pScenario->m_szInstallment));
				set_guide(VOC_WAVE_ID, "ment/_common/common_audio/input_nohalbu_msg");
			}
			else if (atoi((*lpmt)->refinfo) > 12 ||
				(atoi((*lpmt)->refinfo) < 2))
			{//일시불과 이개월에서 십이개월 사이의 할부를 지원합니다.-
				set_guide(VOC_WAVE_ID, "ment/_common/common_audio/input_halbu_err");
				set_guide(VOC_WAVE_ID, "ment/_common/common_audio/input_halbu_start");//"요청하실 할부 개월수를
				setPostfunc(POST_DTMF, KICC_InstallmentCConfrim, 1, 0);
				return send_guide(3);
			}
			else
			{
				if (TTS_Play)
				{
					memset(pScenario->m_szInstallment, 0x00, sizeof(pScenario->m_szInstallment));
					memcpy(pScenario->m_szInstallment, (*lpmt)->refinfo, sizeof(pScenario->m_szInstallment) - 1);
					setPostfunc(POST_NET, KICC_InstallmentCConfrim, 3, 0);
					return TTS_Play((*lpmt)->chanID, 92, "고객님께서 요청하신 할부 개월수는 %d개월 입니다",
						atoi(pScenario->m_szInstallment));
				}
				else
				{
					set_guide(399);
					setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
					return send_guide(NODTMF);
				}

			}
		case 30:
			info_printf(localCh, "KICC_InstallmentCConfrim [%d] 입력 할부 개월 수 확인 부", state);
			eprintf("KICC_InstallmentCConfrim [%d] 입력 할부 개월 수 확인 부", state);
			if (pScenario->m_TTSAccess == -1)//201701.04
			{
				new_guide();
				info_printf(localCh, "KICC_InstallmentCConfrim [%d] 현재 통화량이 많아!지연상황이 발생..", state);
				eprintf("KICC_InstallmentCConfrim [%d] 현재 통화량이 많아!지연상황이 발생..", state);
				set_guide(VOC_WAVE_ID, "ment\\TTS_TimeOut");	 // "현재 통화량이 많아!지연상황이 발생..."
				setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
				return send_guide(NODTMF);
			}
			if (strlen(pScenario->szTTSFile) > 0)
			{
				new_guide();
				char TTSFile[2048 + 1] = { 0x00, };
				sprintf(TTSFile, "%s", pScenario->szTTSFile);
				set_guide(VOC_TTS_ID, TTSFile);
				memset(pScenario->szTTSFile, 0x00, sizeof(pScenario->szTTSFile));// 플래이 직후 삭제 처리
			}
			set_guide(VOC_WAVE_ID, "ment/_common/common_audio/input_confirm");
			setPostfunc(POST_DTMF, KICC_InstallmentCConfrim, 4, 0);
			return send_guide(1);
		case 3:
			info_printf(localCh, "KICC_InstallmentCConfrim [%d] 입력 할부 개월 수 확인 부", state);
			eprintf("KICC_InstallmentCConfrim [%d] 입력 할부 개월 수 확인 부", state);
			if (pScenario->m_TTSAccess == -1)//201701.04
			{
				new_guide();
				info_printf(localCh, "KICC_InstallmentCConfrim [%d] 현재 통화량이 많아!지연상황이 발생..", state);
				eprintf("KICC_InstallmentCConfrim [%d] 현재 통화량이 많아!지연상황이 발생..", state);
				set_guide(VOC_WAVE_ID, "ment\\TTS_TimeOut");	 // "현재 통화량이 많아!지연상황이 발생..."
				setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
				return send_guide(NODTMF);
			}
			if (strlen(pScenario->szTTSFile) > 0)
			{
				new_guide();
				char TTSFile[2048 + 1] = { 0x00, };
				sprintf(TTSFile, "%s", pScenario->szTTSFile);
				set_guide(VOC_TTS_ID, TTSFile);
				memset(pScenario->szTTSFile, 0x00, sizeof(pScenario->szTTSFile));// 플래이 직후 삭제 처리
			}
			set_guide(VOC_WAVE_ID, "ment/_common/common_audio/input_confirm");
			setPostfunc(POST_DTMF, KICC_InstallmentCConfrim, 4, 0);
			return send_guide(1);
		case 4:
			if (!check_validdtmf(c, "12"))	// 누른 DTMF값이 1,2만 유효하다.
			{
				eprintf("KICC_InstallmentCConfrim[%d] > 입력 할부 개월 수 확인 부>잘못 누르셨습니다....", state);
				return send_error();
			}
			new_guide();

			memset(pScenario->m_CardInfo.InstPeriod, 0x00, sizeof(pScenario->m_CardInfo.InstPeriod));
			if (c == '1') //예
			{
				sprintf(pScenario->m_CardInfo.InstPeriod, "%02d", atoi(pScenario->m_szInstallment));
				info_printf(localCh, "KICC_InstallmentCConfrim[%d] > 입력 할부 개월 수 확인 부>맞습니다.", state);
				eprintf("KICC_InstallmentCConfrim[%d] > 입력 할부 개월 수 확인 부>맞습니다.", state);

				return KICC_CardPw(0);
			}
			else if (c == '2')//아니오
			{
				info_printf(localCh, "KICC_InstallmentCConfrim[%d] > 입력 할부 개월 수 확인 부>아니오", state);
				eprintf("KICC_InstallmentCConfrim[%d] > 입력 할부 개월 수 확인 부>아니오", state);

				return KICC_InstallmentCConfrim(0);
			}
		case 9:
			info_printf(localCh, "KICC_InstallmentCConfrim [%d] 할부 개월 수 확인 부", state);
			eprintf("KICC_InstallmentCConfrim [%d] 할부 개월 수 확인 부", state);
			if (pScenario->m_TTSAccess == -1)//201701.04
			{
				new_guide();
				info_printf(localCh, "KICC_InstallmentCConfrim [%d] 현재 통화량이 많아!지연상황이 발생..", state);
				eprintf("KICC_InstallmentCConfrim [%d] 현재 통화량이 많아!지연상황이 발생..", state);
				set_guide(VOC_WAVE_ID, "ment\\TTS_TimeOut");	 // "현재 통화량이 많아!지연상황이 발생..."
				setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
				return send_guide(NODTMF);
			}
			if (strlen(pScenario->szTTSFile) > 0)
			{
				new_guide();
				char TTSFile[2048 + 1] = { 0x00, };
				sprintf(TTSFile, "%s", pScenario->szTTSFile);
				set_guide(VOC_TTS_ID, TTSFile);	 // "할부 개월수 안내"
				memset(pScenario->szTTSFile, 0x00, sizeof(pScenario->szTTSFile));// 플래이 직후 삭제 처리

			}
			memset(pScenario->m_CardInfo.InstPeriod, 0x00, sizeof(pScenario->m_CardInfo.InstPeriod));
			sprintf(pScenario->m_CardInfo.InstPeriod, "%02d", 0);
			setPostfunc(POST_PLAY, KICC_CardPw, 0, 0);
			return send_guide(NODTMF);
			return 0;
		default:
			info_printf(localCh, "KICC_InstallmentCConfrim [%d] 할부 개월 수 확인 부> 시나리오 아이디 오류", state);
			eprintf("KICC_InstallmentCConfrim[%d]  할부 개월 수 확인 부>시나리오 아이디 오류", state);
			set_guide(399);
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
	}
}

/////////////////////////////////////////////////////////////////////////////////
// JuminNo : 주민 번호 또는 법인 번호 입력부
////////////////////////////////////////////////////////////////////////////////
int KICC_JuminNo(int state)
{
	int		c = 0;
	int(*PrevCall)(int);
	int     localCh = (*lpmt)->chanID;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)((*lpmt)->pScenario);

	if (*lpmt)
	{
		c = *((*lpmt)->dtmfs);
		(*lpmt)->PrevCall = KICC_JuminNo;
		(*lpmt)->prevState = state;
	}

	switch (state)
	{
	case 0:
		info_printf(localCh, "KICC_JuminNo [%d] > 주민/법인 입력 부", state);
		eprintf("KICC_JuminNo [%d] > 주민/법인 입력 부", state);

		new_guide();
		set_guide(VOC_WAVE_ID, "ment/_common/common_audio/input_regno_front");	 // "생년 월일......"
		setPostfunc(POST_DTMF, KICC_JuminNo, 1, 0);
		return send_guide(11);
	case 1:
		if ((check_validform("*#:6:10", (*lpmt)->refinfo)) < 0)
		{
			eprintf("KICC_JuminNo[%d] > 주민 / 법인 입력 부>잘못 누르셨습니다.....", state);
			return send_error();
		}

		if (strlen((*lpmt)->refinfo) > 6 &&
			strlen((*lpmt)->refinfo) != 10)
		{
			eprintf("KICC_JuminNo[%d] > 주민 / 법인 입력 부>잘못 누르셨습니다( 6 && != 10 .....", state);
			return send_error();
		}
		// 생년월일 6자리로 입력 받을 경우, 1900년대와 2000년 대의 윤년을 특정하지 못한다.
		// 이에 따른 형식 체크가 무의미 해진다.
		// 또한 음력 생년월일 이용자도 존재 함으로써, 그에 따른 윤달 채크 및 윤년 체크가 되지 않는다.
		// 따라서 일자가 31을 초과 여부만 따질 수 있다.
		if (strlen((*lpmt)->refinfo) == 6)
		{
			int nDay = atoiN(&((*lpmt)->refinfo[4]), 2);

			if (nDay < 1 || nDay>31)
			{
				eprintf("KICC_JuminNo[%d] > 주민 / 법인 입력 부(일자 체크부) >잘못 누르셨습니다.....", state);
				return send_error();
			}
		}
		new_guide();

		info_printf(localCh, "KICC_JuminNo[%d] > 주민 / 법인 입력 부>확인 부(TTS)", state);
		eprintf("KICC_JuminNo[%d] > 주민 / 법인 입력 부>확인 부(TTS)", state);
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
			setPostfunc(POST_NET, KICC_JuminNo, 2, 0);
			return TTS_Play((*lpmt)->chanID, 92, "고객님께서 누르신 생년월일 또는 법인 번호는, %s 번 입니다.", TTSBuf);
		}
		else
		{
			set_guide(399);
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		}
	case 2:
		info_printf(localCh, "KICC_JuminNo [%d]  주민 / 법인  입력부>확인 부", state);
		eprintf("KICC_JuminNo [%d]  주민 / 법인 입력부>확인 부", state);
		if (pScenario->m_TTSAccess == -1)//201701.04
		{
			new_guide();
			info_printf(localCh, "KICC_JuminNo [%d] 현재 통화량이 많아!지연상황이 발생..", state);
			eprintf("KICC_JuminNo [%d] 현재 통화량이 많아!지연상황이 발생..", state);
			set_guide(VOC_WAVE_ID, "ment\\TTS_TimeOut");	 // "현재 통화량이 많아!지연상황이 발생..."
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		}
		if (strlen(pScenario->szTTSFile) > 0)
		{
			new_guide();
			char TTSFile[2048 + 1] = { 0x00, };
			sprintf(TTSFile, "%s", pScenario->szTTSFile);
			set_guide(VOC_TTS_ID, TTSFile);	 // "누르신 유효 기간은  XX년 XX월 입니다."
			set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\input_confirm");
			memset(pScenario->szTTSFile, 0x00, sizeof(pScenario->szTTSFile));// 플래이 직후 삭제 처리
		}
		setPostfunc(POST_DTMF, KICC_JuminNo, 3, 0);
		return send_guide(1);
	case 3:
		if (!check_validdtmf(c, "12"))	// 누른 DTMF값이 1,2만 유효하다.
		{
			eprintf("KICC_JuminNo[%d] > 주민 / 법인 입력 부> 확인 부>잘못 누르셨습니다....", state);
			return send_error();
		}
		new_guide();

		memset(pScenario->m_CardInfo.SecretNo, 0x00, sizeof(pScenario->m_CardInfo.SecretNo));
		if (c == '1') //예
		{
			strncpy(pScenario->m_CardInfo.SecretNo, (*lpmt)->refinfo, sizeof(pScenario->m_CardInfo.SecretNo) - 1);
			info_printf(localCh, "KICC_JuminNo[%d] > 주민 / 법인 입력 부> 확인 부>맞습니다.", state);
			eprintf("KICC_JuminNo[%d] > 주민 / 법인 입력 부> 확인 부>맞습니다.", state);

			// ========================================
			// [MODIFIED] DB 사용 여부에 따라 분기
			// ========================================
			if (pScenario->m_bUseDbCardInfo) {
				// [NEW] DB에서 할부개월 가져오기
				strncpy(pScenario->m_CardInfo.InstPeriod,
					pScenario->m_szDB_InstallPeriod,
					sizeof(pScenario->m_CardInfo.InstPeriod) - 1);

				// 할부개월 변환 (문자열 → 정수)
				int nInstall = atoi(pScenario->m_szDB_InstallPeriod);
				sprintf(pScenario->m_szInstallment, "%02d", nInstall);

				info_printf(localCh, "[KICC] DB 할부개월 사용: %s개월", pScenario->m_szInstallment);
				info_printf(localCh, "[KICC] 할부개월 입력 건너뛰기 (DB 사용)");

				// 할부개월 입력 단계 건너뛰고 비밀번호 입력으로 이동
				return KICC_CardPw(0);
			}
			else {
				// ========================================
				// [기존] 할부개월 입력 단계로 이동 (코드 보존)
				// ========================================
				if ((strlen(pScenario->m_szInstallment) > 0) &&
					(atoi(pScenario->m_szInstallment)) > 0)
				{
					if (TTS_Play)
					{
						memset(pScenario->m_szInstallment, 0x00, sizeof(pScenario->m_szInstallment));
						memcpy(pScenario->m_szInstallment, "00", sizeof(pScenario->m_szInstallment) - 1);
						setPostfunc(POST_NET, KICC_InstallmentCConfrim, 3, 0);
						return TTS_Play((*lpmt)->chanID, 92, "고객님께서 요청하신 할부 개월수는 %d개월 입니다",
							atoi(pScenario->m_szInstallment));
					}
					else
					{
						set_guide(399);
						setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
						return send_guide(NODTMF);
					}
				}// 개월수에 무관하게 50000 미만인 경우(환경 값)
				// 2016.05.19
				// KICC 경우 해당 조건이 없다.
				//else if (pScenario->m_namount < GetPrivateProfileInt("KICC_PAYMEMT", "KICC_MIN_AMT", 50000, PARAINI))
				//{
				//	if (TTS_Play)
				//	{
				//		memset(pScenario->m_szInstallment, 0x00, sizeof(pScenario->m_szInstallment));
				//		memcpy(pScenario->m_szInstallment, "00", sizeof(pScenario->m_szInstallment) - 1);
				//		setPostfunc(POST_NET, KICC_InstallmentCConfrim, 9, 0);
				//		return TTS_Play((*lpmt)->chanID, 92, "고객님께서 결제하실, 금액은 <vtml_speed value=\"90\">오만원미만으로</vtml_speed>, 일시불만 결제 하실수 있습니다.");
				//		//return KICC_InstallmentCConfrim(30);
				//	}
				//	else
				//	{
				//		set_guide(399);
				//		setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
				//		return send_guide(NODTMF);
				//	}
				//}
				return KICC_InstallmentCConfrim(0);
			}
		}
		else if (c == '2')//아니오
		{
			info_printf(localCh, "KICC_JuminNo[%d] > 주민 / 법인 입력 부> 확인 부>아니오", state);
			eprintf("KICC_JuminNo[%d] > 주민 / 법인 입력 부> 확인 부>아니오", state);

			return KICC_JuminNo(0);
		}
	default:
		info_printf(localCh, "KICC_JuminNo [%d]  주민 / 법인 입력 부> 시나리오 아이디 오류", state);
		eprintf("KICC_JuminNo[%d]  주민 / 법인 입력 부>시나리오 아이디 오류", state);
		set_guide(399);
		setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
		return send_guide(NODTMF);
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////
// EffecDate : 카드 유효 기간 입력부
//
// [2025-11-21 NOTE]
// DB 사용 모드(m_bUseDbCardInfo = TRUE)에서는 이 함수가 호출되지 않습니다.
// 유효기간은 DB의 RESERVED_3 필드에서 자동으로 로드됩니다.
//
// 기존 입력 방식(m_bUseDbCardInfo = FALSE)에서는 여전히 사용됩니다.
////////////////////////////////////////////////////////////////////////////////
int KICC_EffecDate(int state)
{
	int		c = 0;
	int(*PrevCall)(int);
	int     localCh = (*lpmt)->chanID;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)((*lpmt)->pScenario);

	if (*lpmt)
	{
		c = *((*lpmt)->dtmfs);
		(*lpmt)->PrevCall = KICC_EffecDate;
		(*lpmt)->prevState = state;
	}

	switch (state)
	{
		case 0:
			info_printf(localCh, "KICC_EffecDate [%d] 유효 기간 입력 부", state);
			eprintf("KICC_EffecDate [%d] 유효 기간 입력 부", state);

			new_guide();
			set_guide(VOC_WAVE_ID, "ment/_common/common_audio/input_cardexp_start");	 // "신용카드 앞 유효기간 년도와 월 순으로......"
			setPostfunc(POST_DTMF, KICC_EffecDate, 1, 0);
			return send_guide(4);
		case 1:
		{
			int    tmpMonth, tmpYear;
			CTime curTime = CTime::GetCurrentTime();//GetTickCount

			tmpMonth = atoiN(&((*lpmt)->dtmfs[2]), 2);//월
			tmpYear = atoiN(&((*lpmt)->dtmfs[0]), 2);//년
			tmpYear += 2000;

			if ((check_validkey("1234567890")) < 0)
			{
				eprintf("KICC_EffecDate [%d] 유효 기간 입력 부>잘못 누르셨습니다.....", state);
				return send_error();
			}

			if ((tmpMonth<1 || tmpMonth> 12) ||
				(tmpYear<curTime.GetYear()) ||       // 올해보다 이전의 년도는 이미 유효기간이 지났으므로 결제할 수 없다.
				(check_validkey && (check_validkey("1234567890")) < 0))
			{
				eprintf("KICC_EffecDate [%d] 일자 오류 유효 기간 입력 부>잘못 누르셨습니다.....", state);
				return send_error();
			}
			//2016.05.24
			//유효기간이 현재 년월보다 이전일 경우 오류 처리
			if ((tmpYear == curTime.GetYear()) &&
				(tmpMonth < curTime.GetMonth()))
			{
				eprintf("KICC_EffecDate [%d] 일자 오류 유효 기간 입력 부>잘못 누르셨습니다.....", state);
				return send_error();
			}
			new_guide();
			// 유호기간을 년월로 그대로 입력한다.	
			memset(pScenario->m_CardInfo.ExpireDt, 0x00, sizeof(pScenario->m_CardInfo.ExpireDt));
			memcpy(pScenario->m_CardInfo.ExpireDt, (*lpmt)->dtmfs, sizeof(pScenario->m_CardInfo.ExpireDt) - 1);

			info_printf(localCh, "KICC_EffecDate [%d] 유효 기간 입력 부>확인 부(TTS)", state);
			eprintf("KICC_EffecDate [%d] 유효 기간 입력 부>확인 부(TTS)", state);
			if (TTS_Play)
			{
				setPostfunc(POST_NET, KICC_EffecDate, 2, 0);
				return TTS_Play((*lpmt)->chanID, 92, "고객님께서 누르신 유효 기간은, 20%d 년, %d월 입니다.", atoiN(&((*lpmt)->dtmfs[0]), 2), atoiN(&((*lpmt)->dtmfs[2]), 2));
			}
			else
			{
				set_guide(399);
				setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
				return send_guide(NODTMF);
			}
		}
		case 2:
			info_printf(localCh, "KICC_EffecDate [%d] Card 번호 입력부>확인 부", state);
			eprintf("KICC_EffecDate [%d] Card 번호 입력부>확인 부", state);
			if (pScenario->m_TTSAccess == -1)//201701.04
			{
				new_guide();
				info_printf(localCh, "KICC_EffecDate [%d] 현재 통화량이 많아!지연상황이 발생..", state);
				eprintf("KICC_EffecDate [%d] 현재 통화량이 많아!지연상황이 발생..", state);
				set_guide(VOC_WAVE_ID, "ment\\TTS_TimeOut");	 // "현재 통화량이 많아!지연상황이 발생..."
				setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
				return send_guide(NODTMF);
			}
			if (strlen(pScenario->szTTSFile) > 0)
			{
				new_guide();
				char TTSFile[2048 + 1] = { 0x00, };
				sprintf(TTSFile, "%s", pScenario->szTTSFile);
				set_guide(VOC_TTS_ID, TTSFile);	 // "누르신 유효 기간은  XX년 XX월 입니다."
				set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\input_confirm");
				memset(pScenario->szTTSFile, 0x00, sizeof(pScenario->szTTSFile));// 플래이 직후 삭제 처리
			}
			setPostfunc(POST_DTMF, KICC_EffecDate, 3, 0);
			return send_guide(1);
		case 3:
			if (!check_validdtmf(c, "12"))	// 누른 DTMF값이 1,2만 유효하다.
			{
				eprintf("KICC_EffecDate [%d] 유효 기간 입력 부>확인 부>잘못 누르셨습니다....", state);
				return send_error();
			}
			new_guide();
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
			else if (c == '2')//아니오
			{
				info_printf(localCh, "KICC_EffecDate[%d] 유효 기간 입력 부>확인 부>아니오", state);
				eprintf("KICC_EffecDate[%d] 유효 기간 입력 부>확인 부>아니오", state);

				return KICC_EffecDate(0);
			}
		default:
			info_printf(localCh, "KICC_EffecDate [%d]  유효 기간 입력 부> 시나리오 아이디 오류", state);
			eprintf("KICC_EffecDate[%d]  유효 기간 입력 부>시나리오 아이디 오류", state);
			set_guide(399);
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
	}

	return 0;

}


/////////////////////////////////////////////////////////////////////////////////
// CardInput : 카드 번호
////////////////////////////////////////////////////////////////////////////////
int KICC_CardInput(int state)
{
	int		c = 0;
	int(*PrevCall)(int);
	int     localCh = (*lpmt)->chanID;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)((*lpmt)->pScenario);

	if (*lpmt)
	{
		c = *((*lpmt)->dtmfs);
		(*lpmt)->PrevCall = KICC_CardInput;
		(*lpmt)->prevState = state;
	}

	switch (state)
	{
		case 0:
			info_printf(localCh, "KICC_CardInput [%d]  Card 번호 입력부", state);
			eprintf("KICC_CardInput [%d]  Card 번호 입력부", state);

			if (new_guide)new_guide();
			// [FIX] dtmfs 초기화 - check_validform이 dtmfs를 검사하므로 초기화 필요
			memset((*lpmt)->dtmfs, 0x00, sizeof((*lpmt)->dtmfs));
			// ========================================
			// [MODIFIED] DB 사용 여부에 따라 분기
			// ========================================
			// set_guide(VOC_WAVE_ID, "ment/_common/common_audio/input_card_num_var");	 // "카드번호 ..우물정자를..."
			// setPostfunc(POST_DTMF, KICC_CardInput, 1, 0);

			if (pScenario->m_bUseDbCardInfo) {
				// [NEW] DB 사용 시: 4자리 입력 (종료 문자 없이 자동 처리)
				info_printf(localCh, "[KICC] DB 사용 모드 - 카드번호 뒤 4자리 입력");
				set_guide(VOC_WAVE_ID, "ment/Travelport/input_card_num_4");	 // "카드번호 ..우물정자를..."
				setPostfunc(POST_DTMF, KICC_CardInput, 1, 0);
				return send_guide(4);
			}
			else {
				// [기존] 전체 카드번호 입력 (최대 17자리)
				info_printf(localCh, "[KICC] 기존 모드 - 카드번호 16자리 입력");
				set_guide(VOC_WAVE_ID, "ment/_common/common_audio/input_card_num_var");	 // "카드번호 ..우물정자를..."
				setPostfunc(POST_DTMF, KICC_CardInput, 1, 0);
				return send_guide(17);
			}
		case 1:
		{
			char szInputCard[20];
			memset(szInputCard, 0x00, sizeof(szInputCard));
			
			// ========================================
			// [MODIFIED] DB 사용 여부에 따라 검증
			// ========================================
			if (pScenario->m_bUseDbCardInfo) {
				// [NEW] 뒤 4자리 검증 - KICC_EffecDate 패턴 적용 (종료 문자 없이 처리)
				// dtmfs에서 직접 읽기 (종료 문자가 없으므로 숫자만 들어있음)
				strncpy(szInputCard, (*lpmt)->dtmfs, sizeof(szInputCard) - 1);
				
				// 길이 검증 (정확히 4자리)
				int nLen = strlen(szInputCard);
				if (nLen != 4) {
					eprintf("KICC_CardInput [%d]  Card 번호 입력부>잘못 누르셨습니다.....", state);
					return send_error();
				}
				
				// 숫자만 허용
				if (check_validkey(szInputCard) < 0) {
					eprintf("KICC_CardInput [%d]  Card 번호 입력부>잘못 누르셨습니다.....", state);
					return send_error();
				}

				// [NEW] DB 앞자리(12자리) + 입력받은 뒤자리(4자리) = 16자리 완성
				char szFullCard[17];
				memset(szFullCard, 0x00, sizeof(szFullCard));
				
				// DB 카드번호 앞자리 확인
				if (strlen(pScenario->m_szDB_CardPrefix) != 12) {
					eprintf("[KICC] DB 카드번호 앞자리 길이 오류: %d자리 (기대: 12자리), 값: %s",
						strlen(pScenario->m_szDB_CardPrefix),
						pScenario->m_szDB_CardPrefix);
					return send_error();
				}
				
				sprintf_s(szFullCard, sizeof(szFullCard), "%s%s", pScenario->m_szDB_CardPrefix, szInputCard);

				// 최종 카드번호 저장
				memset(pScenario->m_CardInfo.Card_Num, 0x00, sizeof(pScenario->m_CardInfo.Card_Num));
				strncpy(pScenario->m_CardInfo.Card_Num, szFullCard, 16);
				pScenario->m_CardInfo.Card_Num[16] = '\0';

				eprintf("[KICC] 완성된 카드번호: %s (길이:%d, DB: %s + 입력: %s)",
					pScenario->m_CardInfo.Card_Num,
					strlen(pScenario->m_CardInfo.Card_Num),
					pScenario->m_szDB_CardPrefix,
					szInputCard);
			}
			else {
				// ========================================
				// [기존] 전체 카드번호 입력 방식 (보존)
				// ========================================
				if ((check_validform) && (check_validform("*#:13:16", (*lpmt)->refinfo)) < 0)	// 눌린 dtmf값이 13~16의 길이이며 *또는#으로 끝이났다.
				{
					eprintf("KICC_CardInput [%d]  Card 번호 입력부>잘못 누르셨습니다.....", state);
					return send_error();
				}

				if (check_validkey(szInputCard) < 0) {
					return send_error();
				}

				strncpy(pScenario->m_CardInfo.Card_Num, szInputCard, sizeof(pScenario->m_CardInfo.Card_Num) - 1);
			}

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
		case 2:
			info_printf(localCh, "KICC_CardInput [%d] Card 번호 입력부>확인 부", state);
			eprintf("KICC_CardInput [%d] Card 번호 입력부>확인 부", state);
			if (pScenario->m_TTSAccess == -1)//201701.04
			{
				new_guide();
				info_printf(localCh, "KICC_CardInput [%d] 현재 통화량이 많아!지연상황이 발생..", state);
				eprintf("KICC_CardInput [%d] 현재 통화량이 많아!지연상황이 발생..", state);
				set_guide(VOC_WAVE_ID, "ment\\TTS_TimeOut");	 // "현재 통화량이 많아!지연상황이 발생..."
				setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
				return send_guide(NODTMF);
			}
			if (strlen(pScenario->szTTSFile) > 0)
			{
				new_guide();
				char TTSFile[2048 + 1] = { 0x00, };
				sprintf(TTSFile, "%s", pScenario->szTTSFile);
				set_guide(VOC_TTS_ID, TTSFile);	 // "누르신 카드번호는 .. 번입니다."
				set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\input_confirm");
				memset(pScenario->szTTSFile, 0x00, sizeof(pScenario->szTTSFile));// 플래이 직후 삭제 처리
			}
			setPostfunc(POST_DTMF, KICC_CardInput, 3, 0);
			return send_guide(1);
		case 3:
			if (check_validdtmf&& !check_validdtmf(c, "12"))	// 누른 DTMF값이 1,2만 유효하다.
			{
				eprintf("KICC_CardInput [%d]  Card 번호 입력부>확인 부> 절못 누르셨습니다.", state);
				return send_error();
			}
			if (new_guide) new_guide();

			if (c == '1') //예
			{
				// 카드번호는 이미 State 1에서 저장됨
				info_printf(localCh, "KICC_CardInput [%d]  Card 번호 입력부>확인 부> 확인 부>맞습니다.", state);
				eprintf("KICC_CardInput [%d]  Card 번호 입력부>확인 부> 확인 부>맞습니다.", state);
				info_printf(localCh, "[KICC] 카드번호 확인 완료: %s", pScenario->m_CardInfo.Card_Num);

				// ========================================
				// [MODIFIED] DB 사용 시 유효기간 건너뛰고 카드 비밀번호로 이동
				// ========================================
				if (pScenario->m_bUseDbCardInfo) {
					// [NEW] DB에서 유효기간 가져오기
					strncpy(pScenario->m_CardInfo.ExpireDt,
						pScenario->m_szDB_ExpireDate,
						sizeof(pScenario->m_CardInfo.ExpireDt) - 1);

					info_printf(localCh, "[KICC] DB 유효기간 사용: %s", pScenario->m_CardInfo.ExpireDt);
					info_printf(localCh, "[KICC] 유효기간 입력 건너뛰기 (DB 사용)");

					// [MODIFIED] 주민번호 입력 생략 - 바로 카드 비밀번호 입력으로 진행
					// SecretNo 초기화 (빈 값으로 설정)
					memset(pScenario->m_CardInfo.SecretNo, 0x00, sizeof(pScenario->m_CardInfo.SecretNo));
					info_printf(localCh, "[KICC] 주민번호 입력 생략 - 카드 비밀번호 입력으로 이동");
					return KICC_CardPw(0);
				}
				else {
					// [기존] 유효기간 입력 단계로 이동
					return KICC_EffecDate(0);
				}
			}
			else if (c == '2')//아니오
			{
				info_printf(localCh, "KICC_CardInput [%d]  Card 번호 입력부>확인 부> 확인 부>아니오", state);
				eprintf("KICC_CardInput[%d]  Card 번호 입력부>확인 부> 확인 부>아니오", state);

				return KICC_CardInput(0);
			}
		default:
			info_printf(localCh, "KICC_CardInput [%d]  Card 번호 입력부> 시나리오 아이디 오류", state);
			eprintf("KICC_CardInput[%d]  Card 번호 입력부>시나리오 아이디 오류", state);
			set_guide(399);
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
	}

	return 0;
}

int KICC_getOrderInfo(/* [in] */int state)
{
	int		c = 0;
	int(*PrevCall)(int);
	int     localCh = (*lpmt)->chanID;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)((*lpmt)->pScenario);

	if (*lpmt)
	{
		c = *((*lpmt)->dtmfs);
		(*lpmt)->PrevCall = KICC_getOrderInfo;
		(*lpmt)->prevState = state;
	}

	switch (state)
	{
	case 0:
		new_guide();
		info_printf(localCh, "KICC_getOrderInfo [%d] 고객 주문 정보 안내 부", state);
		eprintf("KICC_getOrderInfo [%d] 고객 주문 정보 안내 부", state);
		if (pScenario->m_DBAccess == -1 || pScenario->m_bDnisInfo == -1)
		{
			info_printf(localCh, "KICC_getOrderInfo [%d] 고객 주문 정보 안내 부> 주문 정보 시스템 장애", state);
			eprintf("KICC_getOrderInfo [%d] 고객 주문 정보 안내 부> 주문 정보 시스템 장애", state);
			set_guide(399);
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		}

		if (pScenario->m_bDnisInfo == 0)
		{
			info_printf(localCh, "KICC_getOrderInfo [%d] 고객 주문 정보 안내 부> 존재하지 않은 주문 정보", state);
			eprintf("KICC_getOrderInfo [%d] 고객 주문 정보 안내 부> 존재하지 않은 주문 정보", state);
			new_guide();
			set_guide(VOC_WAVE_ID, "ment/Travelport/no_order_msg");	 // "주문이 접수되지 않았습니다. 상점으로 문의하여 주시기 바랍니다."
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		}

		if (TTS_Play)
		{
			setPostfunc(POST_NET, KICC_getOrderInfo, 1, 0);
			//return TTS_Play((*lpmt)->chanID, 92, "%s 고객님께서는 %s에서, 결제하실 금액은 %d원 입니다.",
			// 2016.05.19
			// 상점명 뺄지를 고려 중
			return TTS_Play((*lpmt)->chanID, 92, "%s 고객님, %s에서 결제하실 금액은 %d원 입니다.",
				pScenario->m_szcust_nm,
				pScenario->m_szterminal_nm,
				/*pScenario->m_szgood_nm,*/
				pScenario->m_namount);
		}
		else
		{
			set_guide(399);
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		}
	case 1:
		info_printf(localCh, "KICC_getOrderInfo [%d] 고객 주문 정보 안내 부> 상품정보 안내(%s:%s:%s:%d)", state,
			pScenario->m_szcust_nm,
			pScenario->m_szterminal_nm,
			pScenario->m_szgood_nm,
			pScenario->m_namount);
		eprintf("KICC_getOrderInfo [%d] 고객 주문 정보 안내 부> 상품정보 안내(%s:%s:%s:%d)> 확인 부", state,
			pScenario->m_szcust_nm,
			pScenario->m_szterminal_nm,
			pScenario->m_szgood_nm,
			pScenario->m_namount);
		if (pScenario->m_TTSAccess == -1)//201701.04
		{
			new_guide();
			info_printf(localCh, "KICC_getOrderInfo [%d] 현재 통화량이 많아!지연상황이 발생..", state);
			eprintf("KICC_getOrderInfo [%d] 현재 통화량이 많아!지연상황이 발생..", state);
			set_guide(VOC_WAVE_ID, "ment\\TTS_TimeOut");	 // "현재 통화량이 많아!지연상황이 발생..."
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		}
		if (strlen(pScenario->szTTSFile) > 0)
		{
			new_guide();
			char TTSFile[2048 + 1] = { 0x00, };
			sprintf(TTSFile, "%s", pScenario->szTTSFile);
			set_guide(VOC_TTS_ID, TTSFile);	 // "주문 상품 정보안내"
			set_guide(VOC_WAVE_ID, "ment\\_common\\common_audio\\input_confirm");
			memset(pScenario->szTTSFile, 0x00, sizeof(pScenario->szTTSFile));// 플래이 직후 삭제 처리
		}
		setPostfunc(POST_DTMF, KICC_getOrderInfo, 2, 0);
		return send_guide(1);
	case 2:
		if (check_validdtmf && !check_validdtmf(c, "12"))	// 누른 DTMF값이 1,2만 유효하다.
		{
			eprintf("KICC_getOrderInfo [%d] 고객 주문 정보 안내 부> 상품정보 안내> 확인 부>잘못 누르셨습니다....", state);
			return send_error();
		}
		new_guide();

		if (c == '1') //예
		{
			info_printf(localCh, "KICC_getOrderInfo[%d] 고객 주문 정보 안내 부>확인 부> 맞습니다.", state);
			eprintf("KICC_getOrderInfo[%d] 고객 주문 정보 안내 부>확인 부> 맞습니다.", state);

			return KICC_CardInput(0);
		}
		else if (c == '2')//아니오
		{
			info_printf(localCh, "KICC_getOrderInfo[%d] 고객 주문 정보 안내 부>확인 부> 아니오", state);
			eprintf("KICC_getOrderInfo[%d] 고객 주문 정보 안내 부>확인 부> 아니오", state);

			if (strcmp(pScenario->szArsType, "ARS") == 0) return KICC_ArsScenarioStart(1);
			else if (strcmp(pScenario->szArsType, "SMS") == 0) return KICC_SMSScenarioStart(1);
			else return pScenario->jobArs(0);
		}
	default:
		info_printf(localCh, "KICC_getOrderInfo [%d]  고객 주문 정보 안내 부> 시나리오 아이디 오류", state);
		eprintf("KICC_getOrderInfo[%d]  고객 주문 정보 안내 부>시나리오 아이디 오류", state);
		set_guide(399);
		setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
		return send_guide(NODTMF);
	}

	return 0;
}

int KICC_ArsScenarioStart(/* [in] */int state)
{
	int		c = 0;
	int(*PrevCall)(int);
	int     localCh = (*lpmt)->chanID;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)((*lpmt)->pScenario);

	if (*lpmt)
	{
		c = *((*lpmt)->dtmfs);
		(*lpmt)->PrevCall = KICC_ArsScenarioStart;
		(*lpmt)->prevState = state;
	}
	//일단 CARD 결제 고정
	memset(pScenario->m_szpaymethod, 0x00, sizeof(pScenario->m_szpaymethod));
	memcpy(pScenario->m_szpaymethod, "CARD", sizeof(pScenario->m_szpaymethod) - 1);

	switch (state)
	{
	case 0:
	{
		char TempPath[1024 + 1] = { 0x00, };
		new_guide();
		info_printf(localCh, "KICC_ArsScenarioStart [%d] 인사말...", state);
		eprintf("KICC_ArsScenarioStart [%d] 인사말", state);
		// sprintf_s(TempPath, sizeof(TempPath), "audio\\shop_intro\\%s", (*lpmt)->dnis);
		// set_guide(VOC_WAVE_ID, TempPath);	 // "인사말"

		// [MODIFIED] 기존: case 1로 이동 → 변경: case 20 (ANI 분기점)으로 이동
		setPostfunc(POST_PLAY, KICC_ArsScenarioStart, 20, 0);
		return send_guide(NODTMF);
	}

	case 1:
		new_guide();
		info_printf(localCh, "KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부...", state);
		eprintf("KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부", state);
		set_guide(VOC_WAVE_ID, "ment\\Travelport\\input_telnum_start");	 // "전화 번호 입력"
		setPostfunc(POST_DTMF, KICC_ArsScenarioStart, 2, 0);
		return send_guide(13);

	case 2:// 전화 번호 입력 처리
		if ((check_validform("*#:7:12", (*lpmt)->refinfo)) < 0)	// 눌린 dtmf값이 8~12의 길이이며 *또는#으로 끝이났다.
		{
			eprintf("KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>잘못 누르셨습니다.....", state);
			return send_error();
		}
		if (strncmp((*lpmt)->dtmfs, "010", 3) != 0 &&
			strncmp((*lpmt)->dtmfs, "011", 3) != 0 &&
			strncmp((*lpmt)->dtmfs, "012", 3) != 0 &&
			strncmp((*lpmt)->dtmfs, "016", 3) != 0 &&
			strncmp((*lpmt)->dtmfs, "017", 3) != 0 &&
			strncmp((*lpmt)->dtmfs, "018", 3) != 0 &&
			strncmp((*lpmt)->dtmfs, "019", 3) != 0)
		{
			eprintf("Nice_ArsScenarioStart [%d] 고객 전화 번호 입력 부(형식 오류)>잘못 누르셨습니다.....", state);
			return send_error();
		}
		new_guide();

#if SKIP_PHONE_CONFIRM
		// [MODIFIED] 전화번호 확인 단계 생략 - 바로 주문조회 진행
		info_printf(localCh, "KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>확인 생략, 주문조회 진행", state);
		eprintf("KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>확인 생략, 주문조회 진행", state);

		// 입력된 전화번호 저장
		if (((CKICC_Scenario *)(*lpmt)->pScenario)->m_Myport == (*lpmt))
		{
			memset(pScenario->m_szInputTel, 0x00, sizeof(pScenario->m_szInputTel));
			memcpy(pScenario->m_szInputTel, (*lpmt)->refinfo, sizeof(pScenario->m_szInputTel) - 1);
		}

		// 다중 주문 모드 활성화
		pScenario->m_bMultiOrderMode = TRUE;

		// [MODIFIED] 주문조회 먼저 진행 (삼자통화 안내는 조회 성공 후)
		setPostfunc(POST_NET, KICC_getMultiOrderInfo, 0, 0);
		return getMultiOrderInfo_host(90);
#else
		// 기존 코드: TTS로 전화번호 확인 후 1/2번 선택
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
#endif

#if !SKIP_PHONE_CONFIRM
	// [SKIP_PHONE_CONFIRM] 전화번호 확인 단계 생략 시 아래 case 3, 4는 사용되지 않음
	case 3:
		if (pScenario->m_TTSAccess == -1)//201701.04
		{
			new_guide();
			info_printf(localCh, "KICC_ArsScenarioStart [%d] 현재 통화량이 많아!지연상황이 발생..", state);
			eprintf("KICC_ArsScenarioStart [%d] 현재 통화량이 많아!지연상황이 발생..", state);
			set_guide(VOC_WAVE_ID, "ment\\TTS_TimeOut");	 // "현재 통화량이 많아!지연상황이 발생..."
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		}
		if (strlen(pScenario->szTTSFile) > 0)
		{
			new_guide();
			char TTSFile[2048 + 1] = { 0x00, };
			sprintf(TTSFile, "%s", pScenario->szTTSFile);

			set_guide(VOC_TTS_ID, TTSFile);	 // 주문번호 확일을 위해 주문 번호 재생
			set_guide(VOC_WAVE_ID, "ment/_common/common_audio/input_confirm");
			memset(pScenario->szTTSFile, 0x00, sizeof(pScenario->szTTSFile));// 플래이 직후 삭제 처리
		}
		setPostfunc(POST_DTMF, KICC_ArsScenarioStart, 4, 0);
		return send_guide(1);
	case 4:
		if (check_validdtmf && !check_validdtmf(c, "12"))	// 누른 DTMF값이 1,2만 유효하다.
		{
			eprintf("KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>확인 부> 확인 부>잘못 누르셨습니다....", state);
			return send_error();
		}
		new_guide();

		if (c == '1') //예
		{
			if (((CKICC_Scenario *)(*lpmt)->pScenario)->m_Myport == (*lpmt))
			{
				memset(pScenario->m_szInputTel, 0x00, sizeof(pScenario->m_szInputTel));
				memcpy(pScenario->m_szInputTel, (*lpmt)->refinfo, sizeof(pScenario->m_szInputTel) - 1);
			}

			info_printf(localCh, "KICC_ArsScenarioStart[%d] 고객 전화 번호 입력 부>확인 부> 확인 부>맞습니다.", state);
			eprintf("KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>확인 부> 확인 부>맞습니다.", state);

			// [다중 주문 지원] 다중 주문 모드 활성화
			pScenario->m_bMultiOrderMode = TRUE;

			// [MODIFIED] 주문조회 먼저 진행 (삼자통화 안내는 조회 성공 후)
			setPostfunc(POST_NET, KICC_getMultiOrderInfo, 0, 0);
			return getMultiOrderInfo_host(90);
		}
		else if (c == '2')//아니오
		{
			info_printf(localCh, "KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>확인 부> 확인 부>아니오", state);
			eprintf("KICC_ArsScenarioStart [%d] 고객 전화 번호 입력 부>확인 부> 확인 부>아니오", state);

			return KICC_ArsScenarioStart(1);
		}
#endif  // !SKIP_PHONE_CONFIRM

	// [NEW] 삼자통화/호전환 안내 멘트
	case 10:
		new_guide();
		info_printf(localCh, "KICC_ArsScenarioStart [%d] 삼자통화 안내 멘트 재생", state);
		eprintf("KICC_ArsScenarioStart [%d] 삼자통화 안내: 삼초 이내에 삼자 통화 또는 호전환하시기 바랍니다", state);
		set_guide(VOC_WAVE_ID, "ment\\Travelport\\transfer_guide");  // "삼초 이내에 삼자 통화 또는 호전환하시기 바랍니다"
		setPostfunc(POST_PLAY, KICC_ArsScenarioStart, 11, 0);
		return send_guide(NODTMF);

	// [NEW] 3초 대기 - POST_TIME 타이머 사용 (음성 파일 불필요)
	case 11:
		info_printf(localCh, "KICC_ArsScenarioStart [%d] 3초 대기 중...", state);
		eprintf("KICC_ArsScenarioStart [%d] 3초 대기 (삼자통화/호전환 대기) - POST_TIME 사용", state);
		// POST_TIME: 3초 후 Case 12로 진행 (음성 파일 없이 타이머 대기)
		setPostfunc(POST_TIME, KICC_ArsScenarioStart, 12, 3);
		return 0;  // 타이머 대기 상태로 전환

	// [NEW] 시스템 인사말 후 결제 안내 진행
	case 12:
		new_guide();
		info_printf(localCh, "KICC_ArsScenarioStart [%d] 시스템 인사말 재생", state);
		eprintf("KICC_ArsScenarioStart [%d] 인사말: 안녕하세요? 항공권 인증 결재 시스템입니다", state);
		set_guide(VOC_WAVE_ID, "ment\\Travelport\\intro");  // "안녕하세요? 항공권 인증 결재 시스템입니다"
		// [MODIFIED] 인사말 재생 후 결제 안내로 진행 (주문조회는 이미 완료됨)
		setPostfunc(POST_PLAY, KICC_AnnounceMultiOrders, 0, 0);
		return send_guide(NODTMF);

	// ========================================
	// [2025-12-22 NEW] 발신번호 자동 조회 분기점
	// ========================================
	case 20:
	{
		// 발신번호 조회 조건 확인
		// 1. 아직 발신번호 조회를 시도하지 않았는가? (m_bAniChecked == FALSE)
		// 2. 발신번호가 유효한 휴대폰 번호인가?

		if (!pScenario->m_bAniChecked && IsValidMobileNumber(pScenario->szAni))
		{
			info_printf(localCh, "KICC_ArsScenarioStart [%d] 발신번호 자동 조회 시도", state);
			eprintf("KICC_ArsScenarioStart [%d] 발신번호 자동 조회 시도: ANI=%s", state, pScenario->szAni);

			// 발신번호를 입력 전화번호로 설정
			memset(pScenario->m_szInputTel, 0x00, sizeof(pScenario->m_szInputTel));
			strncpy(pScenario->m_szInputTel, pScenario->szAni, sizeof(pScenario->m_szInputTel) - 1);

			// 발신번호 조회 시도 플래그 설정 (이후 다시 시도하지 않음)
			pScenario->m_bAniChecked = TRUE;

			// 다중 주문 모드 활성화
			pScenario->m_bMultiOrderMode = TRUE;

			// 주문조회 진행 → case 21로 결과 처리
			setPostfunc(POST_NET, KICC_ArsScenarioStart, 21, 0);
			return getMultiOrderInfo_host(90);
		}
		else
		{
			// 발신번호 조회 불가 → 기존 전화번호 입력 단계로 이동
			if (pScenario->m_bAniChecked)
			{
				eprintf("KICC_ArsScenarioStart [%d] 발신번호 이미 조회 시도됨 → 전화번호 입력", state);
			}
			else
			{
				eprintf("KICC_ArsScenarioStart [%d] 발신번호 유효하지 않음 (ANI=%s) → 전화번호 입력",
					state, pScenario->szAni);
			}

			return KICC_ArsScenarioStart(1);  // 휴대폰 번호 입력 state
		}
	}

	// ========================================
	// [2025-12-22 NEW] 발신번호 기반 주문조회 결과 처리
	// ========================================
	case 21:
	{
		if (pScenario->m_DBAccess == -1 || pScenario->m_MultiOrders.nOrderCount == 0)
		{
			// 주문 없음 또는 DB 오류 → 휴대폰 번호 입력으로 이동
			// [IMPORTANT] ANI 자동 조회 실패 시에는 "주문정보가 없습니다" 멘트 없이
			// 바로 휴대폰 번호 입력 단계로 이동 (사용자 경험 개선)
			info_printf(localCh, "KICC_ArsScenarioStart [%d] ANI 주문조회 실패/없음", state);
			eprintf("KICC_ArsScenarioStart [%d] ANI 기반 주문조회 실패/없음 → 전화번호 입력 (멘트 없음)", state);

			// 멘트 없이 바로 휴대폰 번호 입력으로 이동
			return KICC_ArsScenarioStart(1);
		}

		// 주문조회 성공 → 호전환/인사말 흐름으로 진행
		info_printf(localCh, "KICC_ArsScenarioStart [%d] ANI 주문조회 성공: %d건", state, pScenario->m_MultiOrders.nOrderCount);
		eprintf("KICC_ArsScenarioStart [%d] ANI 기반 주문조회 성공: %d건, request_type=%s",
			state, pScenario->m_MultiOrders.nOrderCount, pScenario->m_szRequestType);

		// request_type에 따른 분기 (기존 KICC_getMultiOrderInfo의 로직과 동일)
		// ※ 호전환 안내(case 10) → 3초 대기(case 11) → 인사말(case 12) 흐름 유지
		if (strcmp(pScenario->m_szRequestType, "SMS") == 0 ||
			strcmp(pScenario->m_szRequestType, "TKT") == 0)
		{
			// SMS/TKT: 호전환 생략 → 바로 인사말 → 결제안내
			eprintf("KICC_ArsScenarioStart [%d] → 시스템 인사말 (case 12) [request_type=%s]",
				state, pScenario->m_szRequestType);
			return KICC_ArsScenarioStart(12);
		}
		else
		{
			// ARS: 호전환 안내 → 3초 대기 → 인사말 → 결제안내
			eprintf("KICC_ArsScenarioStart [%d] → 호전환 안내 (case 10) [request_type=%s]",
				state, strlen(pScenario->m_szRequestType) > 0 ? pScenario->m_szRequestType : "ARS(default)");
			return KICC_ArsScenarioStart(10);
		}
	}

	case 0xffff:
		return  goto_hookon();
	default:
		info_printf(localCh, "KICC_ArsScenarioStart [%d]  C고객 전화 번호 입력 부> 시나리오 아이디 오류", state);
		eprintf("KICC_ArsScenarioStart[%d]  고객 전화 번호 입력 부>시나리오 아이디 오류", state);
		set_guide(399);
		setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
		return send_guide(NODTMF);
	}
	return 0;
}

int KICC_SMSScenarioStart(/* [in] */int state)
{
	int		c = 0;
	int(*PrevCall)(int);
	int     localCh = (*lpmt)->chanID;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)((*lpmt)->pScenario);

	if (*lpmt)
	{
		c = *((*lpmt)->dtmfs);
		(*lpmt)->PrevCall = KICC_SMSScenarioStart;
		(*lpmt)->prevState = state;
	}
	//일단 CARD 결제 고정
	memset(pScenario->m_szpaymethod, 0x00, sizeof(pScenario->m_szpaymethod));
	memcpy(pScenario->m_szpaymethod, "CARD", sizeof(pScenario->m_szpaymethod) - 1);

	switch (state)
	{
	case 0:
	{
		char TempPath[1024 + 1] = { 0x00, };
		new_guide();
		info_printf(localCh, "KICC_SMSScenarioStart [%d] 인사말...", state);
		eprintf("KICC_SMSScenarioStart [%d] 인사말", state);
		// sprintf_s(TempPath, sizeof(TempPath), "audio\\shop_intro\\%s", (*lpmt)->dnis);
		// set_guide(VOC_WAVE_ID, TempPath);	 // "인사말"
		setPostfunc(POST_PLAY, KICC_SMSScenarioStart, 1, 0);
		return send_guide(NODTMF);
	}
	case 1:
		new_guide();
		info_printf(localCh, "KICC_SMSScenarioStart [%d] SMS로 받은 주문 번호 입력 부...", state);
		eprintf("KICC_SMSScenarioStart [%d] SMS로 받은 주문 번호 입력 부", state);
		set_guide(VOC_WAVE_ID, "audio/input_sms_start");	 // "SMS로 받은 주문 번호 입력"
		setPostfunc(POST_DTMF, KICC_SMSScenarioStart, 2, 0);
		return send_guide(6);

	case 2:// 전화 번호 입력 처리
		if ((check_validkey("1234567890")) < 0)
		{
			eprintf("KICC_EffecDate [%d] 유효 기간 입력 부>잘못 누르셨습니다.....", state);
			return send_error();
		}

		new_guide();

		info_printf(localCh, "KICC_SMSScenarioStart [%d] SMS로 받은 주문 번호 입력 부>확인 부", state);
		eprintf("KICC_SMSScenarioStart [%d]SMS로 받은 주문 번호 입력 부>확인 부", state);

		memset(pScenario->m_szAuth_no, 0x00, sizeof(pScenario->m_szAuth_no));
		memcpy(pScenario->m_szAuth_no, (*lpmt)->dtmfs, sizeof(pScenario->m_szAuth_no) - 1);
		
		if (TTS_Play)
		{
			setPostfunc(POST_NET, KICC_SMSScenarioStart, 3, 0);
			return TTS_Play((*lpmt)->chanID, 92, "%c,%c,%c,%c,%c,%c, 번 입니다.",
				(*lpmt)->dtmfs[0],
				(*lpmt)->dtmfs[1],
				(*lpmt)->dtmfs[2],
				(*lpmt)->dtmfs[3],
				(*lpmt)->dtmfs[4],
				(*lpmt)->dtmfs[5]);
		}
		else
		{
			set_guide(399);
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		}
	case 3:
		if (pScenario->m_TTSAccess == -1)//201701.04
		{
			new_guide();
			info_printf(localCh, "KICC_SMSScenarioStart [%d] 현재 통화량이 많아!지연상황이 발생..", state);
			eprintf("KICC_SMSScenarioStart [%d] 현재 통화량이 많아!지연상황이 발생..", state);
			set_guide(VOC_WAVE_ID, "ment\\TTS_TimeOut");	 // "현재 통화량이 많아!지연상황이 발생..."
			setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
			return send_guide(NODTMF);
		}
		if (strlen(pScenario->szTTSFile) > 0)
		{
			new_guide();
			char TTSFile[2048 + 1] = { 0x00, };
			new_guide();

			set_guide(VOC_WAVE_ID, "audio/input_sms_msg"); //입력 하신 주문 번호는 
			set_guide(VOC_TTS_ID, TTSFile);	 // 주문번호 확일을 위해 주문 번호 재생
			set_guide(VOC_WAVE_ID, "ment/_common/common_audio/input_confirm");
			memset(pScenario->szTTSFile, 0x00, sizeof(pScenario->szTTSFile));// 플래이 직후 삭제 처리
		}
		setPostfunc(POST_DTMF, KICC_SMSScenarioStart, 4, 0);
		return send_guide(1);
	case 4:
		if (!check_validdtmf(c, "12"))	// 누른 DTMF값이 1,2만 유효하다.
		{
			eprintf("KICC_SMSScenarioStart [%d] SMS로 받은 주문 번호 입력 부>확인 부> 확인 부>잘못 누르셨습니다....", state);
			return send_error();
		}
		new_guide();

		if (c == '1') //예
		{
			info_printf(localCh, "KICC_SMSScenarioStart[%d] SMS로 받은 주문 번호 입력 부>확인 부> 확인 부>맞습니다.", state);
			eprintf("KICC_SMSScenarioStart [%d] SMS로 받은 주문 번호 입력 부>확인 부> 확인 부>맞습니다.", state);

			setPostfunc(POST_NET, KICC_getOrderInfo, 0, 0);
			return getSMSOrderInfo_host(90);
		}
		else if (c == '2')//아니오
		{
			info_printf(localCh, "KICC_SMSScenarioStart [%d] SMS로 받은 주문 번호 입력 부>확인 부> 확인 부>아니오", state);
			eprintf("KICC_SMSScenarioStart [%d] SMS로 받은 주문 번호 입력 부>확인 부> 확인 부>아니오", state);

			return KICC_SMSScenarioStart(1);
		}

	case 0xffff:
		return  goto_hookon();
	default:
		info_printf(localCh, "KICC_SMSScenarioStart [%d]  SMS로 받은 주문 번호 입력 부> 시나리오 아이디 오류", state);
		eprintf("KICC_SMSScenarioStart[%d]  SMS로 받은 주문 번호 입력 부>시나리오 아이디 오류", state);
		set_guide(399);
		setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
		return send_guide(NODTMF);
	}
	return 0;
}

CKICC_Scenario::CKICC_Scenario()
{
	m_AdoDb = NULL;
	m_Myport = NULL;
	memset(szDnis, 0x00, sizeof(szDnis));
	memset(szAni, 0x00, sizeof(szAni));
	memset(szArsType, 0x00, sizeof(szArsType));
	memset(szSessionKey, 0x00, sizeof(szSessionKey));
	nChan = 0;

	// 다중 주문 지원 초기화
	memset(&m_MultiOrders, 0x00, sizeof(MultiOrderInfo));
	m_nCurrentOrderIdx = 0;
	m_bMultiOrderMode = FALSE;
	memset(m_szAuthNo, 0x00, sizeof(m_szAuthNo));
	memset(m_szRequestType, 0x00, sizeof(m_szRequestType));  // [NEW] request_type 초기화

	// [2025-12-22 NEW] 발신번호 조회 플래그 초기화
	m_bAniChecked = FALSE;
}

CKICC_Scenario::~CKICC_Scenario()
{
	if (m_hThread)
	{
		eprintf("CKICC_Scenario DB Access 동작 중....");
		::WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
		eprintf("CKICC_Scenario DB Access 중지....");
	}
	if (m_hPayThread)
	{
		eprintf("CKICC_Scenario PAYMENT 동작 중....");
		::WaitForSingleObject(m_hPayThread, INFINITE);
		CloseHandle(m_hPayThread);
		m_hPayThread = NULL;
		eprintf("CKICC_Scenario PAYMENT 중지....");
	}
	if (m_Myport) m_Myport->ppftbl[POST_NET].postcode = HI_OK;
}

int CKICC_Scenario::ScenarioInit(LPMTP *Port, char *ArsType)
{
	(*lpmt)->pScenario = (void *)this;
	nChan = Port->chanID;
	m_Myport = Port;
	//동기화 개체 
	strncpy(szDnis, m_Myport->dnis, sizeof(szDnis) - 1);
	strncpy(szAni, m_Myport->ani, sizeof(szAni) - 1);
	strncpy(szArsType, ArsType, sizeof(szArsType) - 1);
	strncpy(szSessionKey, m_Myport->Session_Key, sizeof(szSessionKey) - 1);

	// ========================================
	// [2025-11-21 NEW] DB 기반 카드정보 필드 초기화
	// ========================================
	memset(m_szDB_CardPrefix, 0x00, sizeof(m_szDB_CardPrefix));
	memset(m_szDB_ExpireDate, 0x00, sizeof(m_szDB_ExpireDate));
	memset(m_szDB_InstallPeriod, 0x00, sizeof(m_szDB_InstallPeriod));
	m_bUseDbCardInfo = 1;  // 기본값: DB 사용 (1:TRUE, 0:FALSE로 변경하면 기존 방식)

	// ========================================
	// [2025-12-15 NEW] 휴대폰 번호 재시도 카운트 초기화
	// ========================================
	m_nPhoneRetryCount = 0;

	// ========================================
	// [2025-12-17 NEW] 카드번호 재입력 카운트 초기화
	// ========================================
	m_nCardRetryCount = 0;

	// ========================================
	// [2025-12-22 NEW] 발신번호 조회 플래그 초기화
	// ========================================
	m_bAniChecked = FALSE;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////
// 모든 시나리의 시작 부
//
int CKICC_Scenario::jobArs(/* [in] */int state)
{
	// 이후 객체 함수 이용에 제약이 따르므로 
	// 자체 동기 모듈이 요구 된다.
	// 별도의 DLL이므로 
	//IScenario_enter_handler();
	//curyport = m_Myport;
	//curyport->pScenario = (void *)this;
	(*lpmt)->pScenario = (void *)this;
	(*lpmt)->Myexit_service = KICC_ExitSvc;
	(*lpmt)->MyStartState = 10;
	memset(&m_CardInfo, 0x00, sizeof(m_CardInfo));

	if (strcmp(szArsType, "ARS") == 0) return KICC_ArsScenarioStart(0);
	else if (strcmp(szArsType, "SMS") == 0) return KICC_SMSScenarioStart(0);

	return KICC_jobArs(0);
}


#include <Windows.h>                    /**< Using Windows Data Type       */
//#include <crtdbg.h>                     /**< Using _CrtReportMode() Option */
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void InvalidParameterHandler(const wchar_t *expression, const wchar_t * function, const wchar_t * file,
	unsigned int line, uintptr_t pReserved);

_invalid_parameter_handler newHandler;
_invalid_parameter_handler oldHandler;


extern "C" BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		
		errno_t result = 0;
		/**< CRT가 Assertion 다이얼 로그를 출력하지 못하게 함 */
		_CrtSetReportMode(_CRT_ASSERT, 0);


		/**< 사용자 정의 핸들러 등록 */
		newHandler = InvalidParameterHandler;
		oldHandler = _set_invalid_parameter_handler(newHandler);

		//_CrtSetBreakAlloc(9686);
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

		Kicc_Install();
		break;
	}
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		Kicc_UnInstall();
		break;
	}
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                      
//// @param     [in]    expression : CRT 함수 내에서 발생한 테스트 실패에 대한 설명 문자열   
//// [in]    function   : 실패를 발생시킨 CRT 함수 이름                                     
//// [in]    file       : 소스 파일 이름                                                   
//// [in]    line       : 에러가 발생한 소스 코드의 행 번호                                 
//// [out]   pReserved  : 예약어                                                          
//// @return    void                                                                      
//// @note      사용자 정의한 유효 파라미터 핸들러
//// @note      이 핸들러는 CRT가 유효 파라미터 검사를 수행할 때 기본 핸들러 대신 수행
/////////////////////////////////////////////////////////////////////////////////////////////
void InvalidParameterHandler(const wchar_t *expression, const wchar_t *function, const wchar_t * file,
	unsigned int line, uintptr_t pReserved)
{
	printf("Invalid parameter detected in function %s.\n", function);
	printf("File: %s Line: %d\n", file, line);
	printf("Expression: %s\n", expression);
	exit(EXIT_FAILURE);
}
