// ADODB.h: interface for the CADODB class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ADODB_H__2A5DE27D_5F14_4BE9_B330_5D9978E1FECB__INCLUDED_)
#define AFX_ADODB_H__2A5DE27D_5F14_4BE9_B330_5D9978E1FECB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

///////////////////////////////////////////////////////////////////////
//User Coding - DB DLL Append
#pragma warning(push)
#pragma warning(disable:4146)
#import "C:\Program Files\Common Files\System\ADO\msado15.dll" no_namespace rename("EOF", "adoEOF")
#pragma warning(pop)
///////////////////////////////////////////////////////////////////////
class CKICC_Scenario;

class CADODB
{
private:
    _RecordsetPtr  m_RS;
    _CommandPtr    m_CMD;
    _ConnectionPtr m_CONN;

    BOOL     ISRSCon();
    BOOL     ISOpen();
public:
             CADODB();
			 CADODB(CKICC_Scenario  *pScenario);
    virtual ~CADODB();

	void     SetScenarion(CKICC_Scenario  *pScenario);
	void     PrintProviderError();
	void     PrintComError(_com_error &e);
	void     PrintComWARNING(_com_error &e);

    BOOL     DBConnect(char* pWD, char* pID, char* pDataBase, char* pConnectIP);
    void     ConCancel();
    void     ConRollbackTrans();
    void     ConCommitTrans();
    long     ConBeginTrans();
    void     ConClose();
    BOOL     GetDBCon();

    BOOL     Open(char* pSorceBuf, long option = -1);
    BOOL     Excute(char* pSorceBuf, long option = -1);
    void     RSClose();
    BOOL     IsEOF();


    BOOL     Next();
    BOOL     Prev();
    BOOL     First();
    BOOL     Last();
    void     GetRs(_variant_t x, _bstr_t& ret);
    void     GetRs(_variant_t x, _variant_t& ret);
    void     GetRs(_variant_t x, float& ret);
    void     GetRs(_variant_t x, long& ret);
    void     GetRs(_variant_t x, double& ret);

    int      GetRecCount();
    int      GetFieldCount();
	//2015.07.16
	int      sp_getKiccOrderInfoByTel2(CString szDnis, CString szInputTelNum);
	int      sp_getKiccSMSOrderInfoByTel2(CString szDnis, CString szAuthNo);

	// [다중 주문 지원 - 신규]
	int      sp_getKiccMultiOrderInfo(CString szDnis, CString szPhoneNo);      // 유효기간: 7일
	int      sp_getKiccMultiOrderInfoHour(CString szDnis, CString szPhoneNo);  // 유효기간: 1시간
	BOOL     FetchMultiOrderResults(MultiOrderInfo* pMultiOrders);

	BOOL     setPayLog(Card_ResInfo ag_Card_ResInfo);
	BOOL     upOrderPayState(char *sxResultCode, char *szResultMsg, char *szMoid, char *szMid);

	BOOL     SMS_Send(CString strOrder_no, CString strPhone_no, CString szCunt_nm, CString szProdo_nm, int nAmount , CString Rs_Id);

	CKICC_Scenario  *m_pScenario;
};

#endif // !defined(AFX_ADODB_H__2A5DE27D_5F14_4BE9_B330_5D9978E1FECB__INCLUDED_)
