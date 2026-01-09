/******************************************************************************
* 파일명 : KICCpayMent.cpp
* 작성일 : 2015.07.31
* 작성자 : 최영락
******************************************************************************/

#include	"stdafx.h"
#include    "CommonDef.H"
#include    "KICC_Common.h"
#include    "Scenaio.h"
#include    "ADODB.h"
#include    "KICC_Travelport_Scenario.h"

#include <string.h> // strcpy_s() strlen()

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>


#include <stdio.h>
#include <iprtrmib.h>
#include <tlhelp32.h>
#include <iphlpapi.h>
#include <afxinet.h>

#pragma comment(lib, "Iphlpapi.lib")

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define EQUAL 0

extern char g_strMid[50];
extern char g_strLicenekey[100];
extern char g_strCancelPwd[100];
extern char gNiceDebug[10 + 1];
extern char gNiceLog[50 + 1];

extern void(*eprintf)(const char *str, ...);
extern LPMTP **lpmt , **port;

//LPMTP	*curyport=NULL;
extern void(*info_printf)(int chan, const char *str, ...);
extern void(*new_guide)(void);
extern int(*set_guide)(int vid, ...);
extern void(*setPostfunc)(int type, int(*func)(int), int poststate, int wtime);
extern int(*send_guide)(int mode);
extern int(*goto_hookon)(void);
extern int(*check_validform)(char *form, char *data);
extern int(*send_error)(void);
extern int(*check_validdtmf)(int c, char *vkeys);
extern int(*in_multifunc)(int chan);
extern int(*quitchan)(int chan);

extern int(*atoi2)(char *p);
extern int(*atoi3)(char *p);
extern int(*atoi4)(char *p);
extern int(*atoiN)(char *p, int n);
extern long(*atolN)(char *p, int n);
extern int(*atox)(char *s);

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

// #define	PARAINI		".\\KiccPay_Travelport_Test_para.ini"
#define	PARAINI		".\\KiccPay_Travelport_para.ini"
#define MAXCHAN 	240		// 최대 회선 수
//#define MAXCHAN 	120		// 최대 회선 수


#define TRUE		1		// 참
#define FALSE		0		// 거짓

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

#define     KICC_CLIENT_IP "211.196.157.123"
//#define     KICC_MAII_ID   "T5102001"
//#define     KICC_GW_URL    "testgw.easypay.co.kr"
#define     KICC_MAII_ID   "05593362"
#define     KICC_GW_URL    "gw.easypay.co.kr"
#define     KICC_GW_PORT   "80"
#define     KICC__CERT_FILE "./cert/pg_cert.pem"
#define     KICC_LOG        "./KiccLog"
#define     KICC_LOG_LV     1
#define     KICC_ADMIN      "ARS"

/* ------------------------------------------------------------------------ */
/* :::::   struct - config_info   ::::::::::::::::::::::::::::::::::::::::: */
/* ------------------------------------------------------------------------ */
typedef  struct  tagS_CFG_I                                        S_CFG_I;
typedef  struct  tagS_CFG_I*                                      PS_CFG_I;

struct  tagS_CFG_I
{
	const   char*  pszLogDir;
	const   char*  pszKeyDir;

	const   char*  pszMall_ID;
	const   char*  pszSite_Key;

	const   char*  pszTr_CD;

	const   char*  pszPAURL;
	const   char*  pszPAPorts;

	const   char*  pszOrdr_No;

	const   char*  pszPlanData;
	const   char*  pszEncData;
	const   char*  pszSndKey;
	const   char*  pszTraceNo;
	const   char*  pszCust_IP;

	int     nLogLevel;
	int     nOpt;
};
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/* :::::   typedef   :::::::::::::::::::::::::::::::::::::::::::::::::::::: */
/* ------------------------------------------------------------------------ */
typedef int(*lfEP_CLI_DLL__init)(char*, char*, char*, char*, int, PS_CFG_I);
typedef int(*lfEP_CLI_DLL__set_plan_data)(char*, PS_CFG_I);
typedef int(*lfEP_CLI_DLL__set_entry)(const char*, char* const, int);
typedef int(*lfEP_CLI_DLL__set_delim)(unsigned  char, char*, int);
typedef int(*lfEP_CLI_DLL__set_value)(char*, char*, unsigned  char, char*, int);
typedef int(*lfEP_CLI_DLL__proc)(char*, char*, char*, char*, PS_CFG_I, char*, int);
typedef int(*lfEP_CLI_DLL__get_value)(char*, char*, char*, int);
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/* :::::   define   ::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
/* ------------------------------------------------------------------------ */
#define    M_EP_PK__DELI__FS__C                                  ( '\x1c' )
#define    M_EP_PK__DELI__GS__C                                  ( '\x1d' )
#define    M_EP_PK__DELI__RS__C                                  ( '\x1e' )
#define    M_EP_PK__DELI__US__C                                  ( '\x1f' )

#define    TRAN_CD_NOR_PAYMENT									 ( "00101000" ) // 승인
#define    TRAN_CD_NOR_MGR										 ( "00201000" ) // 변경
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

lfEP_CLI_DLL__init          lplfEP_CLI_DLL__init = NULL;
lfEP_CLI_DLL__set_plan_data lplfEP_CLI_DLL__set_plan_data = NULL;
lfEP_CLI_DLL__set_entry     lplfEP_CLI_DLL__set_entry = NULL;
lfEP_CLI_DLL__set_delim     lplfEP_CLI_DLL__set_delim = NULL;
lfEP_CLI_DLL__set_value     lplfEP_CLI_DLL__set_value = NULL;
lfEP_CLI_DLL__proc          lplfEP_CLI_DLL__proc = NULL;
lfEP_CLI_DLL__get_value     lplfEP_CLI_DLL__get_value = NULL;

HINSTANCE    g_KicchDll;
/* ------------------------------------------------------------------------ */

int Kicc_Install()
{

	g_KicchDll = LoadLibrary("ep_cli_dll.dll");

	if (g_KicchDll == NULL)
	{
		eprintf("LoadLibrary null");
		return -1;
	}

	/* ------------------------------------------------------------------------ */

	lplfEP_CLI_DLL__init = (lfEP_CLI_DLL__init)GetProcAddress(g_KicchDll, "lfEP_CLI_DLL__init");

	if (lplfEP_CLI_DLL__init == NULL)
	{
		eprintf("lplfEP_CLI_DLL__init null");
		return -1;
	}

	lplfEP_CLI_DLL__set_plan_data = (lfEP_CLI_DLL__set_plan_data)GetProcAddress(g_KicchDll, "lfEP_CLI_DLL__set_plan_data");

	if (lplfEP_CLI_DLL__set_plan_data == NULL)
	{
		eprintf("lplfEP_CLI_DLL__set_plan_data null");
		return -1;
	}

	lplfEP_CLI_DLL__set_entry = (lfEP_CLI_DLL__set_entry)GetProcAddress(g_KicchDll, "lfEP_CLI_DLL__set_entry");

	if (lplfEP_CLI_DLL__set_entry == NULL)
	{
		eprintf("lplfEP_CLI_DLL__set_entry null");
		return -1;
	}

	lplfEP_CLI_DLL__set_delim = (lfEP_CLI_DLL__set_delim)GetProcAddress(g_KicchDll, "lfEP_CLI_DLL__set_delim");

	if (lplfEP_CLI_DLL__set_delim == NULL)
	{
		eprintf("lplfEP_CLI_DLL__set_delim null");
		return -1;
	}

	lplfEP_CLI_DLL__set_value = (lfEP_CLI_DLL__set_value)GetProcAddress(g_KicchDll, "lfEP_CLI_DLL__set_value");

	if (lplfEP_CLI_DLL__set_value == NULL)
	{
		eprintf("lplfEP_CLI_DLL__set_value null");
		return -1;
	}

	lplfEP_CLI_DLL__proc = (lfEP_CLI_DLL__proc)GetProcAddress(g_KicchDll, "lfEP_CLI_DLL__proc");

	if (lplfEP_CLI_DLL__proc == NULL)
	{
		eprintf("lplfEP_CLI_DLL__proc null");
		return -1;
	}

	lplfEP_CLI_DLL__get_value = (lfEP_CLI_DLL__get_value)GetProcAddress(g_KicchDll, "lfEP_CLI_DLL__get_value");

	if (lplfEP_CLI_DLL__get_value == NULL)
	{
		eprintf("lplfEP_CLI_DLL__get_value null");
		return -1;
	}

	return 0;
}

