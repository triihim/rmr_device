#ifndef RMR_SERVER_H
#define RMR_SERVER_H
#include "esp_http_server.h"

extern httpd_handle_t start_server(void);

extern void stop_server(httpd_handle_t server);

#endif