#include "errno.h"
#include "esp_app_format.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
// #include "esp_lvgl_port.h" // LVGL 포트 제거
#include "esp_ota_ops.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
// #include "lvgl.h" // LVGL 제거

#include "audio.h"
#include "config.h"
// #include "display.h" // 디스플레이 헤더 제거
#include "endpoint/hass.h"
#include "http.h"
#include "shared.h"
// #include "slvgl.h" // slvgl 헤더 제거
#include "system.h"
#include "timer.h" // 타이머 유지 (display_timer는 제거되지만, 다른 타이머 함수에 대한 의존성 있을 수 있음)
#include "was.h"

#define BUFSIZE 4096

static char ota_data[BUFSIZE + 1] = {0};
static const char *TAG = "WILLOW/OTA";

void ota_task(void *data)
{
    bool hdr_checked = false;
    char *url = (char *)data;
    const esp_partition_t *pt_boot, *pt_cur, *pt_new;
    esp_app_desc_t app_desc_cur, app_desc_new;
    esp_err_t ret = ESP_OK;
    esp_http_client_handle_t hdl_hc = init_http_client();
    esp_ota_handle_t hdl_ota = 0;
    int http_status = 0, ota_len = 0, ota_size = 0, read = 0;

    ESP_LOGI(TAG, "downloading OTA from %s", url);

    pt_boot = esp_ota_get_boot_partition();
    pt_cur = esp_ota_get_running_partition();
    pt_new = esp_ota_get_next_update_partition(NULL);

    if (pt_boot != pt_cur) {
        ESP_LOGW(TAG,
                 "boot partition (offset='0x%08" PRIu32 "') does not match running partition (offset='0x%08" PRIu32
                 "')",
                 pt_boot->address, pt_cur->address);
    }

    ret = esp_http_client_set_url(hdl_hc, url);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to set OTA URL: %s", esp_err_to_name(ret));
        goto err;
    }

    ret = esp_http_client_open(hdl_hc, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to open HTTP connection: %s", esp_err_to_name(ret));
        goto err;
    }

    ota_size = esp_http_client_fetch_headers(hdl_hc);
    ESP_LOGI(TAG, "update size: %d byte", ota_size);

    http_status = esp_http_client_get_status_code(hdl_hc);
    if (http_status != 200) {
        ESP_LOGE(TAG, "HTTP error (%d), aborting update", http_status);
        goto err;
    }

    ret = esp_ota_get_partition_description(pt_cur, &app_desc_cur);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to read current firmware version: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "current firmware version: %s", app_desc_cur.version);
    }

    while (true) {
        read = esp_http_client_read(hdl_hc, ota_data, BUFSIZE);

        if (read < 0) {
            goto err;
        }

        if (read == 0) {
            if (errno == ECONNRESET || errno == ENOTCONN) {
                ESP_LOGE(TAG, "connection problem occurred (%d)", errno);
                goto err;
            }

            if (esp_http_client_is_complete_data_received(hdl_hc)) {
                ESP_LOGI(TAG, "OTA download completed");
                break;
            }
        }

        if (read > 0) {
            if (hdr_checked == false) {
                if (read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + +sizeof(esp_app_desc_t)) {
                    memcpy(&app_desc_new, &ota_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)],
                           sizeof(esp_app_desc_t));
                    ESP_LOGI(TAG, "new firmware version: %s", app_desc_new.version);

                    hdr_checked = true;

                    // OTA begin triggers TWDT due to partition erase taking a long time
                    esp_task_wdt_config_t ota_config = {
                        .timeout_ms = 30000,
                        .idle_core_mask = 0,
                        .trigger_panic = true,
                    };
                    esp_task_wdt_reconfigure(&ota_config);

                    ESP_LOGI(TAG, "starting OTA");
                    // use OTA_SIZE_UNKNOWN to always fully erase the partition
                    ret = esp_ota_begin(pt_new, OTA_SIZE_UNKNOWN, &hdl_ota);
                    if (ret != ESP_OK) {
                        ESP_LOGE(TAG, "failed to start OTA: %s", esp_err_to_name(ret));
                        goto err;
                    }
                } else {
                    ESP_LOGE(TAG, "failed to read app info from OTA file, aborting update");
                    goto err;
                }
            }
            ret = esp_ota_write(hdl_ota, (const void *)ota_data, read);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "failed to write OTA data: %s", esp_err_to_name(ret));
                goto err;
            }
            ota_len += read;
            ESP_LOGD(TAG, "OTA data written: %d byte", ota_len);
            // add a delay to allow feeding TWDT
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }

    ESP_LOGI(TAG, "total OTA data written: %d byte", ota_len);
    ret = esp_ota_end(hdl_ota);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA end failed: %s", esp_err_to_name(ret));
        goto err;
    }

    ret = esp_ota_set_boot_partition(pt_new);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to set boot partition: %s", esp_err_to_name(ret));
        goto err;
    }

    ESP_LOGI(TAG, "OTA completed, restarting");
    // LVGL UI 관련 호출 제거
    // if (lvgl_port_lock(lvgl_lock_timeout)) {
    //     lv_label_set_text_static(lbl_ln3, "Upgrade Done");
    //     lvgl_port_unlock();
    // }
    restart_delayed();
err:
    esp_ota_abort(hdl_ota);
    esp_http_client_close(hdl_hc);
    esp_http_client_cleanup(hdl_hc);
    ESP_LOGI(TAG, "OTA failed, restarting");
    // LVGL UI 관련 호출 제거
    // if (lvgl_port_lock(lvgl_lock_timeout)) {
    //     lv_label_set_text_static(lbl_ln3, "Upgrade Failed");
    //     lvgl_port_unlock();
    // }
    restart_delayed();
    vTaskDelete(NULL);
}

void ota_start(char *url)
{
    // 디스플레이 타이머 관련 호출 제거
    // reset_timer(hdl_display_timer, config_get_int("display_timeout", DEFAULT_DISPLAY_TIMEOUT), true);
    // LVGL UI 관련 호출 제거
    // if (lvgl_port_lock(lvgl_lock_timeout)) {
    //     lv_obj_add_flag(lbl_ln1, LV_OBJ_FLAG_HIDDEN);
    //     lv_obj_add_flag(lbl_ln2, LV_OBJ_FLAG_HIDDEN);
    //     lv_obj_add_flag(lbl_ln4, LV_OBJ_FLAG_HIDDEN);
    //     lv_obj_add_flag(lbl_ln5, LV_OBJ_FLAG_HIDDEN);
    //     lv_label_set_text_static(lbl_ln3, "Starting Upgrade");
    //     lv_obj_clear_flag(lbl_ln3, LV_OBJ_FLAG_HIDDEN);
    //     lvgl_port_unlock();
    // }
    // 백라이트 제어 제거
    // display_set_backlight(true, false);
    ESP_LOGI(TAG, "Starting Upgrade"); // 콘솔 로깅으로 대체

    deinit_audio();
    deinit_hass();
    deinit_was();

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    xTaskCreate(&ota_task, "ota_task", 8192, url, 5, NULL);
}
