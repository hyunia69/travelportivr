// ADODB.cpp: implementation of the CADODB class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CommonDef.H"
#include "KICC_Common.h"

#include    "ADODB.h"
#include    "Scenaio.h"
#include    "KICC_Travelport_Scenario.h"


extern void(*eprintf)(const char *str, ...);
extern LPMTP **lpmt , **port;

//LPMTP	*curyport=NULL;
extern void(*info_printf)(int chan, const char *str, ...) ;
extern void(*new_guide)(void) ;
extern int(*set_guide)(int vid, ...);
extern void(*setPostfunc)(int type, int(*func)(int), int poststate, int wtime);
extern int(*send_guide)(int mode);
extern int(*goto_hookon)(void);
extern int(*check_validform)(char *form, char *data);
extern int(*send_error)(void);
extern int(*check_validdtmf)(int c, char *vkeys);
extern int(*in_multifunc)(int chan);
extern int  (*quitchan)(int chan);

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


#define DATABASE_SESSION "DATABASE"
#define DATABASE_IP_URL "arsdb.mediaford.co.kr,1433"
#define DATABASE  "arspg_web"
#define SID       "sa"
#define PASSWORD  "medi@ford"

#define KICC_MAII_NM "옴니텔"
#define KICC_MAII_ID "T5102001"

#define CALLBACK_NO "0234904411"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CADODB::CADODB()
{
	m_RS = NULL;
	m_CMD = NULL;
	m_CONN = NULL;
	SetScenarion(NULL);
}

void CADODB::SetScenarion(CKICC_Scenario  *pScenario)
{
	m_pScenario = pScenario;
}

CADODB::CADODB(CKICC_Scenario  *pScenario)
{
    m_RS = NULL;
    m_CMD = NULL;
    m_CONN = NULL;

	SetScenarion(pScenario);
}

CADODB::~CADODB()
{
    
    if(m_RS != NULL)
    {
        if(ISRSCon())
        {
            m_RS->Close();
        }
    }

    if(m_CONN != NULL)
    {
        if(ISOpen())
        {
            m_CONN->Close();
        }
    }
}

BOOL CADODB::DBConnect(char* pWD, char* pID, char* pDataBase, char* pConnectIP)
{
	char strConnectionString[1024];
	strcpy(strConnectionString, "Provider=SQLOLEDB.1;Password=");
	strcat(strConnectionString, pWD);
	strcat(strConnectionString, ";Persist Security Info=True;User ID=");
	strcat(strConnectionString, pID);
	strcat(strConnectionString, ";Initial Catalog=");
	strcat(strConnectionString, pDataBase);
	strcat(strConnectionString, ";Data Source=");
	strcat(strConnectionString, pConnectIP);
	strcat(strConnectionString, "; Network Library = dbmssocn");
    
    m_CONN.CreateInstance("ADODB.Connection");
    m_CONN->ConnectionString = strConnectionString;
  //HRESULT hr = m_CONN->Open(strConnectionString, "", "", -1);
    HRESULT hr = m_CONN->Open("", "", "", -1);

    if(SUCCEEDED(hr))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL CADODB::ISRSCon()
{
    return ((m_RS->GetState() == adStateOpen ) ? TRUE : FALSE);
}

BOOL CADODB::ISOpen()
{
    return ((m_CONN->GetState() == adStateOpen ) ? TRUE : FALSE);
}

BOOL CADODB::GetDBCon()
{
    return ISOpen();
}

void CADODB::ConClose()
{
    if(ISOpen())
    {
        m_CONN->Close();
    }
}

long CADODB::ConBeginTrans()
{
    return m_CONN->BeginTrans();
}

void CADODB::ConCommitTrans()
{
    m_CONN->CommitTrans();
}

void CADODB::ConRollbackTrans()
{
    m_CONN->RollbackTrans();
}

void CADODB::ConCancel()
{
    m_CONN->Cancel();
}

BOOL CADODB::IsEOF()
{
    return m_RS->adoEOF;
}


BOOL CADODB::Next()
{
    return (FAILED(m_RS->MoveNext()) ? FALSE : TRUE);
}

BOOL CADODB::Prev()
{
    return (FAILED(m_RS->MovePrevious()) ? FALSE : TRUE);
}

BOOL CADODB::First()
{
    return (FAILED(m_RS->MoveFirst()) ? FALSE : TRUE);
}

BOOL CADODB::Last()
{
    return (FAILED(m_RS->MoveLast()) ? FALSE : TRUE);
}

int CADODB::GetRecCount()
{
	HRESULT hr;
    ASSERT(m_RS != NULL);
	try
	{
		int count = (int)m_RS->GetRecordCount();

		if (count > 0)
		{
			hr = m_RS->MoveFirst();
			if (!SUCCEEDED(hr))
			{
				return -1;
			}
			count = 0;
			while (!m_RS->adoEOF)
			{
				count++;
				hr = m_RS->MoveNext();
				if (!SUCCEEDED(hr))
				{
					return count;
				}
			}

			if (m_RS->adoEOF)
			{
				hr = m_RS->MoveFirst();
				if (!SUCCEEDED(hr))
				{
					return -1;
				}
			}

			eprintf("SUCCESS: GetRecordCount  %d:\n", count);
		}
		else
			eprintf("Warning: GetRecordCount  %d; File: %s; Line: %d\n", count, __FILE__, __LINE__);
		return count;
	}
	catch (_com_error e)
	{
		PrintProviderError();
		PrintComError(e);
		return -1;
	} 
}

int CADODB::GetFieldCount()
{
    return (int)m_RS->Fields->GetCount();
}

void CADODB::RSClose()
{
    if(ISRSCon())
    {
        m_RS->Close();
    }
}

///////////////////////////////////////////////////////////
//                                                       //
//      PrintComError Function                           //
//                                                       //
///////////////////////////////////////////////////////////
void CADODB::PrintComWARNING(_com_error &e)
{
	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());

	// Print Com errors.
	eprintf("WARNING");
	eprintf("\tWARNING_Code = %08lx", e.Error());
	eprintf("\tWARNING_Code meaning = %s", e.ErrorMessage());
	eprintf("\tWARNING_Source = %s", (LPCSTR)bstrSource);
	eprintf("\tWARNING_Description = %s", (LPCSTR)bstrDescription);
}

void CADODB::GetRs(_variant_t x, _bstr_t& ret)
{
	try {
		_variant_t vtNull;
		vtNull.ChangeType(VT_NULL);
		ret = (m_RS->Fields->Item[x]->Value != vtNull) ? m_RS->Fields->Item[x]->Value : "";
	}
	catch (_com_error e)
	{
		_bstr_t bstrColName = x;
		LPCSTR strColName = bstrColName;
		eprintf("WARNING:(%s)", strColName);

		PrintProviderError();
		PrintComWARNING(e);
		ret = "";
	}
}

void CADODB::GetRs(_variant_t x, _variant_t& ret)
{
	try {
		_variant_t vtNull;
		vtNull.ChangeType(VT_NULL);
		ret = (m_RS->Fields->Item[x]->Value != vtNull) ? m_RS->Fields->Item[x]->Value : "";
	}
	catch (_com_error e)
	{
		_bstr_t bstrColName = x;
		LPCSTR strColName = bstrColName;
		eprintf("WARNING:(%s)", strColName);

		PrintProviderError();
		PrintComWARNING(e);
		ret = "";
	}
}

void CADODB::GetRs(_variant_t x, float& ret)
{
	try {
		_variant_t vtNull;
		vtNull.ChangeType(VT_NULL);
		ret = (m_RS->Fields->Item[x]->Value != vtNull) ? m_RS->Fields->Item[x]->Value : 0.0;
	}
	catch (_com_error e)
	{
		_bstr_t bstrColName = x;
		LPCSTR strColName = bstrColName;
		eprintf("WARNING:(%s)", strColName);

		PrintProviderError();
		PrintComWARNING(e);
		ret = 0;
	}
}

void CADODB::GetRs(_variant_t x, long& ret)
{
	try {
		_variant_t vtNull;
		vtNull.ChangeType(VT_NULL);
		ret = (m_RS->Fields->Item[x]->Value != vtNull) ? m_RS->Fields->Item[x]->Value : 0;
	}
	catch (_com_error e)
	{
		_bstr_t bstrColName = x;
		LPCSTR strColName = bstrColName;
		eprintf("WARNING:(%s)", strColName);

		PrintProviderError();
		PrintComWARNING(e);
		ret = 0;
	}
}

void CADODB::GetRs(_variant_t x, double& ret)
{
	try {
		_variant_t vtNull;
		vtNull.ChangeType(VT_NULL);
		ret = (m_RS->Fields->Item[x]->Value != vtNull) ? m_RS->Fields->Item[x]->Value : 0;
	}
	catch (_com_error e)
	{
		_bstr_t bstrColName = x;
		LPCSTR strColName = bstrColName;
		eprintf("WARNING:(%s)", strColName);

		PrintProviderError();
		PrintComWARNING(e);
		ret = 0;
	}
}

