#include <stdio.h>
#include <string.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "rmr_wifi.h"
#include "rmr_server.h"
#include "esp_http_client.h"
#include "rmr_camera.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "rmr_config.h"

void init_nvs(void) {
	esp_err_t ret = nvs_flash_init();
	if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
}

esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            printf("Error event\n");
            break;
        case HTTP_EVENT_ON_DATA:
            printf("Data event\n");
            if (!esp_http_client_is_chunked_response(evt->client)) {
                printf("%.*s", evt->data_len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            printf("Finish event\n");
            break;
        case HTTP_EVENT_DISCONNECTED:
            printf("Disconnected event\n");
            break;
        default:
        	break;
    }
    return ESP_OK;
}

esp_err_t post_image(camera_fb_t *fb) {
	printf("Sending image...\n");
	esp_http_client_config_t config = {
	   .url = POST_API_URL,
	   .event_handler = _http_event_handle,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_post_field(client, (const char *)fb->buf, fb->len);
	esp_http_client_set_header(client, "Content-Type", "image/jpeg");
	esp_http_client_set_header(client, RMR_TAG_HEADER_NAME, RMR_TAG_HEADER_VALUE);
	esp_err_t err = esp_http_client_perform(client);

	if(err != ESP_OK) {
		printf("Post image error: %s\n", esp_err_to_name(err));
		printf("Trying once more...\n");
		ESP_ERROR_CHECK(esp_http_client_perform(client));
	}

	esp_http_client_cleanup(client);
  
    if(err != ESP_OK) {
    	printf("Error sending the image\n");
    	printf("%s\n", esp_err_to_name(err));
    	return ESP_FAIL;
    } 
    printf("Image sent\n");
    return ESP_OK;
}


void app_main(void) {

	if(rmr_camera_init() != ESP_OK) {
		esp_restart();
	}

	// If hard reset -> clear stored credentials.
	esp_reset_reason_t rst_reason = esp_reset_reason();
	if(rst_reason == ESP_RST_POWERON) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		init_nvs();
	} else {
	// Otherwise try to use stored credentials.
		init_nvs();
		load_wifi_credentials();
	}

	init_rmr_wifi();

	httpd_handle_t server;

	if(wifi_has_credentials() != 1) {
		// Start in AP mode to receive wifi credentials through web ui.
		init_wifi_ap();
		server = start_server();

		while(1) {
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			if(wifi_has_credentials() == 1) {
				// Switch to STA mode.
				stop_server(server);
				vTaskDelay(1000 / portTICK_PERIOD_MS);
				init_wifi_sta();
				break;
			}
		}

	} else {
		// Start in STA mode.
		init_wifi_sta();
	}

	esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
	printf("WAKEUP CAUSE: %d\n", wakeup_cause);

	uint8_t attempt_counter = 0;
	uint8_t attempt_limit = 10;
	while(1) {
		if(sta_has_ip() == 1) {
			camera_fb_t *fb = camera_capture();
			ESP_ERROR_CHECK(post_image(fb));
			esp_camera_fb_return(fb);
			ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(SLEEP_TIMER_US));
			printf("Going to deep sleep\n");
			esp_deep_sleep_start();
		} else if(attempt_counter < attempt_limit) {
			printf("No IP yet... waiting\n");
			attempt_counter++;
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		} else {
			esp_restart();
		}
	}
	
}
