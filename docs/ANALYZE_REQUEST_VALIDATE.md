# 주문 조회 시 유효기간 검증 분석

## 1. 개요

본 문서는 KICC Travelport IVR 시스템에서 주문 조회 시 적용되는 유효기간(만료일) 검증 메커니즘을 분석합니다.

## 2. 현재 상태 요약

| 구분 | 내용 |
|------|------|
| **유효기간 존재 여부** | O (존재함) |
| **검증 위치** | 데이터베이스 저장프로시저 (SP) |
| **검증 기준** | 주문 등록일(`reg_date`) 기준 7일 이내 |
| **검증 필드** | `KICC_SHOP_ORDER.reg_date` |

## 3. 상세 분석

### 3.1 저장프로시저 레벨 검증

#### sp_getKiccOrderInfoByTel2 (단일 주문 조회)

**위치**: `docs/sp_getKiccOrderInfoByTel2.txt`

```sql
SELECT TOP 1
    A.order_no, A.terminal_id, A.terminal_pw, A.terminal_nm,
    A.cust_nm, A.good_nm, A.amount, A.phone_no,
    B.SHOP_PW, A.ADMIN_ID,
    A.RESERVED_3, A.RESERVED_4, A.RESERVED_5
FROM KICC_SHOP_ORDER A
INNER JOIN dbo.COMMON_DNIS_MID B ON A.terminal_id = B.SHOP_ID
WHERE
    DATEDIFF(dd, A.reg_date, GETDATE()) <= 7 AND  -- [유효기간 검증]
    B.ARS_DNIS = @DNIS AND
    A.payment_code = '0' AND
    A.phone_no = @PHONE_NO
ORDER BY reg_date DESC
```

#### sp_getKiccMultiOrderInfo (다중 주문 조회)

**위치**: `docs/PLAN_MULTI_KR.md` (설계 문서)

```sql
-- [1단계] 최근 주문 조회
SELECT TOP 1 @AUTH_NO = A.AUTH_NO
FROM KICC_SHOP_ORDER A
INNER JOIN dbo.COMMON_DNIS_MID B ON A.terminal_id = B.SHOP_ID
WHERE
    DATEDIFF(dd, A.reg_date, GETDATE()) <= 7 AND  -- [유효기간 검증]
    B.ARS_DNIS = @DNIS AND
    A.payment_code = '0' AND
    A.phone_no = @PHONE_NO
ORDER BY A.reg_date DESC;

-- [2단계] 동일 AUTH_NO 주문 전체 조회
SELECT A.order_no, ...
FROM KICC_SHOP_ORDER A
INNER JOIN dbo.COMMON_DNIS_MID B ON A.terminal_id = B.SHOP_ID
WHERE
    DATEDIFF(dd, A.reg_date, GETDATE()) <= 7 AND  -- [유효기간 검증]
    B.ARS_DNIS = @DNIS AND
    A.payment_code = '0' AND
    A.phone_no = @PHONE_NO AND
    A.AUTH_NO = @AUTH_NO
ORDER BY A.reg_date DESC;
```

### 3.2 유효기간 검증 조건

```sql
DATEDIFF(dd, A.reg_date, GETDATE()) <= 7
```

| 요소 | 설명 |
|------|------|
| `DATEDIFF(dd, ...)` | 일(day) 단위 날짜 차이 계산 |
| `A.reg_date` | 주문 등록일시 |
| `GETDATE()` | 현재 서버 시간 |
| `<= 7` | 7일 이내 |

**의미**: 주문 등록일로부터 7일이 경과하지 않은 주문만 조회 대상

### 3.3 조회 조건 전체 구조

```
┌─────────────────────────────────────────────────────────────┐
│                    주문 조회 필터 조건                       │
├─────────────────────────────────────────────────────────────┤
│  1. 유효기간: reg_date로부터 7일 이내                        │
│  2. 결제상태: payment_code = '0' (미결제)                    │
│  3. 전화번호: phone_no = @PHONE_NO (입력값 일치)             │
│  4. DNIS 매칭: ARS_DNIS = @DNIS (착신번호 일치)             │
│  5. 정렬: reg_date DESC (최신순)                            │
└─────────────────────────────────────────────────────────────┘
```

## 4. 애플리케이션 레벨 처리

### 4.1 조회 결과 없음 처리

**위치**: `KICC_Travelport_Scenario.cpp:2327-2334`

```cpp
if (pScenario->m_bDnisInfo == 0)  // 조회 결과 없음
{
    info_printf(localCh, "KICC_getOrderInfo [%d] 고객 주문 정보 안내 부> 존재하지 않은 주문 정보", state);
    eprintf("KICC_getOrderInfo [%d] 고객 주문 정보 안내 부> 존재하지 않은 주문 정보", state);
    new_guide();
    set_guide(VOC_WAVE_ID, "ment/Travelport/no_order_msg");  // "주문이 접수되지 않았습니다..."
    setPostfunc(POST_PLAY, KICC_ExitSvc, 0, 0);
    return send_guide(NODTMF);
}
```

