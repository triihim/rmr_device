#include "rmr_server.h"
#include "rmr_wifi.h"
#include "esp_http_server.h"
#include <sys/param.h>
#include "rmr_index.h"
#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/sys.h"

esp_err_t get_handler(httpd_req_t *req) {
	const char *response = (const char *)index_html;
	httpd_resp_set_type(req, "text/html");
	httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
	httpd_resp_send(req, response, INDEX_HTML_LEN);
	return ESP_OK;
}

esp_err_t post_handler(httpd_req_t *req) {
	char content[100];
	size_t recv_size = MIN(req->content_len, sizeof(content));
	int ret = httpd_req_recv(req, content, recv_size);
	
	if(ret <= 0) {
		if(ret == HTTPD_SOCK_ERR_TIMEOUT) {
			httpd_resp_send_408(req);
		}
		return ESP_FAIL;
	}

	wifi_credentials_t credentials;
	parse_wifi_credentials(content, &credentials);
	store_wifi_credentials(&credentials);

	const char *resp = "Credentials received";
	httpd_resp_send(req, resp, strlen(resp));

	return ESP_OK;
}

httpd_uri_t uri_get = {
	.uri 		= "/",
	.method 	= HTTP_GET,
	.handler 	= get_handler,
	.user_ctx 	= NULL,
};

httpd_uri_t uri_post = {
	.uri		= "/",
	.method 	= HTTP_POST,
	.handler	= post_handler,
	.user_ctx	= NULL,	
};

httpd_handle_t start_server(void) {
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	httpd_handle_t server = NULL;
	
	if (httpd_start(&server, &config) == ESP_OK) {
		httpd_register_uri_handler(server, &uri_get);
		httpd_register_uri_handler(server, &uri_post);
	}

	return server;
}

void stop_server(httpd_handle_t server) {
	if(server) {
		printf("Stopping server...");
		httpd_stop(server);
	}
}