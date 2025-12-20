# ARS 시나리오 Flow 정리 (Travelport / 다중주문 결제)

- 기준 소스: `KICC_Travelport_Scenario.cpp`
- 범위: **ARS(휴대폰번호 → 다중주문 조회/안내 → 카드정보 입력 → 결제(다중) → 최종안내/종료)**
- 원칙: **내부 동작 로직 설명 없이**, “고객이 듣는 멘트(안내 음원/TTS)의 재생 순서”만 정리
- 표기:
  - **[WAVE]**: `set_guide(VOC_WAVE_ID, "<경로>")`
  - **[TTS]**: `TTS_Play(..., "<문구>", ...)` (코드의 실제 문자열 그대로)
  - `input_confirm` 등 **공통 음원은 파일명만** 표기 (문구는 코드에 직접 문자열로 존재하지 않음)

---

## 0) 현재 코드/설정 기준(흐름에 영향 주는 값)

- **전화번호 확인 멘트 생략**: `SKIP_PHONE_CONFIRM = 1` (`KICC_Common.h`)
  - 전화번호 입력 후 **“고객님께서 누르신 전화번호는 …”** 확인 멘트/1·2번 선택 단계 **재생되지 않음**
- **DB 카드정보 모드 기본 ON**: `m_bUseDbCardInfo = 1` (ScenarioInit 기본값)
  - 주문조회(DB)에서 카드정보 유효성 검증이 깨지면 **수동입력 모드로 폴백** 가능 (아래 4절 참고)
- **휴대폰번호 재시도 최대 3회**: `MAX_PHONE_RETRY_COUNT = 3`
- **전부 실패 시 카드 재입력 최대 3회**: `MAX_CARD_RETRY = 3`
- **카드번호 “확인 멘트” 단계는 현재 흐름에서 스킵됨**
  - (코드 상 카드번호 입력 후 “고객님께서 누르신 카드번호는 …” 확인 단계가 비활성 처리되어 다음 단계로 바로 진행)

---

## 1) 성공 케이스만 (3건 주문 가정 / 3건 모두 결제 성공)

### 1-1. 전체 멘트 흐름(순서) — 현재 기본값(전화번호 확인 생략 + DB 카드정보 모드)

1. **(인사말)**
   - (템플릿) `audio\shop_intro\<DNIS>`  *(코드에는 템플릿만 존재)*
2. **휴대폰 번호 입력**
   - **[WAVE]** `ment\Travelport\input_telnum_start`  // "전화 번호 입력"
3. **다중주문(3건) / 총 승인금액 안내**
   - **[TTS]** `"고객님의 항공권 결제 금액은 %d건 , 총 승인금액은 %d 원입니다"`
4. **다중주문 안내 확인(진행/재입력)**
   - **[WAVE]** `ment\_common\common_audio\input_confirm`
5. **카드번호(뒤 4자리) 입력**
   - **[WAVE]** `ment/Travelport/input_card_num_4`  // "카드번호 ..우물정자를..."
6. **카드 비밀번호(앞 2자리) 입력**
   - **[WAVE]** `ment/Travelport/input_pass_start`  // "카드 비밀번호 네자리중, 앞, 두자리를 입력하여 주시기 바랍니다."
7. **결제 요청 대기 안내(1회)**
   - **[WAVE]** `ment/Travelport/pay_request_wait`  // "결제 요청 중입니다. 잠시만 기다려 주시기 바랍니다."
8. **최종 결과(3/3 성공) 안내**
   - **[TTS]** `"%d건의 결제가 완료되었습니다. 담당직원을 연결해 드리겠습니다."`
9. **종료 멘트**
   - **[WAVE]** `ment\_common\common_audio\service_end`  // "마지막 인사말"

---

## 2) 실패 케이스 포함 (3건 주문 가정 / 순서도 형태) — 현재 기본 흐름 기준

### 2-1. 공통 구간(결제 결과 분기 전까지)