int Kicc_UnInstall()
{
	FreeLibrary(g_KicchDll);
	return 0;
}


/* ------------------------------------------------------------------------ */
void KiccNone_Pay_Quithostio(char *p, int ch)
{
	eprintf("PAYquithostio===START");
	if (!in_multifunc(ch))
	{
		Sleep(500);
	}
	(*port)[ch].ppftbl[POST_NET].postcode = HI_OK;
	if ((*port)[ch].used != L_IDLE && in_multifunc(ch))
		quitchan(ch);

	eprintf("%s", p);
	eprintf("PAYquithostio _endthread");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 함수명 : GetValidIP
// 인  자 : char ** szText
// 설  명 : 지정된 문자열로부터 IP 주소를 획득 한다.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetValidIP(char ** szText)
{
	char *p, *s;
	int n;

	p = s = *szText;

	// 1. 지정된 모든 문자열과 일치하지 않는 최초의 인덱스 획득한다.
	//   - 해당 인덱스의 전제조건은 다음에 의거하여, 획득 한다.
	//     > IP 주소 구성의 최소는 0.0.0.0으로 최소 7자리 이상이다.
	// 1.1 따라서 숫자와 '.'과 일치하지 않는 최초의 인덱스가 당 문자열
	//     시작으로부터 7이상(7자리)이상이면 IP 주소일 가능성 있다.
	n = strspn(p, "1234567890.");
	if (n < 7) {
		*szText = p + n;
		return 0;
	}

	// 2. IP 주소일 가능성있으면, 다음에 의해 IP 주소인지를 검증 한다.
	for (int i = 0; i < 4; i++) {
		// 2.1. 숫자열이 아닐경우 해당 인덱스가 최소 1~최대 3이 아니면
		// 오류 처리한다.(1 보다 작거나 3보다 크면 오류 처리)
		n = strspn(p, "1234567890");
		if ((n < 1) || (n > 3)) {
			*szText = p + n;
			return 0;
		}
		// 2.2. 숫자열이 아닐경우이면서 1~3인 경우
		// 2.2.1. 해당 인덱스 다음으로 포인터 이동
		p += n;
		// 2.2.2 '.'인 경우 다음 포인터로 이동
		if (*p == '.') p++;
		else  {
			// 2.2.3. '.' 아닌 경우, 모든 아이피 구성원인 경우 다음을 처리
			if (i == 3) {
				// 2.2.3.1. 마지막인 '.'인 경우 오류 처리, 그외의 경우 널 종료하도록 조치, 그외는 오류 처리
				if (*p == '.') {
					*szText = p + 1;
					return 0;
				}
				else {
					*p = '\0';//그외의 경우 널 종료하도록 조치
				}
			}
			else
			{
				*szText = p + 1;
				return 0;
			}
		}

	}
	*szText = p;
	return inet_addr(s);// 네트원크 주소로 전환한다,
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 함수명 : FindWebAddresss
// 인  자 : char * zAuthLine(OUTPUT)
// 설  명 : 자신의 IP 주소(외부 공인)를 획득 하여, zAuthLine에 탑재 한다.
//          ipconfig.kr 접속하면 자신의 IP 주소를 HTML로 리턴 되는 것에 착안하였다.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int FindWebAddresss(int ch, char * szAuthLine)
{
	DWORD dwRet = 0;
	BOOL bResult;
	char szTemp[256] = { 0x00, };
	struct in_addr inAddr;
	CString sAddr, WebIP;
	DWORD dwIPNr;

	// 지정 사이트로 접속을 한다.
	CInternetSession *pSession = new CInternetSession(_T("GetIPNr"), 1, INTERNET_OPEN_TYPE_PRECONFIG);
	CHttpConnection* pConnection = pSession->GetHttpConnection(_T("ipconfig.kr"));

	CHttpFile* pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_GET, _T("/"));

	if (szAuthLine[0])               // Userid/ password specified?
	{
		eprintf("FindWebAddresss>szAuthLine (%s)", szAuthLine);
		pFile->AddRequestHeaders(szAuthLine, HTTP_ADDREQ_FLAG_ADD, strlen(szAuthLine));
	}


	try
	{
		// 페이지를 요청한다.
		eprintf("FindWebAddresss>pFile->SendRequest(NULL)");
		bResult = pFile->SendRequest(NULL);
	}
	catch (CInternetException* pEx)
	{
		eprintf("FindWebAddresss>Exception");

		pEx->Delete();

		pFile->Close();
		delete pFile;

		pConnection->Close();
		delete pConnection;

		delete pSession;

		return 0;
	}

	eprintf("FindWebAddresss>QueryInfoStatusCode");
	// 요청 된 결과 페이지를 쿼리한다.
	pFile->QueryInfoStatusCode(dwRet);

	if (dwRet / 100 == 2) {
		char *p = new char[1024 + 1];
		char *q = p;
		memset(p, 0x00, 1024 + 1);
		pFile->Read(p, 1024);// 해당 리턴된 URL를 읽어드린다.
		eprintf("FindWebAddresss>pFile->Read(p:%s, 1024) ", p);

		dwIPNr = 0;
		while (*p) {
			int n = strcspn(p, "1234567890");// 숫자열의 인덱스를 획득 한다.
			// IP일 가능성을 체크한다.
			p += n;
			dwIPNr = GetValidIP(&p);
			if (dwIPNr) {
				//IP 주소인 경우
				memmove(&inAddr, &dwIPNr, 4);
				sAddr = inet_ntoa(inAddr);
				sprintf_s(szTemp, sizeof(szTemp), "%s", sAddr);
				eprintf("FindWebAddresss>sprintf_s(szTemp:%s, sAddr:%s);", szTemp, sAddr);
				WebIP = szTemp;
			}
		}
		delete q;
	}
	pFile->Close();
	delete pFile;

	pConnection->Close();
	delete pConnection;
	delete pSession;

	// 자신의 아이피를 탑재한다.
	memset(szAuthLine, 0x00, 16);
	if (strlen(szTemp)>1 && strlen(szTemp)<16)
		memcpy(szAuthLine, szTemp, strlen(szTemp));

	eprintf("FindWebAddresss>memcpy(szAuthLine:%s, szTemp:%s, strlen(szTemp))", szAuthLine, szTemp);

	eprintf("FindWebAddresss>return : %d", dwRet);
	return dwRet;
}

// 결제 요청 쓰레드용 함수
unsigned int __stdcall KiccArsPayProcess(void *data)
{
	int			ch;
	int			paythreadID;
	char        AnsiNO_Char[257] = { 0x00, };
	LPMTP		*lineTablePtr = (LPMTP *)data;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)(lineTablePtr->pScenario);

	ch = lineTablePtr->chanID;
	paythreadID = pScenario->paythreadID;

	pScenario->m_PaySysCd = 0;
	if (paythreadID != pScenario->paythreadID) {
		pScenario->m_PaySysCd = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		KiccNone_Pay_Quithostio("KiccArsPayProcess the line service is not valid any more.", ch);
		eprintf("KiccArsPayProcess END");
		_endthreadex((unsigned int)pScenario->m_hPayThread);
		return 0;
	}

	char    szClientIP[20 + 1] = { 0x00 };
	char    szMallid[32 + 1] = { 0x00 };
	char    szGwUrl[50 + 1] = {0x00,};
	char    szGWport[10 + 1] = {0x00,};
	char    CertFile[256 + 1] = {0x00,};
	char    LogDir[256 + 1] = {0x00,};
	char    szKICC_ADMIN[256 + 1] = { 0x00, };
	int     LogLv;
	S_CFG_I   obj_s_CFG_I;// 환경변수

	GetPrivateProfileString("KICCPAY", "KICC_GW_URL", KICC_GW_URL, szGwUrl, sizeof(szGwUrl), PARAINI);
	GetPrivateProfileString("KICCPAY", "KICC_GW_PORT", KICC_GW_PORT, szGWport, sizeof(szGWport), PARAINI);

	// 게이트웨이 URL 확인 로그
	eprintf("[KICC] 게이트웨이 URL: %s, 포트: %s", szGwUrl, szGWport);

	// [MODIFIED] 설정 파일에서 IP 읽기 (빠른 처리)
	GetPrivateProfileString("KICCPAY", "KICC_CLIENT_IP", KICC_CLIENT_IP, szClientIP, sizeof(szClientIP), PARAINI);
	// 설정 파일에 IP가 없거나 비어있을 경우에만 웹에서 조회
	if (strlen(szClientIP) == 0) {
		eprintf("KICC_CLIENT_IP not found in config, trying web lookup...");
		FindWebAddresss(ch, szClientIP);
	}
	else {
		eprintf("Using KICC_CLIENT_IP from config: %s", szClientIP);
	}

	GetPrivateProfileString("KICCPAY", "KICC_MAII_ID", KICC_MAII_ID, szMallid, sizeof(szMallid), PARAINI);

	if (strlen(pScenario->m_szterminal_id) > 0)
	{
		memset(szMallid, 0x00, sizeof(szMallid));
		memcpy(szMallid, pScenario->m_szterminal_id, sizeof(szMallid) - 1);
	}

	// ========================================
	// [TEST CODE] 마지막 주문 실패 테스트
	// 테스트 완료 후 이 블록 전체를 삭제하세요
	// ========================================
//#define TEST_LAST_ORDER_FAIL  // 이 줄을 주석 처리하면 테스트 비활성화
#ifdef TEST_LAST_ORDER_FAIL
	if (pScenario->m_bMultiOrderMode && pScenario->m_MultiOrders.nOrderCount > 1) {
		// 마지막 주문인지 확인 (m_nCurrentOrderIdx는 0부터 시작)
		if (pScenario->m_nCurrentOrderIdx == pScenario->m_MultiOrders.nOrderCount - 1) {
			eprintf("[TEST] 마지막 주문 실패 테스트 활성화: 가맹점ID를 Txxxxxx로 변경 (원래값: %s)", szMallid);
			memset(szMallid, 0x00, sizeof(szMallid));
			strncpy(szMallid, "Txxxxxx", sizeof(szMallid) - 1);
		}
	}
#endif
	// ========================================
	// [END TEST CODE]
	// ========================================

	GetPrivateProfileString("KICCPAY", "KICC__CERT_FILE", KICC__CERT_FILE, CertFile, sizeof(CertFile), PARAINI);
	GetPrivateProfileString("KICCPAY", "KICC_LOG", KICC_LOG, LogDir, sizeof(LogDir), PARAINI);

	GetPrivateProfileString("KICCPAY", "KICC_ADMIN", KICC_ADMIN, szKICC_ADMIN, sizeof(szKICC_ADMIN), PARAINI);
	if (strlen(pScenario->m_szADMIN_ID)>0)
	{
		memset(szKICC_ADMIN, 0x00, sizeof(szKICC_ADMIN));
		memcpy(szKICC_ADMIN, pScenario->m_szADMIN_ID, sizeof(szKICC_ADMIN) - 1);
	}

	LogLv = GetPrivateProfileInt("KICCPAY", "KICC_LOG_LV", KICC_LOG_LV, PARAINI);


	/* -------------------------------------- */
	// 초기화 (환경설정)
	int hr = lplfEP_CLI_DLL__init(szGwUrl,
		szGWport,
		CertFile,
		LogDir,
		LogLv,
		&obj_s_CFG_I);

	if (S_OK != hr)
	{
		pScenario->m_PaySysCd = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		KiccNone_Pay_Quithostio("KiccArsPayProcess the line service is not valid any more-lplfEP_CLI_DLL__init.", ch);
		eprintf("KiccArsPayProcess END");
		_endthreadex((unsigned int)pScenario->m_hPayThread);
		return 0;
	}

	// ========================================
	// [2025-11-21 NEW] DB 기반 카드정보 검증
	// ========================================
	if (pScenario->m_bUseDbCardInfo) {
		info_printf(ch, "[KICC] DB 기반 결제 시작");
		eprintf("[KICC] DB 기반 결제 시작: 카드번호=%s(길이:%d), 유효기간=%s, 할부개월=%s",
			pScenario->m_CardInfo.Card_Num,
			strlen(pScenario->m_CardInfo.Card_Num),
			pScenario->m_CardInfo.ExpireDt,
			pScenario->m_CardInfo.InstPeriod);

		// 카드번호 길이 검증 (15~16자리 허용)
		int nCardLen = strlen(pScenario->m_CardInfo.Card_Num);
		if (nCardLen < 15 || nCardLen > 16) {
			info_printf(ch, "[KICC] 카드번호 길이 오류");
			eprintf("[KICC] 카드번호 길이 오류: %d자리 (기대: 15~16자리)", nCardLen);
			(*port)[ch].ppftbl[POST_NET].postcode = HI_OK;
			pScenario->m_PaySysCd = -1;
			KiccNone_Pay_Quithostio("KiccArsPayProcess 카드번호 길이 오류", ch);
			eprintf("KiccArsPayProcess END");
			_endthreadex((unsigned int)pScenario->m_hPayThread);
			return 0;
		}

		// 유효기간 형식 검증
		if (strlen(pScenario->m_CardInfo.ExpireDt) != 4) {
			info_printf(ch, "[KICC] 유효기간 형식 오류");
			eprintf("[KICC] 유효기간 형식 오류: %s (기대: YYMM)",
				pScenario->m_CardInfo.ExpireDt);
			(*port)[ch].ppftbl[POST_NET].postcode = HI_OK;
			pScenario->m_PaySysCd = -1;
			KiccNone_Pay_Quithostio("KiccArsPayProcess 유효기간 형식 오류", ch);
			eprintf("KiccArsPayProcess END");
			_endthreadex((unsigned int)pScenario->m_hPayThread);
			return 0;
		}

		// [MODIFIED] 할부개월 검증 및 기본값 설정
		// 할부개월이 비어있거나 잘못된 경우 DB에서 읽어온 값 또는 기본값 "00"(일시불)로 설정
		if (strlen(pScenario->m_CardInfo.InstPeriod) == 0) {
			// DB에서 할부개월을 읽어온 경우 우선 사용
			if (strlen(pScenario->m_szDB_InstallPeriod) > 0) {
				int nInstall = atoi(pScenario->m_szDB_InstallPeriod);
				if (nInstall >= 0 && nInstall <= 12) {
					memset(pScenario->m_CardInfo.InstPeriod, 0x00, sizeof(pScenario->m_CardInfo.InstPeriod));
					sprintf(pScenario->m_CardInfo.InstPeriod, "%02d", nInstall);
					info_printf(ch, "[KICC] 할부개월 DB값 설정: %s", pScenario->m_CardInfo.InstPeriod);
					eprintf("[KICC] 할부개월이 비어있어 DB 값으로 설정: %s", pScenario->m_CardInfo.InstPeriod);
				}
				else {
					// DB 값이 범위를 벗어난 경우 기본값 사용
					memset(pScenario->m_CardInfo.InstPeriod, 0x00, sizeof(pScenario->m_CardInfo.InstPeriod));
					strncpy(pScenario->m_CardInfo.InstPeriod, "00", sizeof(pScenario->m_CardInfo.InstPeriod) - 1);
					info_printf(ch, "[KICC] 할부개월 기본값 설정: %s", pScenario->m_CardInfo.InstPeriod);
					eprintf("[KICC] DB 할부개월 범위 오류, 기본값(일시불)으로 설정: %s", pScenario->m_CardInfo.InstPeriod);
				}
			}
			else {
				// DB 값도 없는 경우 기본값 사용
				memset(pScenario->m_CardInfo.InstPeriod, 0x00, sizeof(pScenario->m_CardInfo.InstPeriod));
				strncpy(pScenario->m_CardInfo.InstPeriod, "00", sizeof(pScenario->m_CardInfo.InstPeriod) - 1);
				info_printf(ch, "[KICC] 할부개월 기본값 설정: %s", pScenario->m_CardInfo.InstPeriod);
				eprintf("[KICC] 할부개월이 비어있어 기본값(일시불)으로 설정: %s", pScenario->m_CardInfo.InstPeriod);
			}
		}

		// 할부개월 범위 검증
		int nInstallPeriod = atoi(pScenario->m_CardInfo.InstPeriod);
		if (nInstallPeriod < 0 || nInstallPeriod > 12) {
			// 범위를 벗어난 경우 기본값(일시불)으로 설정
			memset(pScenario->m_CardInfo.InstPeriod, 0x00, sizeof(pScenario->m_CardInfo.InstPeriod));
			strncpy(pScenario->m_CardInfo.InstPeriod, "00", sizeof(pScenario->m_CardInfo.InstPeriod) - 1);
			info_printf(ch, "[KICC] 할부개월 기본값 설정");
			eprintf("[KICC] 할부개월 범위 오류로 기본값(일시불)으로 설정: 원래값 범위 초과 (범위: 00~12)");
		}

		// 결제 데이터 전송 직전 로그 (info_printf는 간단하게, eprintf는 상세하게)
		info_printf(ch, "[KICC] 결제 요청 데이터 준비 완료");
		eprintf("[KICC] 결제 요청 데이터: 카드번호=%s, 유효기간=%s, 할부개월=%s, 주민번호=%s",
			pScenario->m_CardInfo.Card_Num,
			pScenario->m_CardInfo.ExpireDt,
			pScenario->m_CardInfo.InstPeriod,
			pScenario->m_CardInfo.SecretNo);
	}

	//거래 유형
	char    szTrCd[8 + 1] = TRAN_CD_NOR_PAYMENT;/// [필수]카드결제종류
	// 요청전문
	char    szReqData[20480];
	// 응답전문
	char    szResData[20480];

	char    szAmount[14 + 1] = { 0x00, };// [필수]결제 총 금액
	char    szCurrency[2 + 1] = "00";    // [필수]통화 구분
	char    szCardTxType[2 + 1] = "20";  // [필수]처리종류 승인(20)
	char    szReqType[1 + 1] = "0";      // [필수]카드결제 전문 암호화 (0:SSL 1:ISP, 2:안심 결제)
	char    szNoint[2 + 1] = "00";       // [필수]무이자여부(일반:00, 무이자:02
	char    szCertType[1 + 1] = "0";     // [필수]인증여부 (인증:0 , 비인증:1, 카유비:0) , 비밀번호 인증 하지 않음(1)으로 한다.
	char    szWcc[1 + 1] = "@";          // [필수]wcc
	char    szUserType[1 + 1] = "0";     // [선택]카드구분 : 인증여부에 따라 필수(개인: 0, 법인:1)

	// [MODIFIED] 주민번호/사업자번호 처리 개선
	if (strlen(pScenario->m_CardInfo.SecretNo) >= 10)
	{// 사업자 번호이면 (10자리 이상)
		memset(szUserType, 0x00, sizeof(szUserType));
		strncpy(szUserType, "1", sizeof(szUserType) - 1);
		eprintf("user_type=1 (사업자번호: %d자리)", strlen(pScenario->m_CardInfo.SecretNo));
	}
	else if (strlen(pScenario->m_CardInfo.SecretNo) == 6)
	{// 개인 (생년월일 6자리)
		eprintf("user_type=0 (개인/생년월일: 6자리)");
	}
	else
	{// 주민번호 없음 - 개인으로 기본 설정
		eprintf("user_type=0 (주민번호 생략 - 개인 기본값)");
	}

	sprintf_s(szAmount, sizeof(szAmount), "%d", pScenario->m_namount);//결제할 금액 설정

	memset(szReqData, 0x00, sizeof(szReqData));
	memset(szResData, 0x00, sizeof(szResData));

	memset(pScenario->m_CardResInfo.REPLY_CODE, 0x00, sizeof(pScenario->m_CardResInfo.REPLY_CODE));
	memset(pScenario->m_CardResInfo.REPLY_MESSAGE, 0x00, sizeof(pScenario->m_CardResInfo.REPLY_MESSAGE));

	// 결제정보 DATA
	lplfEP_CLI_DLL__set_entry("pay_data", szReqData, sizeof(szReqData) - 1); // 결제 요청
	lplfEP_CLI_DLL__set_entry("common", szReqData, sizeof(szReqData) - 1);   // 결제 요청 전문의 공통부

	lplfEP_CLI_DLL__set_value("tot_amt", szAmount, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);    //결제할 금액 설정
	lplfEP_CLI_DLL__set_value("currency", szCurrency, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);
	lplfEP_CLI_DLL__set_value("client_ip", szClientIP, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);
	lplfEP_CLI_DLL__set_value("join_cd", "JC29", M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);
	// 구분 문자
	lplfEP_CLI_DLL__set_delim(M_EP_PK__DELI__RS__C, szReqData, sizeof(szReqData) - 1);

	//입력 정보
	lplfEP_CLI_DLL__set_entry("card", szReqData, sizeof(szReqData) - 1);   // 카드 정보임을 알리기 위한 테그
	lplfEP_CLI_DLL__set_value("card_txtype", szCardTxType, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);
	lplfEP_CLI_DLL__set_value("req_type", szReqType, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);
	lplfEP_CLI_DLL__set_value("card_amt", szAmount, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);
	lplfEP_CLI_DLL__set_value("noint", szNoint, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);
	lplfEP_CLI_DLL__set_value("cert_type", szCertType, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);
	lplfEP_CLI_DLL__set_value("wcc", szWcc, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);
	lplfEP_CLI_DLL__set_value("card_no", pScenario->m_CardInfo.Card_Num, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);        // [필수]카드번호
	lplfEP_CLI_DLL__set_value("expire_date", pScenario->m_CardInfo.ExpireDt, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);  // [필수]유효기간
	lplfEP_CLI_DLL__set_value("password", pScenario->m_CardInfo.Password, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);     // [선택]비밀번호 : 인증여부에 따라 필수
	lplfEP_CLI_DLL__set_value("auth_value", pScenario->m_CardInfo.SecretNo, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);   // [선택]인증값   : 인증여부에 따라 필수(주민/법인번호)
	lplfEP_CLI_DLL__set_value("user_type", szUserType, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);                             // [선택]카드구분 : 인증여부에 따라 필수(개인: 0, 법인:1)
	lplfEP_CLI_DLL__set_value("install_period", pScenario->m_CardInfo.InstPeriod, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);// 할부 개월수
	//구분자
	lplfEP_CLI_DLL__set_delim(M_EP_PK__DELI__FS__C, szReqData, sizeof(szReqData) - 1);

	// 주문정보 DATA
	// 상품단가는 아직 쓰이지 않고, 총액과 동일하게 사용
	// memset(szAmount, 0x00, sizeof(szAmount));
	// sprintf_s(szAmount, sizeof(szAmount), "%d", pScenario->m_CustOrder_Info.AMOUNT);// 상품 단가
	lplfEP_CLI_DLL__set_entry("order_data", szReqData, sizeof(szReqData) - 1);
	char szDefine1[32 + 1] = { 0x00, };
	char szDefine2[32 + 1] = { 0x00, };
	char szDefine3[32 + 1] = { 0x00, };
	char szDefine4[32 + 1] = { 0x00, };
	char szDefine5[32 + 1] = { 0x00, };
	char szDefine6[32 + 1] = { 0x00, };
	// [MODIFIED] 첫 번째 조회 결과 재사용 (중복 호출 제거)
	if (strlen(szClientIP) > 0) {
		strncpy(szDefine2, szClientIP, sizeof(szDefine2) - 1);
		eprintf("Reusing client IP for user_define2: %s", szDefine2);
	}
	else {
		FindWebAddresss(ch, szDefine2);
	}

	char szOrder_no[32 + 1] = { 0x00, };

	// 참좋은여행사를 위한
	if (strcmp(pScenario->m_szterminal_id, /*"05112158"*/"05534047") == EQUAL)
	{
		memcpy(szOrder_no, pScenario->m_szorder_no, 12); //참좋은여행사를 위한 주문번호 처리 가맹점 주문번호 12자리만 전송
		memcpy(szDefine3, pScenario->m_szorder_no, sizeof(szDefine3) - 1); // 참좋은 여행사 특별 (원래 발행)주문번호

		lplfEP_CLI_DLL__set_value("order_no", szOrder_no, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1); //참좋은여행사를 위한 주문번호 처리 가맹점 주문번호 12자리만 전송
	}
	else
	{
		memcpy(szOrder_no, pScenario->m_szorder_no, sizeof(szOrder_no) - 1);
		lplfEP_CLI_DLL__set_value("order_no", szOrder_no, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);//주문 번호
	}

	lplfEP_CLI_DLL__set_value("product_nm", pScenario->m_szgood_nm, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);//상품명
	lplfEP_CLI_DLL__set_value("product_amt", szAmount, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);  // 상품 단가

	lplfEP_CLI_DLL__set_value("user_define1", szDefine1, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);
	lplfEP_CLI_DLL__set_value("user_define2", szDefine2, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);
	lplfEP_CLI_DLL__set_value("user_define3", szDefine3, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);
	lplfEP_CLI_DLL__set_value("user_define4", szDefine4, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);
	lplfEP_CLI_DLL__set_value("user_define5", szDefine5, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);
	lplfEP_CLI_DLL__set_value("user_define6", szDefine6, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);


	//구분자
	lplfEP_CLI_DLL__set_delim(M_EP_PK__DELI__FS__C, szReqData, sizeof(szReqData) - 1);

	// 고객 정보 설정

	lplfEP_CLI_DLL__set_value("user_nm", pScenario->m_szcust_nm, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);//고객명
	lplfEP_CLI_DLL__set_value("user_id", "ARS", M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);//고객 ID "ARS"
	// lplfEP_CLI_DLL__set_value("user_mail", pScenario->m_szCC_email, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1); // 고객 이메일 주소
	lplfEP_CLI_DLL__set_value("user_phone1", pScenario->szDnis, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);  //고객 전화 번호
	lplfEP_CLI_DLL__set_value("user_phone2", pScenario->m_szInputTel, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);  //고객 전화 번호 입력 정보
	//구분자
	lplfEP_CLI_DLL__set_delim(M_EP_PK__DELI__FS__C, szReqData, sizeof(szReqData) - 1);

	// 전문 셋팅
	lplfEP_CLI_DLL__set_plan_data(szReqData, &obj_s_CFG_I);

	// KICC와 통신
	eprintf("[KICC] 결제 요청 전송: 게이트웨이=%s:%s, 가맹점ID=%s, 주문번호=%s",
		szGwUrl, szGWport, szMallid, szOrder_no);
	lplfEP_CLI_DLL__proc(szTrCd, szMallid, szClientIP, szOrder_no, &obj_s_CFG_I, szResData, sizeof(szResData) - 1);

	memset(&pScenario->m_CardResInfo, 0x00, sizeof(pScenario->m_CardResInfo));

	// 응답전문 파싱
	lplfEP_CLI_DLL__get_value(szResData, "res_cd", pScenario->m_CardResInfo.REPLY_CODE, sizeof(pScenario->m_CardResInfo.REPLY_CODE) - 1);
	lplfEP_CLI_DLL__get_value(szResData, "res_msg", pScenario->m_CardResInfo.REPLY_MESSAGE, sizeof(pScenario->m_CardResInfo.REPLY_MESSAGE) - 1);

	lplfEP_CLI_DLL__get_value(szResData, "auth_no", pScenario->m_CardResInfo.APPROVAL_NUM, sizeof(pScenario->m_CardResInfo.APPROVAL_NUM) - 1);     // 승인번호
	lplfEP_CLI_DLL__get_value(szResData, "tran_date", pScenario->m_CardResInfo.APPROVAL_DATE, sizeof(pScenario->m_CardResInfo.APPROVAL_DATE) - 1); // 승인일시

	CTime CurTime = CTime::GetCurrentTime();

	if (strlen(pScenario->m_CardResInfo.APPROVAL_DATE) < 1)
	{
		memcpy(pScenario->m_CardResInfo.APPROVAL_DATE, CurTime.Format("%Y%m%d%H%M%S").GetBuffer(14), sizeof(pScenario->m_CardResInfo.APPROVAL_DATE) - 1);
	}

	lplfEP_CLI_DLL__get_value(szResData, "cno", pScenario->m_CardResInfo.CONTROL_NO, sizeof(pScenario->m_CardResInfo.CONTROL_NO) - 1);             // PG거래번호

	lplfEP_CLI_DLL__get_value(szResData, "issuer_cd", pScenario->m_CardResInfo.issuer_cd, sizeof(pScenario->m_CardResInfo.issuer_cd) - 1);        // 발급사코드
	lplfEP_CLI_DLL__get_value(szResData, "issuer_nm", pScenario->m_CardResInfo.issuer_nm, sizeof(pScenario->m_CardResInfo.issuer_nm) - 1);		// 발급사명
	lplfEP_CLI_DLL__get_value(szResData, "uirer_cd", pScenario->m_CardResInfo.uirer_cd, sizeof(pScenario->m_CardResInfo.uirer_cd) - 1);		    // 매입사코드
	lplfEP_CLI_DLL__get_value(szResData, "acquirer_nm", pScenario->m_CardResInfo.acquirer_nm, sizeof(pScenario->m_CardResInfo.acquirer_nm) - 1);	// 매입사명

	lplfEP_CLI_DLL__get_value(szResData, "install_period", pScenario->m_CardResInfo.InstPeriod, sizeof(pScenario->m_CardResInfo.InstPeriod) - 1);// 할부 개월수
	if (strlen(pScenario->m_CardResInfo.InstPeriod) < 1)
	{
		memcpy(pScenario->m_CardResInfo.InstPeriod, pScenario->m_CardInfo.InstPeriod, sizeof(pScenario->m_CardInfo.InstPeriod) - 1);
	}
	memset(szAmount, 0x00, sizeof(szAmount));
	if (strcmp(pScenario->m_szterminal_id, /*"05112158"*/"05534047") == EQUAL)
	{
		memcpy(pScenario->m_CardResInfo.ORDER_NO, pScenario->m_szorder_no, sizeof(pScenario->m_CardResInfo.ORDER_NO) - 1);// 상점별 주문 번호
	}
	else
	{
		lplfEP_CLI_DLL__get_value(szResData, "order_no", pScenario->m_CardResInfo.ORDER_NO, sizeof(pScenario->m_CardResInfo.ORDER_NO) - 1);// 상점별 주문 번호
	}
	lplfEP_CLI_DLL__get_value(szResData, "tot_amt", szAmount, sizeof(szReqData) - 1);
	pScenario->m_CardResInfo.AMOUNT = atoi(szAmount);
	if (pScenario->m_CardResInfo.AMOUNT < 1) pScenario->m_CardResInfo.AMOUNT = pScenario->m_namount;

	memset(pScenario->m_CardResInfo.TERMINAL_ID, 0x00, sizeof(pScenario->m_CardResInfo.TERMINAL_ID));
	memcpy(pScenario->m_CardResInfo.TERMINAL_ID, pScenario->m_szterminal_id, sizeof(pScenario->m_CardResInfo.TERMINAL_ID) - 1);

	KiccNone_Pay_Quithostio("KiccArsPayProcess the line service is 성공.", ch);
	eprintf("KiccArsPayProcess END");
	_endthreadex((unsigned int)pScenario->m_hPayThread);

	return 0;
}