### 4.2 다중 주문 조회 실패 처리

**위치**: `KICC_Travelport_Scenario.cpp:367-390`

조회 실패 시 최대 3회까지 전화번호 재입력을 허용합니다.

## 5. 유효기간 관련 필드 구분

시스템에는 두 가지 "유효기간" 개념이 존재합니다:

| 구분 | 필드 | 용도 | 검증 위치 |
|------|------|------|-----------|
| **주문 유효기간** | `reg_date` | 주문 조회 가능 기간 (7일) | 저장프로시저 |
| **카드 유효기간** | `RESERVED_3` / `ExpireDt` | 신용카드 만료일 (YYMM) | 애플리케이션 |

### 5.1 카드 유효기간 검증 (참고)

**위치**: `KICC_Travelport_Scenario.cpp:1939-1950`

```cpp
// 입력된 카드 유효기간 검증
if ((tmpMonth > 12 || tmpMonth < 1) ||      // 월 범위 체크
    (tmpYear < curTime.GetYear()) ||         // 이미 지난 연도
    (tmpYear > 2099))                        // 미래 연도 제한
{
    // 오류 처리
}

// 현재 년월보다 이전인 경우 추가 검증
if (tmpYear == curTime.GetYear() && tmpMonth < curTime.GetMonth())
{
    // 만료된 카드
}
```

## 6. 데이터베이스 스키마 (관련 필드)

### KICC_SHOP_ORDER 테이블

| 필드 | 타입 | 설명 |
|------|------|------|
| `order_no` | varchar(32) | 주문번호 |
| `reg_date` | datetime | 주문 등록일시 (**유효기간 기준**) |
| `payment_code` | char(1) | 결제상태 ('0'=미결제, '1'=완료) |
| `phone_no` | varchar(32) | 고객 전화번호 |
| `terminal_id` | varchar(32) | 가맹점 ID |
| `AUTH_NO` | varchar(12) | 인증번호 (다중 주문 그룹화) |
| `RESERVED_3` | varchar(64) | 카드 유효기간 (YYMM) |
| `RESERVED_4` | varchar(64) | 카드번호 앞 12자리 |
| `RESERVED_5` | varchar(64) | 할부개월 |

## 7. 흐름도

```
┌──────────────────────┐
│  고객 전화번호 입력   │
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│  저장프로시저 호출    │
│  sp_getKicc*Info()   │
└──────────┬───────────┘
           │
           ▼
┌──────────────────────────────────────┐
│           유효기간 검증               │
│  DATEDIFF(dd, reg_date, NOW) <= 7   │
└──────────┬───────────────────────────┘
           │
     ┌─────┴─────┐
     │           │
     ▼           ▼
 ┌───────┐   ┌──────────────┐
 │ 7일내  │   │ 7일 초과     │
 │(조회됨)│   │ (조회 안됨)   │
 └───┬───┘   └──────┬───────┘
     │              │
     ▼              ▼
┌─────────────┐  ┌──────────────────┐
│ 주문정보 안내 │  │ "주문이 접수되지  │
│ → 결제 진행  │  │  않았습니다" 안내 │
└─────────────┘  └──────────────────┘
```

## 8. 결론

### 8.1 핵심 사항

1. **주문 유효기간 검증이 존재함**: `reg_date` 기준 7일 이내 주문만 조회
2. **검증 위치**: 데이터베이스 저장프로시저 (SQL WHERE 절)
3. **검증 시점**: 주문 조회 시 (전화번호 입력 후)
4. **결과**: 7일 초과 주문은 조회되지 않으며, "주문이 접수되지 않았습니다" 안내

### 8.2 주의사항

- 주문 등록 후 7일이 지나면 해당 주문은 IVR 시스템에서 더 이상 조회/결제할 수 없음
- 별도의 만료일 필드는 없으며, `reg_date`를 기준으로 상대적 계산

### 8.3 관련 파일

| 파일 | 역할 |
|------|------|
| `ADODB.cpp` | 저장프로시저 호출 및 결과 처리 |
| `KICC_Travelport_Scenario.cpp` | 조회 결과 처리 및 사용자 안내 |
| `docs/sp_getKiccOrderInfoByTel2.txt` | 단일 주문 조회 SP |
| `docs/PLAN_MULTI_KR.md` | 다중 주문 조회 SP 설계 |

---

**문서 작성일**: 2025-12-18
**분석 대상**: KICC Travelport IVR 결제 시스템