BOOL CADODB::Open(char* pSourceBuf, long option)
{
    if(ISOpen())
    {
        m_RS.CreateInstance(__uuidof(Recordset));
        m_RS->PutRefActiveConnection(m_CONN);
        
        HRESULT hr;
        m_RS->CursorType = adOpenStatic;
        hr = m_RS->Open(pSourceBuf, (IDispatch *)m_CONN, adOpenStatic, adLockOptimistic, option);
        
        if(SUCCEEDED(hr))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

BOOL CADODB::Excute(char* pSourceBuf, long option)
{
    if(ISOpen())
    {
        m_CMD.CreateInstance(__uuidof(Command));
        m_CMD->ActiveConnection = m_CONN;
        m_CMD->CommandText = pSourceBuf;
        m_CMD->Execute(NULL, NULL, adCmdText);
        
        m_RS.CreateInstance(__uuidof(Recordset));
        m_RS->PutRefSource(m_CMD);
        
        _variant_t vNull;
        vNull.vt = VT_ERROR;
        vNull.scode = DISP_E_PARAMNOTFOUND;
        m_RS->CursorLocation = adUseClient;
        m_RS->Open(vNull, vNull, adOpenDynamic, adLockOptimistic, adCmdUnknown);
        
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


///////////////////////////////////////////////////////////
//                                                       //
//      PrintProviderError Function                      //
//                                                       //
///////////////////////////////////////////////////////////
void CADODB::PrintProviderError()
{
	// Print Provider Errors from Connection object.
	// pErr is a record object in the Connection's Error collection.
	ErrorPtr  pErr = NULL;

	if ((m_CONN->Errors->Count) > 0)
	{
		long nCount = m_CONN->Errors->Count;

		// Collection ranges from 0 to nCount -1.
		for (long i = 0; i < nCount; i++)
		{
			pErr = m_CONN->Errors->GetItem(i);
			eprintf("Error number: %x\t%s\n", pErr->Number, (LPCSTR)pErr->Description);
		}
	}
}

///////////////////////////////////////////////////////////
//                                                       //
//      PrintComError Function                           //
//                                                       //
///////////////////////////////////////////////////////////
void CADODB::PrintComError(_com_error &e)
{
	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());

	// Print Com errors.
	eprintf("Error");
	eprintf("\tCode = %08lx", e.Error());
	eprintf("\tCode meaning = %s", e.ErrorMessage());
	eprintf("\tSource = %s", (LPCSTR)bstrSource);
	eprintf("\tDescription = %s", (LPCSTR)bstrDescription);
}


BOOL CADODB::upOrderPayState(char *sxResultCode, char *szResultMsg, char *szMoid , char *szMid)
{
	if (ISOpen())
	{
		_CommandPtr& cmd = m_CMD;
		cmd.CreateInstance("ADODB.Command");
		CString strSql;

		char szCALLBACK_NO[15 + 1] = { 0x00, };
		CString strCALLBACK_NO;

		GetPrivateProfileString("SMS_SEND", "CALLBACK_NO", CALLBACK_NO, szCALLBACK_NO, sizeof(szCALLBACK_NO), PARAINI);
		strCALLBACK_NO = szCALLBACK_NO;

		//"MX_SEQ" INT NOT NULL DEFAULT NULL,

		strSql.Format("UPDATE KICC_SHOP_ORDER \n"
			          "SET reply_code = \'%s\'\n"
					  "  , reply_message = \'%s\' \n"
					  "  , payment_code = \'1\' \n"
					  "WHERE order_no = \'%s\'\n"
					  "AND terminal_id = \'%s\';"
					  , sxResultCode, szResultMsg, szMoid, szMid);

		cmd->CommandText = strSql.AllocSysString();
		cmd->CommandType = adCmdText;

		cmd->ActiveConnection = m_CONN;
		try{
			cmd->Execute(NULL, NULL, adCmdText);
		}
		catch (_com_error e)
		{
			PrintProviderError();
			PrintComError(e);
			return FALSE;
		}
		//*Return_INPUT_Error_NUM = long(cmd->Parameters->Item[L"@DB_ERR"]->Value);
		//*Return_iMageNUM = long(cmd->Parameters->Item[L"@iMageNUM_1"]->Value);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CADODB::SMS_Send(CString strOrder_no, CString strPhone_no, CString szCunt_nm, CString szProdo_nm, int nAmount, CString Rs_Id)
{
	if (ISOpen())
	{
		_CommandPtr& cmd = m_CMD;
		cmd.CreateInstance("ADODB.Command");
		CString strSql;

		char szCALLBACK_NO[15 + 1] = { 0x00, };
		CString strCALLBACK_NO;

		GetPrivateProfileString("SMS_SEND", "CALLBACK_NO", CALLBACK_NO, szCALLBACK_NO, sizeof(szCALLBACK_NO), PARAINI);
		strCALLBACK_NO = szCALLBACK_NO;

		strSql.Format( "insert into kb_tran\n"
		"(tran_phone, tran_callback, tran_msg, tran_status, tran_type, tran_date)\n"
		"values(\'%s\', \'%s\' , \'[결제완료] 고객명: %s, 상품명: %s , 금액: %d 원\', \'1\', \'0\', getdate())", strPhone_no, strCALLBACK_NO,
		szCunt_nm, szProdo_nm, nAmount);

		// 6월 29일 이후는 아래 메시지로 변경해야 함. by AHN 
		// 오션월드구매 감사합니다. 입장권은 일2회(11시,5시)순차 발송됩니다 
		/*strSql.Format("insert into em_smt_tran\n"
			"(mt_refkey,recipient_num, callback, content, msg_status, service_type, date_client_req,rs_id,broadcast_yn)\n"
			"values(\'%s\', \'%s\', \'%s\' , \'오션월드구매 감사합니다. 입장권은 일2회(11시,5시)순차 발송됩니다.\', \'1\', \'0\', getdate(),\'%s\',\'N\');"
			, strOrder_no, strPhone_no, strCALLBACK_NO, Rs_Id);*/

		cmd->CommandText = strSql.AllocSysString();
		cmd->CommandType = adCmdText;

		cmd->ActiveConnection = m_CONN;
		try{
			cmd->Execute(NULL, NULL, adCmdText);
		}
		catch (_com_error e)
		{
			PrintProviderError();
			PrintComError(e);
			return FALSE;
		}
		//*Return_INPUT_Error_NUM = long(cmd->Parameters->Item[L"@DB_ERR"]->Value);
		//*Return_iMageNUM = long(cmd->Parameters->Item[L"@iMageNUM_1"]->Value);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CADODB::setPayLog(Card_ResInfo ag_Card_ResInfo)
{
	if (ISOpen())
	{
		_CommandPtr& cmd = m_CMD;
		cmd.CreateInstance("ADODB.Command");
		CString strSql;
		eprintf("setPayLog===> ORDER_NO  : %s", ag_Card_ResInfo.ORDER_NO);

		strSql.Format("insert into KICC_PAY_LOG\n"
			" (terminal_id, order_no, control_no, payment_type, amount, installment, \n"
			" approval_num, approval_date, reply_code , reply_message, reply_date)  \n"
			"VALUES ( \'%s\', \'%s\', \'%s\', \'%s\', \'%d\', \'%s\' , \n"
			" \'%s\',\'%s\',\'%s\',\'%s\',getdate()) ;" ,
			TEXT(ag_Card_ResInfo.TERMINAL_ID), TEXT(ag_Card_ResInfo.ORDER_NO), TEXT(ag_Card_ResInfo.CONTROL_NO),"1", ag_Card_ResInfo.AMOUNT,TEXT(ag_Card_ResInfo.InstPeriod) ,
			TEXT(ag_Card_ResInfo.APPROVAL_NUM), TEXT(ag_Card_ResInfo.APPROVAL_DATE),TEXT(ag_Card_ResInfo.REPLY_CODE), TEXT(ag_Card_ResInfo.REPLY_MESSAGE));


		cmd->CommandText = strSql.AllocSysString();
		cmd->CommandType = adCmdText;

		cmd->ActiveConnection = m_CONN;
		try{
			cmd->Execute(NULL, NULL, adCmdText);
		}
		catch (_com_error e)
		{
			PrintProviderError();
			PrintComError(e);
			return FALSE;
		}
		//*Return_INPUT_Error_NUM = long(cmd->Parameters->Item[L"@DB_ERR"]->Value);
		//*Return_iMageNUM = long(cmd->Parameters->Item[L"@iMageNUM_1"]->Value);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

int CADODB::sp_getKiccSMSOrderInfoByTel2(CString szDnis, CString szAuthNo)
{
	if (ISOpen())
	{
		_ParameterPtr pParam;
		_CommandPtr& cmd = m_CMD;
		//2016.05.23
		//try 위치 및 범위 넓게 하자!!!
		try{
			cmd.CreateInstance("ADODB.Command");
			cmd->CommandText = "dbo.sp_getKiccOrderInfoBySMS2";
			cmd->CommandType = adCmdStoredProc;

			pParam.CreateInstance("ADODB.Parameter");
			pParam->Name = L"@AUTH_NO";
			pParam->Type = adVarChar;
			pParam->Size = 12;
			pParam->Direction = adParamInput;
			cmd->Parameters->Append(pParam);
			cmd->Parameters->Item[L"@AUTH_NO"]->Value = _variant_t(szAuthNo.GetBuffer(pParam->Size));// 주문 번호


			pParam.CreateInstance("ADODB.Parameter");
			pParam->Name = L"@DNIS";
			pParam->Type = adVarChar;
			pParam->Size = 12;
			pParam->Direction = adParamInput;
			cmd->Parameters->Append(pParam);
			cmd->Parameters->Item[L"@DNIS"]->Value = _variant_t(szDnis.GetBuffer(pParam->Size));

			cmd->ActiveConnection = m_CONN;
		//try{
			cmd->Execute(NULL, NULL, adCmdStoredProc);
		}
		catch (_com_error e)
		{
			PrintProviderError();
			PrintComError(e);
			return FALSE;
		}
		m_RS.CreateInstance(__uuidof(Recordset));
		m_RS->PutRefSource(cmd);

		_variant_t vNull;
		vNull.vt = VT_ERROR;
		vNull.scode = DISP_E_PARAMNOTFOUND;
		m_RS->CursorLocation = adUseClient;

		try{
			m_RS->Open(vNull, vNull, adOpenStatic, adLockOptimistic, adCmdStoredProc);
		}
		catch (_com_error e)
		{
			PrintProviderError();
			PrintComError(e);
			return FALSE;
		}

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

int CADODB::sp_getKiccOrderInfoByTel2(CString szDnis, CString szInputTelNum)
{
	if (ISOpen())
	{
		_ParameterPtr pParam;
		_CommandPtr& cmd = m_CMD;

		//2016.05.23
		//try 위치 및 범위 넓게 하자!!!
		try{
			cmd.CreateInstance("ADODB.Command");
			cmd->CommandText = "dbo.sp_getKiccOrderInfoByTel2";
			cmd->CommandType = adCmdStoredProc;

			pParam.CreateInstance("ADODB.Parameter");
			pParam->Name = L"@PHONE_NO";
			pParam->Type = adVarChar;
			pParam->Size = 32;//12자리에서 32 자리로 수정 2016.11.29 인입호가 해외의 번호의 것을 수용하기 위해 수정
			pParam->Direction = adParamInput;
			cmd->Parameters->Append(pParam);
			cmd->Parameters->Item[L"@PHONE_NO"]->Value = _variant_t(szInputTelNum.GetBuffer(pParam->Size));// 전화 번호

			pParam.CreateInstance("ADODB.Parameter");
			pParam->Name = L"@DNIS";
			pParam->Type = adVarChar;
			pParam->Size = 12;
			pParam->Direction = adParamInput;
			cmd->Parameters->Append(pParam);
			cmd->Parameters->Item[L"@DNIS"]->Value = _variant_t(szDnis.GetBuffer(pParam->Size));

			cmd->ActiveConnection = m_CONN;
		//try{
			cmd->Execute(NULL, NULL, adCmdStoredProc);
		}
		catch (_com_error e)
		{
			PrintProviderError();
			PrintComError(e);
			return FALSE;
		}
		m_RS.CreateInstance(__uuidof(Recordset));
		m_RS->PutRefSource(cmd);

		_variant_t vNull;
		vNull.vt = VT_ERROR;
		vNull.scode = DISP_E_PARAMNOTFOUND;
		m_RS->CursorLocation = adUseClient;

		try{
			m_RS->Open(vNull, vNull, adOpenStatic, adLockOptimistic, adCmdStoredProc);
		}
		catch (_com_error e)
		{
			PrintProviderError();
			PrintComError(e);
			return FALSE;
		}

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// [다중 주문 지원 - 신규]
int CADODB::sp_getKiccMultiOrderInfo(CString szDnis, CString szPhoneNo)
{
	if (ISOpen())
	{
		_ParameterPtr pParam;
		_CommandPtr& cmd = m_CMD;

		//2016.05.23
		//try 위치 및 범위 넓게 하자!!!
		try{
			cmd.CreateInstance("ADODB.Command");
			cmd->CommandText = "dbo.sp_getKiccMultiOrderInfo";
			cmd->CommandType = adCmdStoredProc;

			pParam.CreateInstance("ADODB.Parameter");
			pParam->Name = L"@PHONE_NO";
			pParam->Type = adVarChar;
			pParam->Size = 32;
			pParam->Direction = adParamInput;
			cmd->Parameters->Append(pParam);
			cmd->Parameters->Item[L"@PHONE_NO"]->Value = _variant_t(szPhoneNo.GetBuffer(pParam->Size));// 전화 번호

			pParam.CreateInstance("ADODB.Parameter");
			pParam->Name = L"@DNIS";
			pParam->Type = adVarChar;
			pParam->Size = 12;
			pParam->Direction = adParamInput;
			cmd->Parameters->Append(pParam);
			cmd->Parameters->Item[L"@DNIS"]->Value = _variant_t(szDnis.GetBuffer(pParam->Size));

			cmd->ActiveConnection = m_CONN;
		//try{
			cmd->Execute(NULL, NULL, adCmdStoredProc);
		}
		catch (_com_error e)
		{
			PrintProviderError();
			PrintComError(e);
			return FALSE;
		}
		m_RS.CreateInstance(__uuidof(Recordset));
		m_RS->PutRefSource(cmd);

		_variant_t vNull;
		vNull.vt = VT_ERROR;
		vNull.scode = DISP_E_PARAMNOTFOUND;
		m_RS->CursorLocation = adUseClient;

		try{
			m_RS->Open(vNull, vNull, adOpenStatic, adLockOptimistic, adCmdStoredProc);
		}
		catch (_com_error e)
		{
			PrintProviderError();
			PrintComError(e);
			return FALSE;
		}

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CADODB::FetchMultiOrderResults(MultiOrderInfo* pMultiOrders)
{
	if (!pMultiOrders || !m_RS || m_RS->adoEOF) return FALSE;

	int nCount = 0;
	memset(pMultiOrders, 0x00, sizeof(MultiOrderInfo));

	try {
		_variant_t bt = "";
		char szAmount[10 + 1];
		char szAuthNo[12 + 1];

		while (!m_RS->adoEOF && nCount < MAX_ORDERS) {
			Card_ResInfo* pOrder = &(pMultiOrders->orders[nCount]);

			// 모든 필드 조회
			memset(pOrder, 0x00, sizeof(Card_ResInfo));

			bt = "";
			GetRs(_variant_t(L"order_no"), bt);
			strncpy(pOrder->ORDER_NO, (char*)(_bstr_t)bt, sizeof(pOrder->ORDER_NO) - 1);

			bt = "";
			GetRs(_variant_t(L"terminal_nm"), bt);
			strncpy(pOrder->TERMINAL_NM, (char*)(_bstr_t)bt, sizeof(pOrder->TERMINAL_NM) - 1);

			bt = "";
			GetRs(_variant_t(L"terminal_id"), bt);
			strncpy(pOrder->TERMINAL_ID, (char*)(_bstr_t)bt, sizeof(pOrder->TERMINAL_ID) - 1);

			bt = "";
			GetRs(_variant_t(L"terminal_pw"), bt);
			strncpy(pOrder->TERMINAL_PW, (char*)(_bstr_t)bt, sizeof(pOrder->TERMINAL_PW) - 1);

			bt = "";
			GetRs(_variant_t(L"cust_nm"), bt);
			strncpy(pOrder->CUST_NM, (char*)(_bstr_t)bt, sizeof(pOrder->CUST_NM) - 1);

			bt = "";
			GetRs(_variant_t(L"good_nm"), bt);
			strncpy(pOrder->GOOD_NM, (char*)(_bstr_t)bt, sizeof(pOrder->GOOD_NM) - 1);

			memset(szAmount, 0x00, sizeof(szAmount));
			bt = "";
			GetRs(_variant_t(L"amount"), bt);
			strncpy(szAmount, (char*)(_bstr_t)bt, sizeof(szAmount) - 1);
			pOrder->TOTAMOUNT = atoi(szAmount);
			pMultiOrders->nTotalAmount += pOrder->TOTAMOUNT;

			bt = "";
			GetRs(_variant_t(L"phone_no"), bt);
			strncpy(pOrder->PHONE_NO, (char*)(_bstr_t)bt, sizeof(pOrder->PHONE_NO) - 1);

			bt = "";
			GetRs(_variant_t(L"SHOP_PW"), bt);
			strncpy(pOrder->RESERVED_2, (char*)(_bstr_t)bt, sizeof(pOrder->RESERVED_2) - 1);  // SHOP_PW

			bt = "";
			GetRs(_variant_t(L"ADMIN_ID"), bt);
			strncpy(pOrder->ADMIN_ID, (char*)(_bstr_t)bt, sizeof(pOrder->ADMIN_ID) - 1);

			// AUTH_NO 필드 조회 (로깅용)
			memset(szAuthNo, 0x00, sizeof(szAuthNo));
			bt = "";
			GetRs(_variant_t(L"AUTH_NO"), bt);
			strncpy(szAuthNo, (char*)(_bstr_t)bt, sizeof(szAuthNo) - 1);

			// RESERVED 필드들
			bt = "";
			GetRs(_variant_t(L"RESERVED_3"), bt);
			strncpy(pOrder->RESERVED_3, (char*)(_bstr_t)bt, sizeof(pOrder->RESERVED_3) - 1);

			// RESERVED_4: 카드번호 앞자리 (12자리)
			char szReserved4[32 + 1] = { 0x00 };
			bt = "";
			GetRs(_variant_t(L"RESERVED_4"), bt);
			strncpy(szReserved4, (char*)(_bstr_t)bt, sizeof(szReserved4) - 1);

			// RESERVED_5: 할부개월 (2자리)
			char szReserved5[32 + 1] = { 0x00 };
			bt = "";
			GetRs(_variant_t(L"RESERVED_5"), bt);
			strncpy(szReserved5, (char*)(_bstr_t)bt, sizeof(szReserved5) - 1);

			// 첫 번째 주문의 DB 카드 정보를 시나리오에 저장 (모든 주문이 동일한 카드 정보를 사용)
			if (nCount == 0 && m_pScenario) {
				CKICC_Scenario* pScenario = (CKICC_Scenario*)m_pScenario;
				
				// AUTH_NO 저장
				strncpy(pScenario->m_szAuthNo, szAuthNo, sizeof(pScenario->m_szAuthNo) - 1);
				
				// RESERVED_3: 유효기간 (YYMM, 4자리)
				strncpy(pScenario->m_szDB_ExpireDate, pOrder->RESERVED_3, sizeof(pScenario->m_szDB_ExpireDate) - 1);
				
				// RESERVED_4: 카드번호 앞자리 (12자리)
				strncpy(pScenario->m_szDB_CardPrefix, szReserved4, sizeof(pScenario->m_szDB_CardPrefix) - 1);
				
				// RESERVED_5: 할부개월 (2자리)
				strncpy(pScenario->m_szDB_InstallPeriod, szReserved5, sizeof(pScenario->m_szDB_InstallPeriod) - 1);
				
				// DB 카드 정보 사용 플래그 설정
				if (strlen(pScenario->m_szDB_CardPrefix) == 12 && 
					strlen(pScenario->m_szDB_ExpireDate) == 4) {
					pScenario->m_bUseDbCardInfo = TRUE;
					
					eprintf("[KICC] 다중주문 DB 카드정보 로드: 카드앞자리:%s, 유효기간:%s, 할부:%s",
						pScenario->m_szDB_CardPrefix,
						pScenario->m_szDB_ExpireDate,
						pScenario->m_szDB_InstallPeriod);
				}
				else {
					pScenario->m_bUseDbCardInfo = FALSE;
					eprintf("[KICC] 다중주문 DB 카드정보 불완전: 카드앞자리길이:%d, 유효기간길이:%d",
						strlen(pScenario->m_szDB_CardPrefix),
						strlen(pScenario->m_szDB_ExpireDate));
				}
			}

			// 주문 정보 로그 출력
			if (m_pScenario) {
				eprintf("[주문조회] 주문번호:%s, 상품명:%s, AUTH_NO:%s, 금액:%d",
					pOrder->ORDER_NO,
					pOrder->GOOD_NM,
					szAuthNo,
					pOrder->TOTAMOUNT);
			}

			nCount++;
			if (!m_RS->adoEOF) {
				m_RS->MoveNext();
			}
		}

		pMultiOrders->nOrderCount = nCount;
		return TRUE;
	}
	catch (_com_error e) {
		PrintComError(e);
		return FALSE;
	}
}

/* ------------------------------------------------------------------------ */
void ADO_Quithostio(char *p, int ch)
{
	CKICC_Scenario *pScenario = (CKICC_Scenario *)((*port)[ch].pScenario);


	eprintf("ADO_Quithostio===START");

	if (pScenario->m_AdoDb != NULL)
	{
		delete pScenario->m_AdoDb;
		pScenario->m_AdoDb  = NULL;
	}
	CoUninitialize();
	(*port)[ch].ppftbl[POST_NET].postcode = HI_OK;
	if ((*port)[ch].used != L_IDLE && in_multifunc(ch))
		quitchan(ch);

	eprintf("%s", p);
	eprintf("ADO_Quithostio _endthread");
}


unsigned int __stdcall upOrderPayStateProc(void *data)
{
	int			ch;
	int			threadID;
	char        AnsiNO_Char[257] = { 0x00, };
	LPMTP		*lineTablePtr = (LPMTP *)data;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)(lineTablePtr->pScenario);

	ch = lineTablePtr->chanID;
	threadID = pScenario->threadID;

	eprintf("upOrderPaySatetProc START");

	CoInitialize(0);
	pScenario->m_AdoDb = new CADODB(pScenario);

	//리턴값
	//=====================
	//쿼리인자변수선언
	//=====================
	pScenario->m_PayResult = 0;
	if (threadID != pScenario->threadID) {
		pScenario->m_PayResult = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("upOrderPaySatetProc the line service is not valid any more.", ch);
		eprintf("upOrderPaySatetProc END");
		_endthreadex((unsigned int)pScenario->m_hThread);
		return 0;
	}
	if (pScenario->m_AdoDb == NULL){
		pScenario->m_PayResult = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("upOrderPaySatetProc the line service Object is Null.", ch);
		eprintf("upOrderPaySatetProc END");
		_endthreadex((unsigned int)pScenario->m_hThread);
		return 0;
	}

	char szDATABASE_IP_URL[50 + 1];
	char szDATABASE[50 + 1];
	char szSID[50 + 1];
	char szPASSWORD[50 + 1];

	GetPrivateProfileString(DATABASE_SESSION, "DATABASE_IP_URL", DATABASE_IP_URL, szDATABASE_IP_URL, sizeof(szDATABASE_IP_URL), PARAINI);
	GetPrivateProfileString(DATABASE_SESSION, "DATABASE", DATABASE, szDATABASE, sizeof(szDATABASE), PARAINI);
	GetPrivateProfileString(DATABASE_SESSION, "SID", SID, szSID, sizeof(szSID), PARAINI);
	GetPrivateProfileString(DATABASE_SESSION, "PASSWORD", PASSWORD, szPASSWORD, sizeof(szPASSWORD), PARAINI);

	if (pScenario->m_AdoDb->DBConnect(szPASSWORD, szSID, szDATABASE, szDATABASE_IP_URL) == NULL)
	{
		pScenario->m_DBAccess = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("upOrderPaySatetProc the line service is Conneted Error", ch);
		eprintf("upOrderPaySatetProc END");
		_endthreadex((unsigned int)pScenario->m_hThread);
		return 0;
	}
	eprintf("upOrderPayState===> ORDER_NO  : %s", pScenario->m_CardResInfo.ORDER_NO);

	BOOL bRet = pScenario->m_AdoDb->upOrderPayState(pScenario->m_CardResInfo.REPLY_CODE
		                               , pScenario->m_CardResInfo.REPLY_MESSAGE
									   , pScenario->m_CardResInfo.ORDER_NO, pScenario->m_CardResInfo.TERMINAL_ID);

	if (bRet == TRUE) pScenario->m_PayResult = 1;
	else pScenario->m_PayResult = 0;

	ADO_Quithostio("upOrderPaySatetProc the line service is Success...........", ch);
	eprintf("upOrderPaySatetProc END");
	_endthreadex((unsigned int)pScenario->m_hThread);
	return 0;
}

unsigned int __stdcall setPayLogPorc(void *data)
{
	int			ch;
	int			threadID;
	char        AnsiNO_Char[257] = { 0x00, };
	LPMTP		*lineTablePtr = (LPMTP *)data;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)(lineTablePtr->pScenario);

	ch = lineTablePtr->chanID;
	threadID = pScenario->threadID;

	eprintf("setPayLogPorc START");

	CoInitialize(0);
	pScenario->m_AdoDb = new CADODB(pScenario);

	//리턴값
	//=====================
	//쿼리인자변수선언
	//=====================
	pScenario->m_PayResult = 0;
	if (threadID != pScenario->threadID) {
		pScenario->m_PayResult = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("setPayLogPorc the line service is not valid any more.", ch);
		eprintf("setPayLogPorc END");
		_endthreadex((unsigned int)pScenario->m_hThread);
		return 0;
	}
	if (pScenario->m_AdoDb == NULL){
		pScenario->m_PayResult = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("setPayLogPorc the line service Object is Null.", ch);
		eprintf("setPayLogPorc END");
		_endthreadex((unsigned int)pScenario->m_hThread);
		return 0;
	}

	char szDATABASE_IP_URL[50 + 1];
	char szDATABASE[50 + 1];
	char szSID[50 + 1];
	char szPASSWORD[50 + 1];

	GetPrivateProfileString(DATABASE_SESSION, "DATABASE_IP_URL", DATABASE_IP_URL, szDATABASE_IP_URL, sizeof(szDATABASE_IP_URL), PARAINI);
	GetPrivateProfileString(DATABASE_SESSION, "DATABASE", DATABASE, szDATABASE, sizeof(szDATABASE), PARAINI);
	GetPrivateProfileString(DATABASE_SESSION, "SID", SID, szSID, sizeof(szSID), PARAINI);
	GetPrivateProfileString(DATABASE_SESSION, "PASSWORD", PASSWORD, szPASSWORD, sizeof(szPASSWORD), PARAINI);

	if (pScenario->m_AdoDb->DBConnect(szPASSWORD, szSID, szDATABASE, szDATABASE_IP_URL) == NULL)
	{
		pScenario->m_DBAccess = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("setPayLogPorc the line service is Conneted Error", ch);
		eprintf("setPayLogPorc END");
		_endthreadex((unsigned int)pScenario->m_hThread);
		return 0;
	}

	BOOL bRet = pScenario->m_AdoDb->setPayLog(pScenario->m_CardResInfo);

	if (bRet == TRUE) pScenario->m_PayResult = 1;
	else pScenario->m_PayResult = 0;

	ADO_Quithostio("setPayLogPorc the line service is Success...........", ch);
	eprintf("setPayLogPorc END");
	_endthreadex((unsigned int)pScenario->m_hThread);
	return 0;
}

// DNIS에 맵핑 된 해당 DLL 정보 및 시나리오 유형을 획득 한다.
// 지정된 주문번호를 키로 한다.
unsigned int __stdcall sp_getKiccSMSOrderInfoByTel2(void *data)
{
	int			ch;
	int			threadID;
	char        AnsiNO_Char[257] = { 0x00, };
	LPMTP		*lineTablePtr = (LPMTP *)data;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)(lineTablePtr->pScenario);

	ch = lineTablePtr->chanID;
	threadID = pScenario->threadID;

	eprintf("sp_getKiccSMSOrderInfoByTel2 START");
	//2016.12.26
	//교환기로부터 정의 되지 않은 데이터 유입 시
	//대응을 위해 초기화 위치 변경
	memset(pScenario->m_szorder_no, 0x00, sizeof(pScenario->m_szorder_no));
	memset(pScenario->m_szterminal_nm, 0x00, sizeof(pScenario->m_szterminal_nm));
	memset(pScenario->m_szterminal_id, 0x00, sizeof(pScenario->m_szterminal_id));
	memset(pScenario->m_szterminal_pw, 0x00, sizeof(pScenario->m_szterminal_pw));
	memset(pScenario->m_szcust_nm, 0x00, sizeof(pScenario->m_szcust_nm));
	memset(pScenario->m_szgood_nm, 0x00, sizeof(pScenario->m_szgood_nm));
	memset(pScenario->m_szphone_no, 0x00, sizeof(pScenario->m_szphone_no));
	//KICC는 라이선스 키 개념이 없다.
	memset(pScenario->m_szMx_opt, 0x00, sizeof(pScenario->m_szMx_opt));
	memset(pScenario->m_szCC_email, 0X00, sizeof(pScenario->m_szCC_email));
	memset(pScenario->m_szSHOP_PW, 0X00, sizeof(pScenario->m_szSHOP_PW));
	pScenario->m_namount = 0;
	//KICC는 웹으로부터 할부개월수를 받지 않는다.
	memset(pScenario->m_szInstallment, 0x00, sizeof(pScenario->m_szInstallment));

	CoInitialize(0);
	pScenario->m_AdoDb = new CADODB(pScenario);

	//리턴값
	//=====================
	//쿼리인자변수선언
	//=====================
	pScenario->m_bDnisInfo = 0;
	if (threadID != pScenario->threadID) {
		pScenario->m_bDnisInfo = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("sp_getKiccSMSOrderInfoByTel2 the line service is not valid any more.", ch);
		eprintf("sp_getKiccSMSOrderInfoByTel2 END");
		_endthreadex((unsigned int)pScenario->m_hThread);
		return 0;
	}
	if (pScenario->m_AdoDb == NULL){
		pScenario->m_bDnisInfo = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("sp_getKiccSMSOrderInfoByTel2 the line service Object is Null.", ch);
		eprintf("sp_getKiccSMSOrderInfoByTel2 END");
		_endthreadex((unsigned int)pScenario->m_hThread);
		return 0;
	}

	char szDATABASE_IP_URL[50 + 1];
	char szDATABASE[50 + 1];
	char szSID[50 + 1];
	char szPASSWORD[50 + 1];

	GetPrivateProfileString(DATABASE_SESSION, "DATABASE_IP_URL", DATABASE_IP_URL, szDATABASE_IP_URL, sizeof(szDATABASE_IP_URL), PARAINI);
	GetPrivateProfileString(DATABASE_SESSION, "DATABASE", DATABASE, szDATABASE, sizeof(szDATABASE), PARAINI);
	GetPrivateProfileString(DATABASE_SESSION, "SID", SID, szSID, sizeof(szSID), PARAINI);
	GetPrivateProfileString(DATABASE_SESSION, "PASSWORD", PASSWORD, szPASSWORD, sizeof(szPASSWORD), PARAINI);

	if (pScenario->m_AdoDb->DBConnect(szPASSWORD, szSID, szDATABASE, szDATABASE_IP_URL) == NULL)
	{
		pScenario->m_DBAccess = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("sp_getKiccSMSOrderInfoByTel2 the line service is Conneted Error", ch);
		eprintf("sp_getKiccSMSOrderInfoByTel2 END");
		_endthreadex((unsigned int)pScenario->m_hThread);
		return 0;
	}

	if (pScenario->m_AdoDb->sp_getKiccSMSOrderInfoByTel2(pScenario->szDnis, pScenario->m_szAuth_no))
	{
		// 2015.12.10  형 변환 호환성 문제 발생
		/*_bstr_t*/ _variant_t bt = "";
		char szAmount[10 + 1];
		int iROWCOUNT;
		int iField;

		iROWCOUNT = pScenario->m_AdoDb->GetRecCount();
		if (iROWCOUNT <= 0)
		{
			pScenario->m_bDnisInfo = 0;
			(*port)[ch].ppftbl[POST_NET].postcode = HI_OK;
			ADO_Quithostio("sp_getKiccSMSOrderInfoByTel2 the line service is GetRecCount Warning", ch);
			eprintf("sp_getKiccSMSOrderInfoByTel2 END");
			_endthreadex((unsigned int)pScenario->m_hThread);
			return 0;
		}

		iField = pScenario->m_AdoDb->GetFieldCount();
		for (int iROW = 0; iROW < iROWCOUNT; iROW++)
		{//무조건 하나다
			// 필요한 변수 초기화 반드시 한다.
			//2016.12.26
			//교환기로부터 정의 되지 않은 데이터 유입 시
			//대응을 위해 초기화 위치 변경
			//memset(pScenario->m_szorder_no, 0x00, sizeof(pScenario->m_szorder_no));
			//memset(pScenario->m_szterminal_nm, 0x00, sizeof(pScenario->m_szterminal_nm));
			//memset(pScenario->m_szterminal_id, 0x00, sizeof(pScenario->m_szterminal_id));
			//memset(pScenario->m_szterminal_pw, 0x00, sizeof(pScenario->m_szterminal_pw));
			//memset(pScenario->m_szcust_nm, 0x00, sizeof(pScenario->m_szcust_nm));
			//memset(pScenario->m_szgood_nm, 0x00, sizeof(pScenario->m_szgood_nm));
			//memset(pScenario->m_szphone_no, 0x00, sizeof(pScenario->m_szphone_no));
			////KICC는 라이선스 키 개념이 없다.
			//memset(pScenario->m_szMx_opt, 0x00, sizeof(pScenario->m_szMx_opt));
			//memset(pScenario->m_szCC_email, 0X00, sizeof(pScenario->m_szCC_email));
			//memset(pScenario->m_szSHOP_PW, 0X00, sizeof(pScenario->m_szSHOP_PW));
			//pScenario->m_namount = 0;
			////KICC는 웹으로부터 할부개월수를 받지 않는다.
			//memset(pScenario->m_szInstallment, 0x00, sizeof(pScenario->m_szInstallment));

			pScenario->m_AdoDb->GetRs(_variant_t(L"order_no"), bt);
			strncpy(pScenario->m_szorder_no, (char*)(_bstr_t)bt, sizeof(pScenario->m_szorder_no) - 1);
			pScenario->m_AdoDb->GetRs(_variant_t(L"terminal_id"), bt);
			strncpy(pScenario->m_szterminal_id, (char*)(_bstr_t)bt, sizeof(pScenario->m_szterminal_id) - 1);
			pScenario->m_AdoDb->GetRs(_variant_t(L"terminal_pw"), bt);
			strncpy(pScenario->m_szterminal_pw, (char*)(_bstr_t)bt, sizeof(pScenario->m_szterminal_pw) - 1);
			pScenario->m_AdoDb->GetRs(_variant_t(L"terminal_nm"), bt);
			strncpy(pScenario->m_szterminal_nm, (char*)(_bstr_t)bt, sizeof(pScenario->m_szterminal_nm) - 1);
			pScenario->m_AdoDb->GetRs(_variant_t(L"cust_nm"), bt);
			strncpy(pScenario->m_szcust_nm, (char*)(_bstr_t)bt, sizeof(pScenario->m_szcust_nm) - 1);
			pScenario->m_AdoDb->GetRs(_variant_t(L"good_nm"), bt);
			strncpy(pScenario->m_szgood_nm, (char*)(_bstr_t)bt, sizeof(pScenario->m_szgood_nm) - 1);

			memset(szAmount, 0x00, sizeof(szAmount));
			pScenario->m_AdoDb->GetRs(_variant_t(L"amount"), bt);
			strncpy(szAmount, (char*)(_bstr_t)bt, sizeof(szAmount) - 1);
			pScenario->m_namount = atoi(szAmount);

			pScenario->m_AdoDb->GetRs(_variant_t(L"phone_no"), bt);
			strncpy(pScenario->m_szphone_no, (char*)(_bstr_t)bt, sizeof(pScenario->m_szphone_no) - 1);
			/*pScenario->m_AdoDb->GetRs(_variant_t(L"INSTALLMENT"), bt);
			strncpy(pScenario->m_szInstallment, (char*)(_bstr_t)bt, sizeof(pScenario->m_szInstallment) - 1);*/
			pScenario->m_AdoDb->GetRs(_variant_t(L"SHOP_PW"), bt);
			strncpy(pScenario->m_szSHOP_PW, (char*)(_bstr_t)bt, sizeof(pScenario->m_szSHOP_PW) - 1);



			/*pScenario->m_AdoDb->GetRs(_variant_t(L"CC_EMAIL"), bt);
			strncpy(pScenario->m_szCC_email, (char*)(_bstr_t)bt, sizeof(pScenario->m_szCC_email) - 1);

			memset(szAmount, 0x00, sizeof(szAmount));
			pScenario->m_AdoDb->GetRs(_variant_t(L"SERVICEAMT"), bt);
			strncpy(szAmount, (char*)(_bstr_t)bt, sizeof(szAmount) - 1);
			pScenario->m_nServiceAmt = atoi(szAmount);

			memset(szAmount, 0x00, sizeof(szAmount));
			pScenario->m_AdoDb->GetRs(_variant_t(L"SUPPLYAMT"), bt);
			strncpy(szAmount, (char*)(_bstr_t)bt, sizeof(szAmount) - 1);
			pScenario->m_nSupplyAmt = atoi(szAmount);

			memset(szAmount, 0x00, sizeof(szAmount));
			pScenario->m_AdoDb->GetRs(_variant_t(L"GOODSVAT"), bt);
			strncpy(szAmount, (char*)(_bstr_t)bt, sizeof(szAmount) - 1);
			pScenario->m_nGoodsVat = atoi(szAmount);

			memset(szAmount, 0x00, sizeof(szAmount));
			pScenario->m_AdoDb->GetRs(_variant_t(L"TAXFREEAMT"), bt);
			strncpy(szAmount, (char*)(_bstr_t)bt, sizeof(szAmount) - 1);
			pScenario->m_nTaxFreeAmt = atoi(szAmount);*/

			pScenario->m_AdoDb->Next();
		}
		pScenario->m_AdoDb->RSClose();
		
		pScenario->m_bDnisInfo = 1;
		ADO_Quithostio("sp_getKiccSMSOrderInfoByTel2 the line service is Success...........", ch);
		eprintf("sp_getKiccSMSOrderInfoByTel2 END");
		_endthreadex((unsigned int)pScenario->m_hThread);
		return 0;
	}

	pScenario->m_bDnisInfo = -1;
	ADO_Quithostio("sp_getKiccSMSOrderInfoByTel2 the line service is Fail...........", ch);
	eprintf("sp_getKiccSMSOrderInfoByTel2 END");
	_endthreadex((unsigned int)pScenario->m_hThread);
	return 0;
}


// DNIS에 맵핑 된 해당 DLL 정보 및 시나리오 유형을 획득 한다.
// 지정된 핸드폰 번호를 키로 한다.
unsigned int __stdcall sp_getKiccOrderInfoByTel2(void *data)
{
	int			ch;
	int			threadID;
	char        AnsiNO_Char[257] = { 0x00, };
	LPMTP		*lineTablePtr = (LPMTP *)data;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)(lineTablePtr->pScenario);

	ch = lineTablePtr->chanID;
	threadID = pScenario->threadID;

	eprintf("sp_getKiccOrderInfoByTel2 START");
	//2016.12.26
	//교환기로부터 정의 되지 않은 데이터 유입 시
	//대응을 위해 초기화 위치 변경
	memset(pScenario->m_szorder_no, 0x00, sizeof(pScenario->m_szorder_no));
	memset(pScenario->m_szterminal_nm, 0x00, sizeof(pScenario->m_szterminal_nm));
	memset(pScenario->m_szterminal_id, 0x00, sizeof(pScenario->m_szterminal_id));
	memset(pScenario->m_szterminal_pw, 0x00, sizeof(pScenario->m_szterminal_pw));
	memset(pScenario->m_szcust_nm, 0x00, sizeof(pScenario->m_szcust_nm));
	memset(pScenario->m_szgood_nm, 0x00, sizeof(pScenario->m_szgood_nm));
	memset(pScenario->m_szphone_no, 0x00, sizeof(pScenario->m_szphone_no));
	memset(pScenario->m_szADMIN_ID, 0x00, sizeof(pScenario->m_szADMIN_ID));
	//KICC는 라이선스 키 개념이 없다.
	memset(pScenario->m_szMx_opt, 0x00, sizeof(pScenario->m_szMx_opt));
	memset(pScenario->m_szCC_email, 0X00, sizeof(pScenario->m_szCC_email));
	memset(pScenario->m_szSHOP_PW, 0X00, sizeof(pScenario->m_szSHOP_PW));
	pScenario->m_namount = 0;
	//KICC는 웹으로부터 할부개월수를 받지 않는다.
	memset(pScenario->m_szInstallment, 0x00, sizeof(pScenario->m_szInstallment));

	CoInitialize(0);
	pScenario->m_AdoDb = new CADODB(pScenario);

	//리턴값
	//=====================
	//쿼리인자변수선언
	//=====================
	pScenario->m_bDnisInfo = 0;
	if (threadID != pScenario->threadID) {
		pScenario->m_bDnisInfo = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("sp_getKiccOrderInfoByTel2 the line service is not valid any more.", ch);
		eprintf("sp_getKiccOrderInfoByTel2 END");
		_endthreadex((unsigned int)pScenario->m_hThread);
		return 0;
	}
	if (pScenario->m_AdoDb == NULL){
		pScenario->m_bDnisInfo = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("sp_getKiccOrderInfoByTel2 the line service Object is Null.", ch);
		eprintf("sp_getKiccOrderInfoByTel2 END");
		_endthreadex((unsigned int)pScenario->m_hThread);
		return 0;
	}
	
	char szDATABASE_IP_URL[50 + 1];
	char szDATABASE[50 + 1];
	char szSID[50 + 1];
	char szPASSWORD[50 + 1];

	GetPrivateProfileString(DATABASE_SESSION, "DATABASE_IP_URL", DATABASE_IP_URL, szDATABASE_IP_URL, sizeof(szDATABASE_IP_URL), PARAINI);
	GetPrivateProfileString(DATABASE_SESSION, "DATABASE", DATABASE, szDATABASE, sizeof(szDATABASE), PARAINI);
	GetPrivateProfileString(DATABASE_SESSION, "SID", SID, szSID, sizeof(szSID), PARAINI);
	GetPrivateProfileString(DATABASE_SESSION, "PASSWORD", PASSWORD, szPASSWORD, sizeof(szPASSWORD), PARAINI);

	if (pScenario->m_AdoDb->DBConnect(szPASSWORD, szSID, szDATABASE, szDATABASE_IP_URL) == NULL)
	{
		pScenario->m_DBAccess = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("sp_getKiccOrderInfoByTel2 the line service is Conneted Error", ch);
		eprintf("sp_getKiccOrderInfoByTel2 END");
		_endthreadex((unsigned int)pScenario->m_hThread);
		return 0;
	}

	if (pScenario->m_AdoDb->sp_getKiccOrderInfoByTel2(pScenario->szDnis, pScenario->m_szInputTel))
	{
		// 2015.12.10  형 변환 호환성 문제 발생
		/*_bstr_t*/ _variant_t bt = "";
		char szAmount[10 + 1];
		int iROWCOUNT;
		int iField;

		iROWCOUNT = pScenario->m_AdoDb->GetRecCount();
		if (iROWCOUNT <= 0)
		{
			pScenario->m_bDnisInfo = 0;
			(*port)[ch].ppftbl[POST_NET].postcode = HI_OK;
			ADO_Quithostio("sp_getKiccOrderInfoByTel2 the line service is GetRecCount Warning", ch);
			eprintf("sp_getKiccOrderInfoByTel2 END");
			_endthreadex((unsigned int)pScenario->m_hThread);
			return 0;
		}

		iField = pScenario->m_AdoDb->GetFieldCount();
		for (int iROW = 0; iROW < iROWCOUNT; iROW++)
		{//무조건 하나다
			// 필요한 변수 초기화 반드시 한다.
			
			//2016.12.26
			//교환기로부터 정의 되지 않은 데이터 유입 시
			//대응을 위해 초기화 위치 변경
			//memset(pScenario->m_szorder_no, 0x00, sizeof(pScenario->m_szorder_no));
			//memset(pScenario->m_szterminal_nm, 0x00, sizeof(pScenario->m_szterminal_nm));
			//memset(pScenario->m_szterminal_id, 0x00, sizeof(pScenario->m_szterminal_id));
			//memset(pScenario->m_szterminal_pw, 0x00, sizeof(pScenario->m_szterminal_pw));
			//memset(pScenario->m_szcust_nm, 0x00, sizeof(pScenario->m_szcust_nm));
			//memset(pScenario->m_szgood_nm, 0x00, sizeof(pScenario->m_szgood_nm));
			//memset(pScenario->m_szphone_no, 0x00, sizeof(pScenario->m_szphone_no));
			//memset(pScenario->m_szADMIN_ID, 0x00, sizeof(pScenario->m_szADMIN_ID));
			////KICC는 라이선스 키 개념이 없다.
			//memset(pScenario->m_szMx_opt, 0x00, sizeof(pScenario->m_szMx_opt));
			//memset(pScenario->m_szCC_email, 0X00, sizeof(pScenario->m_szCC_email));
			//memset(pScenario->m_szSHOP_PW, 0X00, sizeof(pScenario->m_szSHOP_PW));
			//pScenario->m_namount = 0;
			////KICC는 웹으로부터 할부개월수를 받지 않는다.
			//memset(pScenario->m_szInstallment, 0x00, sizeof(pScenario->m_szInstallment));

			pScenario->m_AdoDb->GetRs(_variant_t(L"order_no"), bt);
			strncpy(pScenario->m_szorder_no, (char*)(_bstr_t)bt, sizeof(pScenario->m_szorder_no) - 1);
			pScenario->m_AdoDb->GetRs(_variant_t(L"terminal_id"), bt);
			strncpy(pScenario->m_szterminal_id, (char*)(_bstr_t)bt, sizeof(pScenario->m_szterminal_id) - 1);
			pScenario->m_AdoDb->GetRs(_variant_t(L"terminal_pw"), bt);
			strncpy(pScenario->m_szterminal_pw, (char*)(_bstr_t)bt, sizeof(pScenario->m_szterminal_pw) - 1);
			pScenario->m_AdoDb->GetRs(_variant_t(L"terminal_nm"), bt);
			strncpy(pScenario->m_szterminal_nm, (char*)(_bstr_t)bt, sizeof(pScenario->m_szterminal_nm) - 1);
			pScenario->m_AdoDb->GetRs(_variant_t(L"cust_nm"), bt);
			strncpy(pScenario->m_szcust_nm, (char*)(_bstr_t)bt, sizeof(pScenario->m_szcust_nm) - 1); 
			pScenario->m_AdoDb->GetRs(_variant_t(L"good_nm"), bt);
			strncpy(pScenario->m_szgood_nm, (char*)(_bstr_t)bt, sizeof(pScenario->m_szgood_nm) - 1);

			memset(szAmount, 0x00, sizeof(szAmount));
			pScenario->m_AdoDb->GetRs(_variant_t(L"amount"), bt);
			strncpy(szAmount, (char*)(_bstr_t)bt, sizeof(szAmount) - 1);
			pScenario->m_namount = atoi(szAmount);
						
			pScenario->m_AdoDb->GetRs(_variant_t(L"phone_no"), bt);
			strncpy(pScenario->m_szphone_no, (char*)(_bstr_t)bt, sizeof(pScenario->m_szphone_no) - 1);
			/*pScenario->m_AdoDb->GetRs(_variant_t(L"INSTALLMENT"), bt);
			strncpy(pScenario->m_szInstallment, (char*)(_bstr_t)bt, sizeof(pScenario->m_szInstallment) - 1);*/
			pScenario->m_AdoDb->GetRs(_variant_t(L"SHOP_PW"), bt);
			strncpy(pScenario->m_szSHOP_PW, (char*)(_bstr_t)bt, sizeof(pScenario->m_szSHOP_PW) - 1);

			pScenario->m_AdoDb->GetRs(_variant_t(L"ADMIN_ID"), bt);
			strncpy(pScenario->m_szADMIN_ID, (char*)(_bstr_t)bt, sizeof(pScenario->m_szADMIN_ID) - 1);

			// ========================================
			// [2025-11-21 NEW] RESERVED_3/4/5 필드 읽기
			// ========================================
			_variant_t btReserved;
			char szReservedValue[64 + 1] = { 0x00, };

			// RESERVED_3: 유효기간 (YYMM, 4자리)
			btReserved = "";
			pScenario->m_AdoDb->GetRs(_variant_t(L"RESERVED_3"), btReserved);
			strncpy(szReservedValue, (char*)(_bstr_t)btReserved, sizeof(szReservedValue) - 1);
			strncpy(pScenario->m_szDB_ExpireDate, szReservedValue, sizeof(pScenario->m_szDB_ExpireDate) - 1);
			eprintf("[KICC] DB RESERVED_3 (유효기간) 로드: %s", pScenario->m_szDB_ExpireDate);

			// RESERVED_4: 카드번호 앞자리 (12자리)
			btReserved = "";
			memset(szReservedValue, 0x00, sizeof(szReservedValue));
			pScenario->m_AdoDb->GetRs(_variant_t(L"RESERVED_4"), btReserved);
			strncpy(szReservedValue, (char*)(_bstr_t)btReserved, sizeof(szReservedValue) - 1);
			strncpy(pScenario->m_szDB_CardPrefix, szReservedValue, sizeof(pScenario->m_szDB_CardPrefix) - 1);
			eprintf("[KICC] DB RESERVED_4 (카드앞자리) 로드: %s", pScenario->m_szDB_CardPrefix);

			// RESERVED_5: 할부개월 (2자리)
			btReserved = "";
			memset(szReservedValue, 0x00, sizeof(szReservedValue));
			pScenario->m_AdoDb->GetRs(_variant_t(L"RESERVED_5"), btReserved);
			strncpy(szReservedValue, (char*)(_bstr_t)btReserved, sizeof(szReservedValue) - 1);
			strncpy(pScenario->m_szDB_InstallPeriod, szReservedValue, sizeof(pScenario->m_szDB_InstallPeriod) - 1);
			eprintf("[KICC] DB RESERVED_5 (할부개월) 로드: %s", pScenario->m_szDB_InstallPeriod);

			// ========================================
			// [2025-11-21 NEW] DB 필드 유효성 검증
			// ========================================
			BOOL bDbFieldsValid = TRUE;

			// RESERVED_4 검증: 카드번호 앞자리는 정확히 12자리여야 함
			if (strlen(pScenario->m_szDB_CardPrefix) != 12) {
				eprintf("[KICC] RESERVED_4 카드번호 앞자리 길이 오류: %d자리 (기대: 12자리)", strlen(pScenario->m_szDB_CardPrefix));
				bDbFieldsValid = FALSE;
			}

			// RESERVED_3 검증: 유효기간은 정확히 4자리(YYMM)여야 함
			if (strlen(pScenario->m_szDB_ExpireDate) != 4) {
				eprintf("[KICC] RESERVED_3 유효기간 길이 오류: %d자리 (기대: 4자리)", strlen(pScenario->m_szDB_ExpireDate));
				bDbFieldsValid = FALSE;
			}

			// RESERVED_5 검증: 할부개월은 0~12 범위여야 함
			int nInstallPeriod = atoi(pScenario->m_szDB_InstallPeriod);
			if (strlen(pScenario->m_szDB_InstallPeriod) == 0 || nInstallPeriod < 0 || nInstallPeriod > 12) {
				eprintf("[KICC] RESERVED_5 할부개월 범위 오류: %s (기대: 00~12)", pScenario->m_szDB_InstallPeriod);
				bDbFieldsValid = FALSE;
			}

			// 검증 결과에 따라 DB 사용 여부 설정
			if (bDbFieldsValid) {
				pScenario->m_bUseDbCardInfo = 1;  // DB 사용 가능
				info_printf(ch, "[KICC] DB 카드정보 로드 완료");
				eprintf("[KICC] DB 필드 검증 성공 - DB 사용 모드 활성화");
				eprintf("[KICC] DB 필드 로드 완료: 유효기간=%s, 카드앞자리=%s, 할부개월=%s",
					pScenario->m_szDB_ExpireDate,
					pScenario->m_szDB_CardPrefix,
					pScenario->m_szDB_InstallPeriod);
			}
			else {
				pScenario->m_bUseDbCardInfo = 0;  // 기존 입력 방식으로 폴백
				info_printf(ch, "[KICC] DB 카드정보 오류 - 수동입력 모드");
				eprintf("[KICC] DB 필드 검증 실패 - 기존 입력 방식으로 폴백");
			}

			/*pScenario->m_AdoDb->GetRs(_variant_t(L"CC_EMAIL"), bt);
			strncpy(pScenario->m_szCC_email, (char*)(_bstr_t)bt, sizeof(pScenario->m_szCC_email) - 1);

			memset(szAmount, 0x00, sizeof(szAmount));
			pScenario->m_AdoDb->GetRs(_variant_t(L"SERVICEAMT"), bt);
			strncpy(szAmount, (char*)(_bstr_t)bt, sizeof(szAmount) - 1);
			pScenario->m_nServiceAmt = atoi(szAmount);

			memset(szAmount, 0x00, sizeof(szAmount));
			pScenario->m_AdoDb->GetRs(_variant_t(L"SUPPLYAMT"), bt);
			strncpy(szAmount, (char*)(_bstr_t)bt, sizeof(szAmount) - 1);
			pScenario->m_nSupplyAmt = atoi(szAmount);

			memset(szAmount, 0x00, sizeof(szAmount));
			pScenario->m_AdoDb->GetRs(_variant_t(L"GOODSVAT"), bt);
			strncpy(szAmount, (char*)(_bstr_t)bt, sizeof(szAmount) - 1);
			pScenario->m_nGoodsVat = atoi(szAmount);

			memset(szAmount, 0x00, sizeof(szAmount));
			pScenario->m_AdoDb->GetRs(_variant_t(L"TAXFREEAMT"), bt);
			strncpy(szAmount, (char*)(_bstr_t)bt, sizeof(szAmount) - 1);
			pScenario->m_nTaxFreeAmt = atoi(szAmount);*/

			pScenario->m_AdoDb->Next();
		}
		pScenario->m_AdoDb->RSClose();

		pScenario->m_bDnisInfo = 1;
		ADO_Quithostio("sp_getKiccOrderInfoByTel2 the line service is Success...........", ch);
		eprintf("sp_getKiccOrderInfoByTel2 END");
		_endthreadex((unsigned int)pScenario->m_hThread);

		return 0;
	}

	pScenario->m_bDnisInfo = -1;
	ADO_Quithostio("sp_getKiccOrderInfoByTel2 the line service is Fail...........", ch);
	eprintf("sp_getKiccOrderInfoByTel2 END");
	_endthreadex((unsigned int)pScenario->m_hThread);

	return 0;
}

// SMS 발송
unsigned int __stdcall ADO_SMS_Send(void *data)
{
	int			ch;
	int			threadID;
	char        AnsiNO_Char[257] = { 0x00, };
	LPMTP		*lineTablePtr = (LPMTP *)data;
	CKICC_Scenario *pScenario = (CKICC_Scenario *)lineTablePtr->pScenario;

	ch = lineTablePtr->chanID;
	threadID = pScenario->threadID;

	CoInitialize(0);
	pScenario->m_AdoDb = new CADODB(pScenario);

	eprintf("ADO_SMS_Send START");
	//리턴값
	//=====================
	//쿼리인자변수선언
	//=====================
	if (threadID != pScenario->threadID) {
		pScenario->m_PayResult = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("ADO_SMS_Send the line service is not valid any more.", ch);
		eprintf("ADO_SMS_Send END");
		_endthreadex((unsigned int)pScenario->m_hThread);
		return 0;
	}


	if (pScenario->m_AdoDb == NULL){
		pScenario->m_PayResult = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("ADO_SMS_Send the line service Object is Null.", ch);
		eprintf("ADO_SMS_Send END");
		_endthreadex((unsigned int)pScenario->m_hThread);
		return 0;
	}

	char szDATABASE_IP_URL[50 + 1];
	char szDATABASE[50 + 1];
	char szSID[50 + 1];
	char szPASSWORD[50 + 1];

	GetPrivateProfileString("SMS_SEND", "DATABASE_IP_URL", DATABASE_IP_URL, szDATABASE_IP_URL, sizeof(szDATABASE_IP_URL), PARAINI);
	GetPrivateProfileString("SMS_SEND", "DATABASE", DATABASE, szDATABASE, sizeof(szDATABASE), PARAINI);
	GetPrivateProfileString("SMS_SEND", "SID", SID, szSID, sizeof(szSID), PARAINI);
	GetPrivateProfileString("SMS_SEND", "PASSWORD", PASSWORD, szPASSWORD, sizeof(szPASSWORD), PARAINI);

	if (pScenario->m_AdoDb->DBConnect(szPASSWORD, szSID, szDATABASE, szDATABASE_IP_URL) == NULL)
	{
		pScenario->m_DBAccess = -1;
		(*lpmt)->ppftbl[POST_NET].postcode = HI_COMM;
		eprintf("ADO_SMS_Send Nice Payment Load Failed ...");
		ADO_Quithostio("ADO_SMS_Send the line service is Load Error", ch);
		_endthreadex((unsigned int)pScenario->m_hThread);
		return -1;
	}
	// 홈플러스이면 관리자 번호--> 임시적으로 안현 이사 핸드폰 번호로
	BOOL bRet = pScenario->m_AdoDb->SMS_Send(pScenario->m_szorder_no, "01026420684"/*pScenario->m_szInputTel*/,
		pScenario->m_szcust_nm, pScenario->m_szgood_nm, pScenario->m_namount, pScenario->m_szterminal_nm);

	if (bRet == TRUE) pScenario->m_PayResult = 1;
	else pScenario->m_PayResult = 0;

	ADO_Quithostio("ADO_SMS_Send the line service is 성공.", ch);
	eprintf("ADO_SMS_Send END");
	_endthreadex((unsigned int)pScenario->m_hThread);

	return 0;
}


int  SMS_host(int holdm)
{
	CKICC_Scenario *pScenario = (CKICC_Scenario *)(*lpmt)->pScenario;
	//초기화	
	pScenario->m_DBAccess = 0;

	if (holdm != 0) {
		new_guide();
		(*lpmt)->trials = 0;
		(*lpmt)->Hmusic = HM_LOOP;
		set_guide(holdm);
		send_guide(NODTMF);
	}


	(*lpmt)->ppftbl[POST_NET].postcode = HI_NCMPLT;// 종료하지 않았다.

	pScenario->m_hThread = (HANDLE)_beginthreadex(NULL, 0, ADO_SMS_Send, (LPVOID)(*lpmt), 0, &(pScenario->threadID));

	return(0);
}

int getOrderInfo_host(int holdm)
{
	//초기화	
	((CKICC_Scenario *)((*lpmt)->pScenario))->m_DBAccess = 0;
	if (holdm != 0) {
		if (new_guide) new_guide();
		(*lpmt)->trials = 0;
		(*lpmt)->Hmusic = HM_LOOP;
		if (set_guide) set_guide(holdm);
		if (send_guide)  send_guide(NODTMF);
	}
	
	if (((CKICC_Scenario *)((*lpmt)->pScenario))->m_AdoDb != NULL)
	{
		((CKICC_Scenario *)((*lpmt)->pScenario))->m_DBAccess = -1;
		(*lpmt)->ppftbl[POST_NET].postcode = HI_OK;
		ADO_Quithostio("DnisVsDll_Host the line service is Load Error", (*lpmt)->chanID);
		return 0;
	}

	(*lpmt)->ppftbl[POST_NET].postcode = HI_NCMPLT;// 종료하지 않았다.
	((CKICC_Scenario *)((*lpmt)->pScenario))->m_hThread = (HANDLE)_beginthreadex(NULL, 0, sp_getKiccOrderInfoByTel2, (LPVOID)(*lpmt), 0, &(((CKICC_Scenario *)((*lpmt)->pScenario))->threadID));

	return(0);
}

// [다중 주문 지원 - 신규] 다중 주문 조회 스레드
unsigned int __stdcall getMultiOrderInfoProc(void* arg)
{
	int			ch;
	int			threadID;
	LPMTP		*lineTablePtr = (LPMTP *)arg;
	CKICC_Scenario* pScenario = (CKICC_Scenario*)(lineTablePtr->pScenario);

	ch = lineTablePtr->chanID;
	threadID = pScenario->threadID;

	eprintf("getMultiOrderInfoProc 시작");

	CoInitialize(0);
	pScenario->m_DBAccess = 0;

	if (threadID != pScenario->threadID) {
		pScenario->m_DBAccess = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("getMultiOrderInfoProc the line service is not valid any more.", ch);
		eprintf("getMultiOrderInfoProc END");
		_endthreadex(0);
		return 0;
	}

	if (pScenario->m_AdoDb == NULL) {
		pScenario->m_AdoDb = new CADODB(pScenario);
	}

	if (pScenario->m_AdoDb == NULL) {
		eprintf("CADODB 인스턴스 생성 실패");
		pScenario->m_DBAccess = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("getMultiOrderInfoProc the line service Object is Null.", ch);
		eprintf("getMultiOrderInfoProc END");
		_endthreadex(0);
		return 0;
	}

	char szDATABASE_IP_URL[50 + 1];
	char szDATABASE[50 + 1];
	char szSID[50 + 1];
	char szPASSWORD[50 + 1];

	GetPrivateProfileString(DATABASE_SESSION, "DATABASE_IP_URL", DATABASE_IP_URL, szDATABASE_IP_URL, sizeof(szDATABASE_IP_URL), PARAINI);
	GetPrivateProfileString(DATABASE_SESSION, "DATABASE", DATABASE, szDATABASE, sizeof(szDATABASE), PARAINI);
	GetPrivateProfileString(DATABASE_SESSION, "SID", SID, szSID, sizeof(szSID), PARAINI);
	GetPrivateProfileString(DATABASE_SESSION, "PASSWORD", PASSWORD, szPASSWORD, sizeof(szPASSWORD), PARAINI);

	// 데이터베이스 연결
	if (!pScenario->m_AdoDb->DBConnect(szPASSWORD, szSID, szDATABASE, szDATABASE_IP_URL)) {
		eprintf("데이터베이스 연결 실패");
		pScenario->m_DBAccess = -1;
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("getMultiOrderInfoProc the line service is Conneted Error", ch);
		eprintf("getMultiOrderInfoProc END");
		_endthreadex(0);
		return 0;
	}

	// 다중 주문 조회 실행
	// AUTH_NO는 SP 내부에서 자동으로 추출됨
	CString szDnis(pScenario->szDnis);
	CString szPhoneNo(pScenario->m_szInputTel);

	if (!pScenario->m_AdoDb->sp_getKiccMultiOrderInfo(szDnis, szPhoneNo)) {
		eprintf("다중 주문 조회 실패");
		pScenario->m_DBAccess = -1;
		pScenario->m_AdoDb->ConClose();
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("getMultiOrderInfoProc the line service is Fail", ch);
		eprintf("getMultiOrderInfoProc END");
		_endthreadex(0);
		return 0;
	}

	// 일치하는 모든 주문 가져오기
	if (!pScenario->m_AdoDb->FetchMultiOrderResults(&pScenario->m_MultiOrders)) {
		eprintf("다중 주문 결과 가져오기 실패");
		pScenario->m_DBAccess = -1;
		pScenario->m_AdoDb->ConClose();
		(*port)[ch].ppftbl[POST_NET].postcode = HI_COMM;
		ADO_Quithostio("getMultiOrderInfoProc the line service is Fail", ch);
		eprintf("getMultiOrderInfoProc END");
		_endthreadex(0);
		return 0;
	}

	eprintf("%d건 주문 조회됨, 총액: %d",
		pScenario->m_MultiOrders.nOrderCount,
		pScenario->m_MultiOrders.nTotalAmount);

	pScenario->m_DBAccess = 1;  // 성공
	pScenario->m_AdoDb->ConClose();
	(*port)[ch].ppftbl[POST_NET].postcode = HI_OK;
	ADO_Quithostio("getMultiOrderInfoProc the line service is Success", ch);

	eprintf("getMultiOrderInfoProc 종료");
	_endthreadex(0);
	return 0;
}

int getMultiOrderInfo_host(int holdm)
{
	CKICC_Scenario* pScenario = (CKICC_Scenario*)((*lpmt)->pScenario);

	pScenario->m_DBAccess = 0;

	if (holdm != 0) {
		if (new_guide) new_guide();
		(*lpmt)->trials = 0;
		(*lpmt)->Hmusic = HM_LOOP;
		if (set_guide) set_guide(holdm);
		if (send_guide) send_guide(NODTMF);
	}

	if (pScenario->m_AdoDb != NULL) {
		pScenario->m_DBAccess = -1;
		(*lpmt)->ppftbl[POST_NET].postcode = HI_OK;
		ADO_Quithostio("다중 주문 조회 DB가 이미 열려있음", (*lpmt)->chanID);
		return 0;
	}

	(*lpmt)->ppftbl[POST_NET].postcode = HI_NCMPLT;
	pScenario->m_hThread = (HANDLE)_beginthreadex(
		NULL, 0,
		getMultiOrderInfoProc,
		(LPVOID)(*lpmt),
		0,
		&(pScenario->threadID)
	);

	return 0;
}

int setPayLog_host(int holdm)
{
	//초기화	
	((CKICC_Scenario *)((*lpmt)->pScenario))->m_DBAccess = 0;
	if (holdm != 0) {
		if (new_guide) new_guide();
		(*lpmt)->trials = 0;
		(*lpmt)->Hmusic = HM_LOOP;
		if (set_guide) set_guide(holdm);
		if (send_guide)  send_guide(NODTMF);
	}
	((CKICC_Scenario *)((*lpmt)->pScenario))->m_PayResult = 0;
	if (((CKICC_Scenario *)((*lpmt)->pScenario))->m_AdoDb != NULL)
	{
		((CKICC_Scenario *)((*lpmt)->pScenario))->m_DBAccess = -1;
		(*lpmt)->ppftbl[POST_NET].postcode = HI_OK;
		ADO_Quithostio("DnisVsDll_Host the line service is Load Error", (*lpmt)->chanID);
		return 0;
	}

	(*lpmt)->ppftbl[POST_NET].postcode = HI_NCMPLT;// 종료하지 않았다.
	((CKICC_Scenario *)((*lpmt)->pScenario))->m_hThread = (HANDLE)_beginthreadex(NULL, 0, setPayLogPorc, (LPVOID)(*lpmt), 0, &(((CKICC_Scenario *)((*lpmt)->pScenario))->threadID));

	return(0);
}

int upOrderPayState_host(int holdm)
{
	//초기화	
	((CKICC_Scenario *)((*lpmt)->pScenario))->m_DBAccess = 0;
	if (holdm != 0) {
		if (new_guide) new_guide();
		(*lpmt)->trials = 0;
		(*lpmt)->Hmusic = HM_LOOP;
		if (set_guide) set_guide(holdm);
		if (send_guide)  send_guide(NODTMF);
	}
	((CKICC_Scenario *)((*lpmt)->pScenario))->m_PayResult = 0;
	if (((CKICC_Scenario *)((*lpmt)->pScenario))->m_AdoDb != NULL)
	{
		((CKICC_Scenario *)((*lpmt)->pScenario))->m_DBAccess = -1;
		(*lpmt)->ppftbl[POST_NET].postcode = HI_OK;
		ADO_Quithostio("DnisVsDll_Host the line service is Load Error", (*lpmt)->chanID);
		return 0;
	}

	(*lpmt)->ppftbl[POST_NET].postcode = HI_NCMPLT;// 종료하지 않았다.
	((CKICC_Scenario *)((*lpmt)->pScenario))->m_hThread = (HANDLE)_beginthreadex(NULL, 0, upOrderPayStateProc, (LPVOID)(*lpmt), 0, &(((CKICC_Scenario *)((*lpmt)->pScenario))->threadID));

	return(0);
}



int getSMSOrderInfo_host(int holdm)
{
	//초기화	
	((CKICC_Scenario *)((*lpmt)->pScenario))->m_DBAccess = 0;
	if (holdm != 0) {
		if (new_guide) new_guide();
		(*lpmt)->trials = 0;
		(*lpmt)->Hmusic = HM_LOOP;
		if (set_guide) set_guide(holdm);
		if (send_guide)  send_guide(NODTMF);
	}

	if (((CKICC_Scenario *)((*lpmt)->pScenario))->m_AdoDb != NULL)
	{
		((CKICC_Scenario *)((*lpmt)->pScenario))->m_DBAccess = -1;
		(*lpmt)->ppftbl[POST_NET].postcode = HI_OK;
		ADO_Quithostio("DnisVsDll_Host the line service is Load Error", (*lpmt)->chanID);
		return 0;
	}

	(*lpmt)->ppftbl[POST_NET].postcode = HI_NCMPLT;// 종료하지 않았다.
	((CKICC_Scenario *)((*lpmt)->pScenario))->m_hThread = (HANDLE)_beginthreadex(NULL, 0, sp_getKiccSMSOrderInfoByTel2, (LPVOID)(*lpmt), 0, &(((CKICC_Scenario *)((*lpmt)->pScenario))->threadID));

	return(0);
}