// 결제 취소 요청 쓰레드용 함수
unsigned int __stdcall KiccArsPayCancleProcess(void *data)
{
	int			ch;
	int			paythreadID;
	char        AnsiNO_Char[257] = { 0x00, };
	LPMTP		*lineTablePtr = (LPMTP *)data;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)(lineTablePtr->pScenario);

	ch = lineTablePtr->chanID;
	paythreadID = pScenario->paythreadID;

	pScenario->m_PaySysCd = 0;
	if (paythreadID != pScenario->paythreadID) {
		pScenario->m_PaySysCd = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		KiccNone_Pay_Quithostio("ArsPayCancleProcess the line service is not valid any more.", ch);
		eprintf("KiccArsPayCancleProcess END");
		_endthreadex((unsigned int)pScenario->m_hPayThread);
		return 0;
	}

	char    szClientIP[20 + 1] = { 0x00 };
	char    szMallid[32 + 1] = { 0x00 };
	char    szGwUrl[50 + 1] = {0x00,};
	char    szGWport[10 + 1] = {0x00,};
	char    CertFile[256 + 1] = {0x00,};
	char    LogDir[256 + 1] = {0x00,};
	char    szKICC_ADMIN[256 + 1] = { 0x00, };

	int     LogLv;
	S_CFG_I   obj_s_CFG_I;// 환경변수

	GetPrivateProfileString("KICCPAY", "KICC_GW_URL", KICC_GW_URL, szGwUrl, sizeof(szGwUrl), PARAINI);
	GetPrivateProfileString("KICCPAY", "KICC_GW_PORT", KICC_GW_PORT, szGWport, sizeof(szGWport), PARAINI);

	// 게이트웨이 URL 확인 로그
	eprintf("[KICC] 게이트웨이 URL: %s, 포트: %s", szGwUrl, szGWport);

	// [MODIFIED] 설정 파일에서 IP 읽기 (빠른 처리)
	GetPrivateProfileString("KICCPAY", "KICC_CLIENT_IP", KICC_CLIENT_IP, szClientIP, sizeof(szClientIP), PARAINI);
	// 설정 파일에 IP가 없거나 비어있을 경우에만 웹에서 조회
	if (strlen(szClientIP) == 0) {
		eprintf("KICC_CLIENT_IP not found in config, trying web lookup...");
		FindWebAddresss(ch, szClientIP);
	}
	else {
		eprintf("Using KICC_CLIENT_IP from config: %s", szClientIP);
	}

	GetPrivateProfileString("KICCPAY", "KICC_MAII_ID", KICC_MAII_ID, szMallid, sizeof(szMallid), PARAINI);
	if (strlen(pScenario->m_Card_CancleInfo.TERMINAL_ID) > 0)
	{
		memset(szMallid, 0x00, sizeof(szMallid));
		memcpy(szMallid, pScenario->m_Card_CancleInfo.TERMINAL_ID, sizeof(szMallid) - 1);
	}
	GetPrivateProfileString("KICCPAY", "KICC__CERT_FILE", KICC__CERT_FILE, CertFile, sizeof(CertFile), PARAINI);
	GetPrivateProfileString("KICCPAY", "KICC_LOG", KICC_LOG, LogDir, sizeof(LogDir), PARAINI);

	GetPrivateProfileString("KICCPAY", "KICC_ADMIN", KICC_ADMIN, szKICC_ADMIN, sizeof(szKICC_ADMIN), PARAINI);
	if (strlen(pScenario->m_Card_CancleInfo.ADMIN_ID)>0)
	{
		memset(szKICC_ADMIN, 0x00, sizeof(szKICC_ADMIN));
		memcpy(szKICC_ADMIN, pScenario->m_Card_CancleInfo.ADMIN_ID, sizeof(szKICC_ADMIN) - 1);

	}
	LogLv = GetPrivateProfileInt("KICCPAY", "KICC_LOG_LV", KICC_LOG_LV, PARAINI);


	/* -------------------------------------- */
	// 초기화 (환경설정)
	int hr = lplfEP_CLI_DLL__init(szGwUrl,
		szGWport,
		CertFile,
		LogDir,
		LogLv,
		&obj_s_CFG_I);

	if (S_OK != hr)
	{
		pScenario->m_PaySysCd = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		KiccNone_Pay_Quithostio("KiccArsPayCancleProcess the line service is not valid any more-lplfEP_CLI_DLL__init.", ch);
		eprintf("KiccArsPayCancleProcess END");
		_endthreadex((unsigned int)pScenario->m_hPayThread);
		return 0;
	}

	//거래 유형
	char    szTrCd[8 + 1] = TRAN_CD_NOR_MGR;//카드 거래 변경
	// 요청전문
	char    szReqData[20480];
	// 응답전문
	char    szResData[20480];

	char    szMgrTxType[2 + 1] = "40";// 거래 취소
	char    szReqId[32 + 1] = { 0x00, }; // 누가 취소했는지 알기 위한 필드

	memcpy(szReqId, szKICC_ADMIN, sizeof(szReqId) - 1);

	memset(szReqData, 0x00, sizeof(szReqData));
	memset(szResData, 0x00, sizeof(szResData));

	memset(pScenario->m_Cancel_ResInfo.REPLY_CODE, 0x00, sizeof(pScenario->m_Cancel_ResInfo.REPLY_CODE));
	memset(pScenario->m_Cancel_ResInfo.REPLY_MESSAGE, 0x00, sizeof(pScenario->m_Cancel_ResInfo.REPLY_MESSAGE));

	// 결제정보 DATA
	lplfEP_CLI_DLL__set_entry("mgr_data", szReqData, sizeof(szReqData));

	lplfEP_CLI_DLL__set_value("mgr_txtype", szMgrTxType, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData));
	lplfEP_CLI_DLL__set_value("org_cno", pScenario->m_Card_CancleInfo.CONTROL_NO, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData)); // 승인응답전문에 내려주는 KICC 거래번호(cno)
	lplfEP_CLI_DLL__set_value("req_ip", szClientIP, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData));
	lplfEP_CLI_DLL__set_value("req_id", szReqId, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData));

	lplfEP_CLI_DLL__set_delim(M_EP_PK__DELI__FS__C, szReqData, sizeof(szReqData));

	// 전문 셋팅
	lplfEP_CLI_DLL__set_plan_data(szReqData, &obj_s_CFG_I);

	// KICC와 통신
	eprintf("[KICC] 취소 요청 전송: 게이트웨이=%s:%s, 가맹점ID=%s, 주문번호=%s",
		szGwUrl, szGWport, szMallid, pScenario->m_Card_CancleInfo.ORDER_NO);
	lplfEP_CLI_DLL__proc(szTrCd, szMallid, szClientIP, pScenario->m_Card_CancleInfo.ORDER_NO, &obj_s_CFG_I, szResData, sizeof(szResData) - 1);

	// 응답전문 파싱
	lplfEP_CLI_DLL__get_value(szResData, "res_cd", pScenario->m_Cancel_ResInfo.REPLY_CODE, sizeof(pScenario->m_Cancel_ResInfo.REPLY_CODE) - 1);
	lplfEP_CLI_DLL__get_value(szResData, "res_msg", pScenario->m_Cancel_ResInfo.REPLY_MESSAGE, sizeof(pScenario->m_Cancel_ResInfo.REPLY_MESSAGE) - 1);

	lplfEP_CLI_DLL__get_value(szResData, "auth_no", pScenario->m_Cancel_ResInfo.APPROVAL_NUM, sizeof(pScenario->m_Cancel_ResInfo.APPROVAL_NUM) - 1);     // 승인번호
	lplfEP_CLI_DLL__get_value(szResData, "tran_date", pScenario->m_Cancel_ResInfo.APPROVAL_DATE, sizeof(pScenario->m_Cancel_ResInfo.APPROVAL_DATE) - 1); // 승인일시
	lplfEP_CLI_DLL__get_value(szResData, "cno", pScenario->m_Cancel_ResInfo.CONTROL_NO, sizeof(pScenario->m_Cancel_ResInfo.CONTROL_NO) - 1);             // PG거래번호

	lplfEP_CLI_DLL__get_value(szResData, "issuer_cd", pScenario->m_Cancel_ResInfo.issuer_cd, sizeof(pScenario->m_Cancel_ResInfo.issuer_cd) - 1);        // 발급사코드
	lplfEP_CLI_DLL__get_value(szResData, "issuer_nm", pScenario->m_Cancel_ResInfo.issuer_nm, sizeof(pScenario->m_Cancel_ResInfo.issuer_nm) - 1);		// 발급사명
	lplfEP_CLI_DLL__get_value(szResData, "uirer_cd", pScenario->m_Cancel_ResInfo.uirer_cd, sizeof(pScenario->m_Cancel_ResInfo.uirer_cd) - 1);		    // 매입사코드
	lplfEP_CLI_DLL__get_value(szResData, "acquirer_nm", pScenario->m_Cancel_ResInfo.acquirer_nm, sizeof(pScenario->m_Cancel_ResInfo.acquirer_nm) - 1);	// 매입사명

	lplfEP_CLI_DLL__set_value("install_period", pScenario->m_Cancel_ResInfo.InstPeriod, M_EP_PK__DELI__US__C, szReqData, sizeof(szReqData) - 1);// 할부 개월수

	CTime CurTime = CTime::GetCurrentTime();

	if (strlen(pScenario->m_Cancel_ResInfo.APPROVAL_DATE) < 1)
	{
		memcpy(pScenario->m_CardResInfo.APPROVAL_DATE, CurTime.Format("%Y%m%d%H%M%S").GetBuffer(14), sizeof(pScenario->m_CardResInfo.APPROVAL_DATE) - 1);
	}

	char    szAmount[14 + 1] = { 0x00, };// [필수]결제 총 금액

	memset(szAmount, 0x00, sizeof(szAmount));
	if (strcmp(pScenario->m_szterminal_id, /*"05112158"*/"05534047") == EQUAL)
	{
		memcpy(pScenario->m_Cancel_ResInfo.ORDER_NO, pScenario->m_szorder_no, sizeof(pScenario->m_Cancel_ResInfo.ORDER_NO) - 1);// 상점별 주문 번호
	}
	else
	{
		lplfEP_CLI_DLL__get_value(szResData, "order_no", pScenario->m_Cancel_ResInfo.ORDER_NO, sizeof(pScenario->m_Cancel_ResInfo.ORDER_NO) - 1);// 상점별 주문 번호
	}
	lplfEP_CLI_DLL__get_value(szResData, "tot_amt", szAmount, sizeof(szReqData) - 1);
	pScenario->m_Cancel_ResInfo.AMOUNT = atoi(szAmount);
	if (pScenario->m_CardResInfo.AMOUNT < 1) pScenario->m_Cancel_ResInfo.AMOUNT = pScenario->m_namount;

	memset(pScenario->m_Cancel_ResInfo.TERMINAL_ID, 0x00, sizeof(pScenario->m_Cancel_ResInfo.TERMINAL_ID));
	memcpy(pScenario->m_Cancel_ResInfo.TERMINAL_ID, pScenario->m_szterminal_id, sizeof(pScenario->m_Cancel_ResInfo.TERMINAL_ID) - 1);

	KiccNone_Pay_Quithostio("KiccArsPayCancleProcess the line service is 성공.", ch);
	eprintf("KiccArsPayCancleProcess END");
	_endthreadex((unsigned int)pScenario->m_hPayThread);
	return 0;
}

