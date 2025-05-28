// #include "periph_lcd.h" // 더 이상 필요 없음

// typedef struct { // 더 이상 필요 없음
//     int period_ms;
// } willow_strobe_parms_t;

// extern esp_lcd_panel_handle_t hdl_lcd; // 제거

// stub 함수를 제공하여 다른 파일에서 호출될 때 컴파일 오류를 방지
#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "esp_err.h" // esp_err_t를 위해 남겨둘 수 있음

// 함수 선언만 남겨두고, 실제 구현은 display.c에서 비워둠
esp_err_t init_display(void);
void display_backlight_strobe_task(void *data);
void display_set_backlight(const bool on, const bool max);

// 여기서는 display_backlight_strobe_task 함수가 남아있으므로 정의를 남김
typedef struct {
    int period_ms;
} willow_strobe_parms_t;


#endif 
