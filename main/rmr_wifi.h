#ifndef RMR_WIFI_H
#define RMR_WIFI_H

typedef struct {
	char ssid[40];
	char password[40];
} wifi_credentials_t;

extern void init_rmr_wifi(void);

extern void parse_wifi_credentials(char *, wifi_credentials_t *);

extern void store_wifi_credentials(wifi_credentials_t *);

extern void load_wifi_credentials(void);

extern int wifi_has_credentials(void);

extern int sta_has_ip(void);

extern void init_wifi_ap(void);

extern void init_wifi_sta(void);

#endif