int  KiccPaymemt_host(int holdm)
{//초기화
	CKICC_Scenario *pScenario = (CKICC_Scenario *)((*lpmt)->pScenario);

	if (holdm != 0) {
		new_guide();
		(*lpmt)->trials = 0;
		(*lpmt)->Hmusic = HM_LOOP;
		set_guide(holdm);
		send_guide(NODTMF);
	}
	if (((CKICC_Scenario *)((*lpmt)->pScenario))->m_hPayThread)
	{
		WaitForSingleObject(((CKICC_Scenario *)((*lpmt)->pScenario))->m_hPayThread, INFINITE);
		CloseHandle(((CKICC_Scenario *)((*lpmt)->pScenario))->m_hPayThread);
		((CKICC_Scenario *)((*lpmt)->pScenario))->m_hPayThread = NULL;
	}

	(*lpmt)->ppftbl[POST_NET].postcode = HI_NCMPLT;// 종료하지 않았다.

	pScenario->m_hPayThread = (HANDLE)_beginthreadex(NULL, 0, KiccArsPayProcess, (LPVOID)(*lpmt), 0, &(pScenario->paythreadID));
	return(0);
}

int  KiccPaymemtCancle_host(int holdm)
{
	//초기화
	CKICC_Scenario *pScenario = (CKICC_Scenario *)((*lpmt)->pScenario);

	if (holdm != 0) {
		new_guide();
		(*lpmt)->trials = 0;
		(*lpmt)->Hmusic = HM_LOOP;
		set_guide(holdm);
		send_guide(NODTMF);
	}
	if (((CKICC_Scenario *)((*lpmt)->pScenario))->m_hPayThread)
	{
		WaitForSingleObject(((CKICC_Scenario *)((*lpmt)->pScenario))->m_hPayThread, INFINITE);
		CloseHandle(((CKICC_Scenario *)((*lpmt)->pScenario))->m_hPayThread);
		((CKICC_Scenario *)((*lpmt)->pScenario))->m_hPayThread = NULL;
	}

	(*lpmt)->ppftbl[POST_NET].postcode = HI_NCMPLT;// 종료하지 않았다.
	pScenario->m_hPayThread = (HANDLE)_beginthreadex(NULL, 0, KiccArsPayCancleProcess, (LPVOID)(*lpmt), 0, &(pScenario->paythreadID));

	return(0);
}

