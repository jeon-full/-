#include "esp_err.h"
#include "esp_log.h"
// #include "esp_lvgl_port.h" // LVGL 포트는 더 이상 필요 없음

#include "config.h"
#include "shared.h"
// #include "slvgl.h" // slvgl.h는 이제 LVGL 관련 선언이 없으므로 필요 없음

static const char *TAG = "WILLOW/UI";

// LVGL 관련 외부 변수들은 더 이상 선언할 필요가 없음
// extern lv_disp_t *ld;
// extern lv_obj_t *btn_cancel, *lbl_btn_cancel, *lbl_ln1, *lbl_ln2, *lbl_ln3, *lbl_ln4, *lbl_ln5;
// extern int lvgl_lock_timeout;

void init_ui(void)
{
    // 디스플레이가 없으므로 UI 초기화는 건너뜀
    ESP_LOGI(TAG, "UI (display) is not enabled. Skipping UI initialization.");
    // speech_rec_mode 변수 선언은 여전히 config_get_char 함수를 사용하므로 남겨둠
    char *speech_rec_mode = config_get_char("speech_rec_mode", DEFAULT_SPEECH_REC_MODE);

    // 로그에 어떤 모드로 시작하는지 출력
    if (strcmp(speech_rec_mode, "Multinet") == 0) {
#if defined(WILLOW_SUPPORT_MULTINET)
        ESP_LOGI(TAG, "Starting up (local multinet)...");
#else
        ESP_LOGE(TAG, "Multinet Not Supported (but enabled in config)");
#endif
    } else if (strcmp(speech_rec_mode, "WIS") == 0) {
        ESP_LOGI(TAG, "Starting up (server WIS)...");
    }
    free(speech_rec_mode); // 할당된 메모리 해제
}

void ui_pr_err(char *ln3, char *ln4)
{
    // 디스플레이가 없으므로 모든 UI 업데이트 대신 콘솔에 오류를 출력
    // ld == NULL 조건문은 이제 필요 없음
    ESP_LOGE(TAG, "Error: %s", ln3 ? ln3 : "");
    if (ln4 != NULL) {
        ESP_LOGE(TAG, "        %s", ln4);
    }
    // 여기에 다른 오류 처리 추가 가능
    
}