`(인사말)`
→ **[WAVE]** `ment\Travelport\input_telnum_start`
→ **[TTS]** `"고객님의 항공권 결제 금액은 %d건 , 총 승인금액은 %d 원입니다"`
→ **[WAVE]** `ment\_common\common_audio\input_confirm`
→ **[WAVE]** `ment/Travelport/input_card_num_4`
→ **[WAVE]** `ment/Travelport/input_pass_start`
→ **[WAVE]** `ment/Travelport/pay_request_wait`

이후 아래 3가지 중 하나로 분기합니다.

---

### 2-2. (케이스 A) 3건 모두 성공 (성공=3, 실패=0)

공통 구간
→ **[TTS]** `"%d건의 결제가 완료되었습니다. 담당직원을 연결해 드리겠습니다."`
→ **[WAVE]** `ment\_common\common_audio\service_end`

---

### 2-3. (케이스 B) 2건 성공 + 1건 실패 (성공=2, 실패=1)

공통 구간
→ **[TTS]** `"%d건은 결제가 완료되었으며 %d건은 결제 실패하였습니다. 담당직원을 연결해 드리겠습니다."`
→ **[WAVE]** `ment\_common\common_audio\service_end`

---

### 2-4. (케이스 C) 3건 모두 실패 (성공=0, 실패=3)

공통 구간
→ **[TTS]** `"%d건 결제가 모두 실패하였습니다. 다시 결제를 진행하시려면 1번, 담당 직원을 연결하시려면 2번을 눌러 주시기 바랍니다."`
→ **(대기음)** **[WAVE]** `ment\wait_sound`
→ **고객 입력 분기**

- **1번(재결제 선택)** *(최대 `MAX_CARD_RETRY=3` 범위 내에서 반복 가능)*
  → **[WAVE]** `ment/Travelport/input_card_num_4`
  → 이후 공통 구간의 카드입력~결제대기 안내로 재진입

- **2번(담당직원 연결/종료 선택)**
  → **[WAVE]** `ment\_common\common_audio\service_end`

- **(재시도 초과 시) 종료 안내**
  → **[TTS]** `"결제에 실패하였습니다. 다시 전화해 주시기 바랍니다."`
  → **[WAVE]** `ment\_common\common_audio\service_end`

---

## 3) 설정값에 의해 갈리는 “주문조회 실패/주문없음” 흐름(참고)

아래는 결제 성공/실패와 별개로, **휴대폰번호 재시도(`MAX_PHONE_RETRY_COUNT=3`)**에 의해 반복/종료가 결정되는 구간입니다.

`휴대폰 번호 입력`
→ **[WAVE]** `ment/Travelport/no_order_msg`  // (코드 주석: "주문이 접수되지 않았습니다. 상점으로 문의하여 주시기 바랍니다.")
→ (재시도 3회 미만) **[WAVE]** `ment\Travelport\input_telnum_start` 로 복귀
→ (재시도 3회 도달) **[WAVE]** `ment\_common\common_audio\service_end`

---

## 4) (폴백) DB 카드정보 불완전 시 “수동입력 모드” 카드구간 멘트 흐름(참고)

다음 흐름은 `m_bUseDbCardInfo = 0`(폴백)일 때 카드 입력 구간에서 재생되는 멘트들입니다.

`(다중주문 안내 확인: input_confirm)`  
→ **[WAVE]** `ment/_common/common_audio/input_card_num_var`  // "카드번호 ..우물정자를..."
→ **[WAVE]** `ment/_common/common_audio/input_cardexp_start`  // "신용카드 앞 유효기간 년도와 월 순으로......"
→ **[TTS]** `"고객님께서 누르신 유효 기간은, 20%d 년, %d월 입니다."`
→ **[WAVE]** `ment\_common\common_audio\input_confirm`
→ **[WAVE]** `ment/_common/common_audio/input_halbu_start`  // "요청하실 할부 개월수를 ... 일시불은 0번...."
→ **[TTS]** `"고객님께서 요청하신 할부 개월수는 %d개월 입니다"`
→ **[WAVE]** `ment/_common/common_audio\input_confirm`
→ **[WAVE]** `ment/Travelport/input_pass_start`
→ **[WAVE]** `ment/Travelport/pay_request_wait`