/**
 * [NEW] EUC-KR(CP949) 문자열을 UTF-8로 변환
 * @param pszEucKr  EUC-KR 입력 문자열
 * @param pszUtf8   UTF-8 출력 버퍼
 * @param nUtf8Size 출력 버퍼 크기
 * @return 변환된 UTF-8 문자열 길이, 실패 시 -1
 */
int ConvertEucKrToUtf8(const char* pszEucKr, char* pszUtf8, int nUtf8Size)
{
	if (!pszEucKr || !pszUtf8 || nUtf8Size <= 0) {
		return -1;
	}

	// Step 1: EUC-KR(CP949) → Unicode (wchar_t)
	int nWideLen = MultiByteToWideChar(949, 0, pszEucKr, -1, NULL, 0);
	if (nWideLen <= 0) {
		return -1;
	}

	wchar_t* pwszWide = new wchar_t[nWideLen];
	MultiByteToWideChar(949, 0, pszEucKr, -1, pwszWide, nWideLen);

	// Step 2: Unicode → UTF-8
	int nUtf8Len = WideCharToMultiByte(CP_UTF8, 0, pwszWide, -1, NULL, 0, NULL, NULL);
	if (nUtf8Len <= 0 || nUtf8Len > nUtf8Size) {
		delete[] pwszWide;
		return -1;
	}

	WideCharToMultiByte(CP_UTF8, 0, pwszWide, -1, pszUtf8, nUtf8Size, NULL, NULL);
	delete[] pwszWide;

	return nUtf8Len - 1;  // null terminator 제외
}

