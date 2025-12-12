# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트 개요

KICC Travelport IVR 결제 시나리오 플러그인입니다. WinIVR 시스템에서 동적으로 로드되는 DLL로, KICC 결제 게이트웨이를 통한 신용카드 결제 처리를 담당합니다.

**기술 스택**: C++/MFC, Visual Studio 2013 (v120 toolset), Win32
**출력물**: KICC_Scenario_Travelport.dll
**결제 연동**: KICC EasyPay API (ep_cli_dll.dll)

## 빌드

```cmd
# Visual Studio 2013에서 열기
start ..\WinIVR_V1.0\WinIVR.sln

# 명령줄 빌드
msbuild KICC_Scenario_Travelport.vcxproj /p:Configuration=Release /p:Platform=Win32
msbuild KICC_Scenario_Travelport.vcxproj /p:Configuration=Debug /p:Platform=Win32
```

**빌드 출력물**:
- Debug: `KICC_Scenario_Travelport_D.dll`
- Release: `KICC_Scenario_Travelport.dll`

## 아키텍처

### 플러그인 구조

```
CKICC_Scenario (IScenario 상속)
├── ScenarioInit()      - 통화별 초기화
├── jobArs()            - ARS 진입점
└── 결제 처리 함수들
    ├── KICC_ArsScenarioStart()   - 전화번호 입력
    ├── KICC_getOrderInfo()       - 주문정보 조회
    ├── KICC_CardInput()          - 카드번호 입력
    ├── KICC_EffecDate()          - 유효기간 입력
    ├── KICC_JuminNo()            - 주민/사업자번호 입력
    ├── KICC_InstallmentCConfrim()- 할부개월 확인
    ├── KICC_CardPw()             - 비밀번호 입력
    └── KICC_payARS()             - 결제 처리
```

### 주요 파일

| 파일 | 역할 |
|------|------|
| `KICC_Scenario_Travelport.cpp` | 메인 시나리오 로직, 상태 머신 |
| `KICC_Scenario_Travelport.h` | 클래스 선언, 멤버 변수 |
| `KICCpayMent.cpp` | KICC API 연동, 결제 처리 |
| `ADODB.cpp` | 데이터베이스 접근 (ADO) |
| `KICC_Common.h` | 데이터 구조체 (CARDINFO, Card_ResInfo 등) |

### 데이터 구조

**CARDINFO** - 카드 입력 정보:
```cpp
char Card_Num[20];      // 카드번호 (13~16자리)
char ExpireDt[5];       // 유효기간 (YYMM)
char SecretNo[10+1];    // 주민번호(6자리) 또는 사업자번호(10자리)
char Password[3];       // 비밀번호 앞 2자리
char InstPeriod[3];     // 할부개월 (00~12)
```

**Card_ResInfo** - 결제 응답 정보:
```cpp
char ORDER_NO[32+1];        // 주문번호
char TERMINAL_ID[32+1];     // 가맹점ID
char REPLY_CODE[20+1];      // 응답코드 (0000=성공)
char APPROVAL_NUM[15+1];    // 승인번호
// ... 기타 필드
```

## 통화 흐름

### 표준 ARS 흐름
```
전화번호 입력 → 주문정보 조회 → 카드번호 입력 → 유효기간 입력
→ 주민번호 입력 → 할부개월 입력 → 비밀번호 입력 → 결제 처리
```

### DB 카드 모드 흐름 (간소화)
```
전화번호 입력 → 주문정보 조회 → 카드번호 뒤 4자리 입력
→ 주민번호 입력 → 비밀번호 입력 → 결제 처리
```

DB에서 카드정보(RESERVED_3/4/5)를 읽어 유효기간/할부개월 입력을 생략합니다.

## 설정 파일

**`KiccPay_Travelport_para.ini`**:
```ini
KICC_GW_URL=testgw.easypay.co.kr   # 테스트: testgw, 운영: gw
KICC_GW_PORT=80
KICC_MAII_ID=T5102001              # 가맹점 ID
KICC__CERT_FILE=./cert/pg_cert.pem # 인증서 경로
KICC_LOG=./KiccLog                 # 로그 디렉터리
```

## 에러 코드

| 코드 | 의미 |
|------|------|
| `0000` | 성공 |
| `E106` | 가맹점 ID FORMAT 오류 |
| `-1` | 시스템 오류 (m_PaySysCd, m_DBAccess) |

## 관련 문서

상세 구현 문서는 `docs/` 디렉터리를 참조하세요:

| 문서 | 내용 |
|------|------|
| [ANALYZE_KICC.md](docs/ANALYZE_KICC.md) | ARS 시나리오 흐름 상세 분석 |
| [PLAN_KICC_USING_DB.md](docs/PLAN_KICC_USING_DB.md) | DB 필드 사용 구현 계획 |
| [PLAN_MULTI_KR.md](docs/PLAN_MULTI_KR.md) | 다중 주문 결제 처리 설계 |
| [PLAN_TRAVELPORT_JUMIN.md](docs/PLAN_TRAVELPORT_JUMIN.md) | 주민번호 입력 생략 계획 |
| [IMPLEMENT_PHASE1.md](docs/IMPLEMENT_PHASE1.md) | Phase 1 구현 보고서 |
| [IMPLEMENT_TRAVELPORT_JUMIN.md](docs/IMPLEMENT_TRAVELPORT_JUMIN.md) | 주민번호 생략 구현 완료 |
| [IMPLEMENTATION_STATUS.md](docs/IMPLEMENTATION_STATUS.md) | DB 필드 구현 진행 상황 |
| [IMPLEMENT_CURSOR.md](docs/IMPLEMENT_CURSOR.md) | 구현 완료 상태 확인 |
| [SUMMARY.md](docs/SUMMARY.md) | 다중 주문 기능 구현 요약 |

## 개발 가이드

### 새로운 기능 추가 시
1. `docs/` 디렉터리에 PLAN_*.md 문서 작성
2. 기존 함수 수정 시 `[MODIFIED]` 또는 `[NEW]` 주석 추가
3. 디버그 로그는 `info_printf()` 또는 `eprintf()` 사용
    - `info_printf()`는 간단한 진행 상태를 나타낼 때 사용
    - `eprintf()`는 간단한 진행 상태를 포함해서 대부분 내용을 나타낼 때 사용    
4. 기존 코드 삭제 금지 - 조건부 분기로 처리

### 상태 머신 패턴
각 함수는 `state` 파라미터로 상태를 관리합니다:
```cpp
int KICC_함수명(int state)
{
    switch(state)
    {
    case 0:  // 초기 상태
    case 1:  // 검증/처리
    case 2:  // TTS 재생 완료 대기
    case 3:  // 사용자 확인
    }
}
```

### 비동기 처리
`setPostfunc()`를 통해 비동기 이벤트를 처리합니다:
```cpp
setPostfunc(POST_NET, 다음함수, 다음상태, 타임아웃);
return 비동기작업_host(대기음악);
```

## 디버깅

**로그 출력**:
- `info_printf(ch, ...)` - IVR 어플리케이션의 제한된 UI에 출력. 간단한 진행 상태만 표시
- `eprintf(...)` - 로그 파일 출력. info_printf 내용 포함 + 디테일한 정보/데이터 로그. 채널 정보는 내부 처리됨

**KICC DLL 로그**: `KICC_LOG` 디렉터리에 자동 저장
