#include <string.h>
extern "C"{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
}

#include "setting.h"

#include "wifi.h"

#define delay(ms) (vTaskDelay(ms/portTICK_RATE_MS))

#define AP_SSID_HIDDEN 0
#define AP_MAX_CONNECTIONS 4
#define AP_AUTHMODE WIFI_AUTH_WPA2_PSK	// the passpharese should be atleast 8 chars long
#define AP_BEACON_INTERVAL 100 // in milli seconds

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;
const int CLIENT_CONNECTED_BIT = BIT0;
const int CLIENT_DISCONNECTED_BIT = BIT1;

static const char *TAG = "wifi";


int get_wifi_connected(){
	
	if(xEventGroupGetBits(wifi_event_group) == CLIENT_CONNECTED_BIT){
		printf("connected!\n");
		return 0;
	}else if(xEventGroupGetBits(wifi_event_group) == CLIENT_DISCONNECTED_BIT){
		printf("dis connected!\n");
		return -1;
	}else{
		printf("undefined!\n");
		return -1;
	}
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	printf("event_id = %d\n", event->event_id);
	switch(event->event_id) {
	case SYSTEM_EVENT_AP_START:
		printf("Event:ESP32 is started in AP mode\n");
		break;
		
	case SYSTEM_EVENT_AP_STACONNECTED:
		xEventGroupSetBits(wifi_event_group, CLIENT_CONNECTED_BIT);
		break;

	case SYSTEM_EVENT_AP_STADISCONNECTED:
		xEventGroupSetBits(wifi_event_group, CLIENT_DISCONNECTED_BIT);
		break;
		
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
		
	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		printf("got ip:%s\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		ip_address = std::string(ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		break;
		
	case SYSTEM_EVENT_STA_DISCONNECTED:
		xEventGroupSetBits(wifi_event_group, CLIENT_DISCONNECTED_BIT);
		/* This is a workaround as ESP32 WiFi libs don't currently
		   auto-reassociate. */
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		break;
		
	default:
		break;
	}
	return ESP_OK;
}

int wifi_start()
{	
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

	wifi_config_t wifi_config = {};
	sprintf((char *)wifi_config.sta.ssid , "%s" , wifi_ssid.c_str());
	sprintf((char *)wifi_config.sta.password , "%s" , wifi_pass.c_str());
	
	printf("ssid : %s\n",wifi_config.sta.ssid);
	printf("password : %s\n",wifi_config.sta.password);
	
	ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	
	int cnt = 0;
	while(1){
		printf("cnt=%d\n",cnt);
		if(xEventGroupGetBits(wifi_event_group) == CLIENT_CONNECTED_BIT){
			break;
		}
		if(xEventGroupGetBits(wifi_event_group) == CLIENT_DISCONNECTED_BIT){
			esp_wifi_stop();
			return -1;
		}
		if(cnt > 10){
			esp_wifi_stop();
			return -1;
		}
			
		vTaskDelay(1000 / portTICK_RATE_MS);
		cnt++;
	}
	
	return 0;
}

void wifi_end(){
	esp_wifi_stop();
}


static void start_dhcp_server(){

	// stop DHCP server
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
	// assign a static IP to the network interface
	tcpip_adapter_ip_info_t info;
	memset(&info, 0, sizeof(info));
	IP4_ADDR(&info.ip, 192, 168, 1, 1);
	IP4_ADDR(&info.gw, 192, 168, 1, 1);//ESP acts as router, so gw addr will be its own addr
	IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
	// start the DHCP server   
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
	printf("DHCP server started \n");
}

static void initialise_wifi_in_ap(void)
{
	esp_log_level_set("wifi", ESP_LOG_NONE); // disable wifi driver logging
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

	// configure the wifi connection and start the interface
	/*
	wifi_config_t ap_config = {
		.ap = {
			.ssid = AP_SSID,
			.password = AP_PASSPHARSE,
			.ssid_len = 0,
			.channel = 0,
			.authmode = AP_AUTHMODE,
			.ssid_hidden = AP_SSID_HIDDEN,
			.max_connection = AP_MAX_CONNECTIONS,
			.beacon_interval = AP_BEACON_INTERVAL,			
		},
	};
	*/
	wifi_config_t ap_config = {};
	sprintf((char *)ap_config.ap.ssid , "%s" , AP_SSID);
	sprintf((char *)ap_config.ap.password , "%s" , AP_PASSPHARSE);
	ap_config.ap.ssid_len = 0;
	ap_config.ap.channel = 0;
	ap_config.ap.authmode = AP_AUTHMODE;
	ap_config.ap.ssid_hidden = AP_SSID_HIDDEN;
	ap_config.ap.max_connection = AP_MAX_CONNECTIONS;
	ap_config.ap.beacon_interval = AP_BEACON_INTERVAL;
	
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	printf("ESP WiFi started in AP mode \n");
}

void softap_start(){
	start_dhcp_server();
	initialise_wifi_in_ap();
}

void softap_end(){
	
	esp_wifi_stop();
}

void wifi_initialize(){
	ESP_ERROR_CHECK( nvs_flash_init() );
	ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
	wifi_event_group = xEventGroupCreate();
	tcpip_adapter_init();
}

