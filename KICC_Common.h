

// 카드 번호
typedef struct CarsInfo
{
	char Card_Num[16 + 1]; // 카드 번호
	char ExpireDt[4 + 1];  // 유효 기간 (년월)
	char SecretNo[10 + 1]; // 주민번호,법인번호
	char Password[2 + 1];  // 비밀 번호
	char InstPeriod[2 + 1];// 할부 개월수

} CARDINFO;

// 카드 승인 정보 및 취소 승인 정보
typedef struct Card_ResInfo
{
	char ORDER_NO[32 + 1];
	char TERMINAL_NM[50 + 1];    // 고정: ""
	char TERMINAL_ID[32 + 1];    // 고정: ""
	char TERMINAL_PW[32 + 1];    // 고정: ""
	char ADMIN_ID[20 + 1];	     // 고정: ""
	char ADMIN_NAME[20 + 1];     // 고정: ""
	char CUST_NM[64 + 1];	     // 고정: "홍길동"(회원제가 아니므로 임의의 값)
	char GOOD_NM[255 + 1];	     // 상품이름: 2인 입장권 패키지, 3인 패키지
	int  NUMBER;                 // 상품갯수: 1~
	int  TOTAMOUNT;		         // 총금액
	char PHONE_NO[32 + 1];       // 고객 휴대폰 번호(SMS수신을 위함)
	char DNIS[4 + 1];            // 수신번호
	char PAYMENT_CODE;           // 1 : 결제완료, 2 : 결제취소
	char REPLY_CODE[20 + 1];     // 승인결과 코드
	char REPLY_MESSAGE[255 + 1]; // 승인결과 메시지
	char CONTROL_NO[32 + 1];     // PG거래번호
	char APPROVAL_NUM[15 + 1];   // 승인번호
	char APPROVAL_DATE[20 + 1];  // 승인일시
	char REPLY_DATE[32 + 1];     // now()
	char RESERVED_2[64 + 1];
	char RESERVED_3[64 + 1];

	char InstPeriod[2 + 1];      // 할부 개월수

	int  AMOUNT;		         // 단가 : DB에는 기록 하지 않는다.
	char GOOD_CD[20 + 1];


	// 결제 챨?정보
	char issuer_cd[3 + 1]; //발급사코드
	char issuer_nm[20 + 1]; //발급사명
	char uirer_cd[3 + 1]; // 매입사코드
	char acquirer_nm[20 + 1];//매입사명
}Card_ResInfo;


//  결제 취소 요청 정보
typedef struct Card_CancleInfo
{
	char ORDER_NO[32 + 1];
	char TERMINAL_NM[50 + 1];    // 고정: ""
	char TERMINAL_ID[32 + 1];    // 고정: ""
	char TERMINAL_PW[32 + 1];    // 고정: ""
	char ADMIN_ID[20 + 1];	     // 고정: ""
	char ADMIN_NAME[20 + 1];     // 고정: ""
	char CUST_NM[64 + 1];	     // 고정: ""(회원제가 아니므로 임의의 값)
	char GOOD_NM[255 + 1];	     // 상품이름: 2인 입장권 패키지, 3인 패키지
	int  NUMBER;                 // 상품갯수: 1~
	int  TOTAMOUNT;		         // 총금액
	char PHONE_NO[32 + 1];       // 고객 휴대폰 번호(SMS수신을 위함)
	char DNIS[4 + 1];            // 수신번호
	char PAYMENT_CODE;           // 1 : 결제완료, 2 : 결제취소
	char REPLY_CODE[20 + 1];     // 승인결과 코드
	char REPLY_MESSAGE[255 + 1]; // 승인결과 메시지
	char CONTROL_NO[32 + 1];     // PG거래번호
	char APPROVAL_NUM[15 + 1];   // 승인번호
	char APPROVAL_DATE[20 + 1];  // 승인일시
	char REPLY_DATE[32 + 1];     // now()
	char RESERVED_2[64 + 1];
	char RESERVED_3[64 + 1];

	int  AMOUNT;		         // 단가 : DB에는 기록 하지 않는다.
	char GOOD_CD[20 + 1];


	// 결제 챨?정보
	char issuer_cd[3 + 1]; //발급사코드
	char issuer_nm[20 + 1]; //발급사명
	char uirer_cd[3 + 1]; // 매입사코드
	char acquirer_nm[20 + 1];//매입사명
}Card_CancleInfo;

// ?? ?? ??: ?? ?? ??? ?? ?? ??
#define MAX_ORDERS 10  // PHONE_NO + AUTH_NO ??? ?? ?? ??

typedef struct MultiOrderInfo
{
    int nOrderCount;                    // ??? ?? ??
    Card_ResInfo orders[MAX_ORDERS];    // ?? ?? ??
    int nTotalAmount;                   // ?? ??? ?? ??
    int nProcessedCount;                // ????? ??? ?? ??
    int nFailedCount;                   // ??? ?? ??
    char szFailedOrders[512];           // ??? ???? ?? (?? ??)
} MultiOrderInfo;