// #include "board.h" // LCD_CTRL_GPIO 정의를 위해 남겨둘 수 있지만, LEDC 사용 안하므로 불필요
// #include "driver/ledc.h" // LEDC를 사용하여 백라이트 제어하므로 필요없음
#include "esp_log.h" // 로그를 위해 필요
// #include "esp_lvgl_port.h" // LVGL 관련이므로 제거

#include "config.h" // config_get_int를 위해 필요
#include "display.h" // 이 파일의 헤더이므로 필요
#include "system.h" // hw_type, str_hw_type을 위해 필요

// 디스플레이가 없으므로 백라이트 제어 관련 정의 및 변수 대부분은 제거
// #define DEFAULT_LCD_BRIGHTNESS 700
// #define MIN_STROBE_PERIOD      20

static const char *TAG = "WILLOW/DISPLAY";
// static int bl_duty_max; // 제거
// static int bl_duty_off; // 제거
// static int bl_duty_on; // 제거
enum willow_hw_t hw_type; // system.h에서 extern으로 선언되어 있으므로 제거 필요 없음

esp_err_t init_display(void)
{
    ESP_LOGD(TAG, "Display is not used. Skipping initialization.");
    return ESP_OK; // 디스플레이 초기화를 건너뛰고 성공 반환
}

void display_set_backlight(const bool on, const bool max)
{
    // 디스플레이 백라이트가 없으므로 아무 작업도 하지 않음
    ESP_LOGD(TAG, "Display backlight control is not enabled.");
    (void)on; // 사용되지 않는 변수 경고 억제
    (void)max; // 사용되지 않는 변수 경고 억제
}

void display_backlight_strobe_task(void *data)
{
    // 디스플레이 백라이트 스트로브 기능이 없으므로 태스크를 바로 종료
    ESP_LOGD(TAG, "Display backlight strobe task is not enabled. Deleting task.");
    // 할당된 메모리 해제 (willow_strobe_parms_t)
    if (data != NULL) {
        free(data);
    }
    vTaskDelete(NULL);
}