/**
 * [NEW] 결제 실패 노티 전송 함수
 * @brief 결제 실패 시 KICC EasyPay 노티 형식과 동일한 JSON 데이터를 외부 URL로 HTTP POST 전송
 * @param pCardResInfo 결제 응답 정보 구조체 포인터
 * @param pCardInfo 카드 입력 정보 구조체 포인터
 * @return 전송 결과
 *         1: 성공
 *         0: 비활성화 상태 (스킵)
 *        -1: URL 미설정
 *        -2: URL 파싱 실패
 *        -3: HTTP 응답 오류
 *        -4: 네트워크 예외
 */
int SendFailNoti(Card_ResInfo* pCardResInfo)
{
	// [1] INI 설정 읽기 - 노티 활성화 여부 확인
	char szEnabled[10 + 1] = { 0x00, };
	GetPrivateProfileString("FAIL_NOTI", "NOTI_ENABLED", "0", szEnabled, sizeof(szEnabled), PARAINI);

	if (strcmp(szEnabled, "1") != 0) {
		eprintf("[FAIL_NOTI] 노티 전송 비활성화 상태 (NOTI_ENABLED=%s)", szEnabled);
		return 0;  // 비활성화 상태
	}

	// [2] 노티 URL 읽기
	char szNotiUrl[512 + 1] = { 0x00, };
	GetPrivateProfileString("FAIL_NOTI", "NOTI_URL", "", szNotiUrl, sizeof(szNotiUrl), PARAINI);

	if (strlen(szNotiUrl) == 0) {
		eprintf("[FAIL_NOTI] 노티 URL 미설정 오류");
		return -1;  // URL 미설정
	}

	// [3] 타임아웃 설정 읽기
	int nTimeout = GetPrivateProfileInt("FAIL_NOTI", "NOTI_TIMEOUT", 5000, PARAINI);
	if (nTimeout < 1000) nTimeout = 1000;    // 최소 1초
	if (nTimeout > 30000) nTimeout = 30000;  // 최대 30초

	eprintf("[FAIL_NOTI] 노티 전송 시작 - URL:%s, TIMEOUT:%d", szNotiUrl, nTimeout);

	// [4] JSON 데이터 생성 - resMsg EUC-KR→UTF-8 변환 후 이스케이프 처리
	// [MODIFIED] KICC API 응답이 EUC-KR이므로 UTF-8로 변환
	char szUtf8Msg[512] = { 0x00, };
	int nConvLen = ConvertEucKrToUtf8(pCardResInfo->REPLY_MESSAGE, szUtf8Msg, sizeof(szUtf8Msg));
	if (nConvLen < 0) {
		eprintf("[FAIL_NOTI] UTF-8 변환 실패, 원본 사용");
		strncpy_s(szUtf8Msg, sizeof(szUtf8Msg), pCardResInfo->REPLY_MESSAGE, _TRUNCATE);
	}

	char szEscapedMsg[512] = { 0x00, };
	char* pSrc = szUtf8Msg;  // UTF-8 변환된 문자열 사용
	char* pDst = szEscapedMsg;
	int nDstLen = sizeof(szEscapedMsg) - 1;

	// JSON 특수문자 이스케이프 처리 (", \, 제어문자)
	while (*pSrc && (pDst - szEscapedMsg) < nDstLen - 2) {
		if (*pSrc == '"' || *pSrc == '\\') {
			*pDst++ = '\\';
		}
		else if (*pSrc == '\n') {
			*pDst++ = '\\';
			*pDst++ = 'n';
			pSrc++;
			continue;
		}
		else if (*pSrc == '\r') {
			*pDst++ = '\\';
			*pDst++ = 'r';
			pSrc++;
			continue;
		}
		else if (*pSrc == '\t') {
			*pDst++ = '\\';
			*pDst++ = 't';
			pSrc++;
			continue;
		}
		*pDst++ = *pSrc++;
	}
	*pDst = '\0';

	// [MODIFIED] JSON 문자열 생성 - 필수 4개 필드만 전송
	char szJsonData[1024] = { 0x00, };
	sprintf_s(szJsonData, sizeof(szJsonData),
		"{"
		"\"resCd\":\"%s\","
		"\"resMsg\":\"%s\","
		"\"mallId\":\"%s\","
		"\"shopOrderNo\":\"%s\""
		"}",
		pCardResInfo->REPLY_CODE,       // resCd - 응답코드
		szEscapedMsg,                   // resMsg - 응답메시지 (이스케이프됨)
		pCardResInfo->TERMINAL_ID,      // mallId - 가맹점ID
		pCardResInfo->ORDER_NO          // shopOrderNo - 주문번호
	);

	eprintf("[FAIL_NOTI] JSON 데이터 생성 완료 (길이:%d)", strlen(szJsonData));
	eprintf("[FAIL_NOTI] JSON: %s", szJsonData);

	// [6] URL 파싱 (서버, 포트, 경로 분리)
	CString strUrl(szNotiUrl);
	CString strServer, strPath;
	INTERNET_PORT nPort = INTERNET_DEFAULT_HTTP_PORT;
	DWORD dwServiceType = AFX_INET_SERVICE_HTTP;

	// URL 파싱
	if (!AfxParseURL(strUrl, dwServiceType, strServer, strPath, nPort)) {
		eprintf("[FAIL_NOTI] URL 파싱 실패: %s", szNotiUrl);
		return -2;  // URL 파싱 실패
	}

	// HTTPS 처리
	DWORD dwFlags = 0;
	if (dwServiceType == AFX_INET_SERVICE_HTTPS) {
		nPort = (nPort == INTERNET_DEFAULT_HTTP_PORT) ? INTERNET_DEFAULT_HTTPS_PORT : nPort;
		dwFlags = INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
	}

	eprintf("[FAIL_NOTI] URL 파싱 결과 - Server:%s, Port:%d, Path:%s, HTTPS:%d",
		strServer.GetBuffer(), nPort, strPath.GetBuffer(), (dwServiceType == AFX_INET_SERVICE_HTTPS) ? 1 : 0);

	// [7] HTTP POST 전송
	int nResult = 1;  // 기본값: 성공
	CInternetSession* pSession = NULL;
	CHttpConnection* pConnection = NULL;
	CHttpFile* pFile = NULL;

	try {
		// 세션 생성
		pSession = new CInternetSession(_T("KICC_FailNoti"), 1, INTERNET_OPEN_TYPE_PRECONFIG);
		pSession->SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, nTimeout);
		pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, nTimeout);
		pSession->SetOption(INTERNET_OPTION_SEND_TIMEOUT, nTimeout);

		// HTTP 연결
		pConnection = pSession->GetHttpConnection(strServer, nPort);

		// HTTP 요청 생성
		pFile = pConnection->OpenRequest(
			CHttpConnection::HTTP_VERB_POST,
			strPath,
			NULL,
			1,
			NULL,
			NULL,
			dwFlags
		);

		// Content-Type 헤더 추가
		CString strHeaders = _T("Content-Type: application/json; charset=utf-8\r\n");

		// 요청 전송
		BOOL bResult = pFile->SendRequest(strHeaders, (LPVOID)szJsonData, (DWORD)strlen(szJsonData));

		if (!bResult) {
			eprintf("[FAIL_NOTI] HTTP 요청 전송 실패");
			nResult = -3;
		}
		else {
			// HTTP 응답 코드 확인
			DWORD dwStatusCode = 0;
			pFile->QueryInfoStatusCode(dwStatusCode);

			eprintf("[FAIL_NOTI] HTTP 응답 코드: %d", dwStatusCode);

			if (dwStatusCode >= 200 && dwStatusCode < 300) {
				// 성공 (2xx)
				eprintf("[FAIL_NOTI] 노티 전송 성공");
				nResult = 1;
			}
			else {
				// HTTP 오류
				eprintf("[FAIL_NOTI] HTTP 응답 오류 (코드:%d)", dwStatusCode);
				nResult = -3;
			}
		}
	}
	catch (CInternetException* pEx) {
		// 네트워크 예외 처리
		TCHAR szError[256] = { 0x00, };
		pEx->GetErrorMessage(szError, 255);
		eprintf("[FAIL_NOTI] 네트워크 예외 발생: %s", szError);
		pEx->Delete();
		nResult = -4;
	}
	catch (...) {
		eprintf("[FAIL_NOTI] 알 수 없는 예외 발생");
		nResult = -4;
	}

	// [8] 리소스 정리
	if (pFile) {
		pFile->Close();
		delete pFile;
	}
	if (pConnection) {
		pConnection->Close();
		delete pConnection;
	}
	if (pSession) {
		delete pSession;
	}

	eprintf("[FAIL_NOTI] 노티 전송 완료 - 결과:%d", nResult);
	return nResult;
}
