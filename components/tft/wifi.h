#ifndef _wifi_h_
#define _wifi_h_

#define AP_SSID "REAMAS_AP"
#define AP_PASSPHARSE "reamaspass"

int get_wifi_connected();

int wifi_start();
void wifi_end();

void softap_start();
void softap_end();

void wifi_initialize();

#endif
