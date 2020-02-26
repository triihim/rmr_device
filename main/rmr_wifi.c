#include "rmr_wifi.h"
#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "rmr_config.h"

static wifi_credentials_t credentials;
static int has_credentials = 0;
static int has_ip = 0;

static void wifi_event_handler(void *arg, esp_event_base_t e_base, int32_t e_id, void *e_data) {
	printf("Event handler called. %s, %d\n", e_base, e_id);
	if (e_base == WIFI_EVENT && e_id == WIFI_EVENT_AP_START) {
		printf("AP Start\n");
	} else if (e_base == WIFI_EVENT && e_id == WIFI_EVENT_AP_STACONNECTED) {
		printf("STACONNECTED\n");
	} else if(e_base == WIFI_EVENT && e_id == WIFI_EVENT_STA_START) {
		printf("Connecting...\n");
		ESP_ERROR_CHECK(esp_wifi_connect());
	} else if (e_base == IP_EVENT && e_id == IP_EVENT_STA_GOT_IP) {
		printf("Got IP\n");
		has_ip = 1;
	} else if(e_base == WIFI_EVENT && e_id == WIFI_EVENT_STA_DISCONNECTED) {
		printf("STA Disconnected\n");
	}
}

void init_rmr_wifi(void) {
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
}

int wifi_has_credentials(void) {
	return has_credentials;
}

int sta_has_ip(void) {
	return has_ip;
}

void init_wifi_ap(void) {
	esp_netif_create_default_wifi_ap();
	wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&config));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

	wifi_config_t ap_config = {
		.ap = {
			.ssid = AP_SSID,
			.ssid_len = strlen(AP_SSID),
			.password = AP_PASSWORD,
			.max_connection = 5,
			.authmode = WIFI_AUTH_WPA_WPA2_PSK
		},
	};	

	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	printf("WiFi AP starting...\n");
}

void init_wifi_sta(void) {
	esp_netif_create_default_wifi_sta();
	wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&config));

	wifi_config_t sta_config;
	memset(&sta_config, 0, sizeof(sta_config));
	strcpy((char *)sta_config.sta.ssid, credentials.ssid);
	strcpy((char *)sta_config.sta.password, credentials.password);

	printf("\nUsing credentials: \n%s: %d, \n%s: %d\n", sta_config.sta.ssid, 
															strlen((const char*)sta_config.sta.ssid), 
															sta_config.sta.password, 
															strlen((const char*)sta_config.sta.password));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	printf("WiFi STA starting...\n");
}

void parse_wifi_credentials(char *data, wifi_credentials_t *credentials) {
	size_t data_len = strlen(data);
	char _ssid[40];
	char _password[40];
	memset(_ssid, 0, sizeof(_ssid));
	memset(_password, 0, sizeof(_password));
	char *selected_buffer = _ssid;
	for(int i = 0; i < data_len; i++) {
		if(data[i] == '|') {
			*selected_buffer = '\0';
			selected_buffer = _password;
		} else if(i >= data_len) {
			*selected_buffer = '\0';
		} else {
			*(selected_buffer++) = data[i];
		}
	}
	strcpy(credentials->ssid, _ssid);
	strcpy(credentials->password, _password);
}

void store_wifi_credentials(wifi_credentials_t *credentials_to_store) {
	strcpy(credentials.ssid, (const char*)credentials_to_store->ssid);
	strcpy(credentials.password, (const char*)credentials_to_store->password);
	has_credentials = 1;

	nvs_handle_t nvs_handle;
	esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
	if(err != ESP_OK) {
		printf("Error opening NVS\n");
	} else {
		printf("Storing to NVS\n");
		ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "ssid", credentials.ssid));
		ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "password", credentials.password));
		nvs_commit(nvs_handle);
		nvs_close(nvs_handle);
	}	
}

void load_wifi_credentials(void) {
	printf("Loading wifi credentials from NVS\n");
	nvs_handle_t nvs_handle;
	esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
	if(err != ESP_OK) {
		printf("Error opening NVS\n");
	} else {
		uint32_t ssid_len = 0;
		uint32_t password_len = 0;
		ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "ssid", 0, &ssid_len)); // Get required len;
		ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "ssid", credentials.ssid, &ssid_len));
		ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "password", 0, &password_len)); // Get required len;
		ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "password", credentials.password, &password_len));
		nvs_close(nvs_handle);
		has_credentials = 1;
	}
}