#include <time.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <vector>
#include <iostream>
#include <string>
#include <random>

extern "C"{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "tft.h"
#include "tftspi.h"
#include "ntp.h"
}

#include "setting.h"

#include "http.h"
#include "wifi.h"

#include "weather.h"
#include "spiffs.h"
#include "gpio.h"

#include "tftjp.h"
#include "reamas.h"

/* wifi */
std::string wifi_ssid;
std::string wifi_pass;
std::string ip_address;

/* weather */
std::string weather_zipcode;

/* time */
time_t morning_time;
time_t evening_time;
time_t talk_time;

int morning_time_hour;
int morning_time_min;
int evening_time_hour;
int evening_time_min;

int interval_sec;

/* talk */
std::vector<std::string> talk_random;
std::vector<std::string> talk_menu;
std::vector<std::string> talk_morning;
std::vector<std::string> talk_evening;
std::vector<std::string> talk_weather_today;
std::vector<std::string> talk_weather_tomorrow;
std::vector<std::string> talk_weather_end;
std::vector<std::string> talk_goodbye;

std::string talk_random_str;
std::string talk_menu_str;
std::string talk_morning_str;
std::string talk_evening_str;
std::string talk_weather_today_str;
std::string talk_weather_tomorrow_str;
std::string talk_weather_end_str;
std::string talk_goodbye_str;

//=============

void touch_setting(void *pvParameters){
	
	int menu_y = TFT_Y;
	TFT_print_w("\n");
	TFT_print_w("[設定完了]\n");
	
	int tx, ty = 0;

	while (1) {
		if (TFT_read_touch(&tx, &ty, 0)) {
//			printf("%d,%d\n", tx, ty);
			if(ty >= (menu_y + FONTSIZE/2) && ty < (menu_y + FONTSIZE/2 + (FONTSIZE*2))){
				esp_restart();
			}
			vTaskDelay(500 / portTICK_PERIOD_MS);
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
		
	}
	
}

void print_setting(){
	
	TFT_reset();
	TFT_print_w("準備中...\n");
	
	if(!get_wifi_connected()){
		TFT_reset();
		TFT_print_w("接続先ネットワーク\n");
		TFT_print_w("<SSID>\n");
		TFT_print_w(const_cast<char*>(wifi_ssid.c_str()));
		TFT_print_w("\n");
		TFT_print_w("<パスワード>\n");
		TFT_print_w("********");
		TFT_print_w("\n");
		TFT_print_w("\n");
		
		TFT_print_w("設定ページ\n");
		TFT_print_w("http://");
		TFT_print_w(const_cast<char*>(ip_address.c_str()));
		TFT_print_w("/\n");
		
	}else{
		wifi_end();
		vTaskDelay(500 / portTICK_PERIOD_MS);
		
		softap_start();
		
		TFT_reset();
		TFT_print_w("接続先ネットワーク\n");
		TFT_print_w("<SSID>\n");
		TFT_print_w(AP_SSID);
		TFT_print_w("\n");
		TFT_print_w("<パスワード>\n");
		TFT_print_w(AP_PASSPHARSE);
		TFT_print_w("\n");
		TFT_print_w("\n");
	
		TFT_print_w("設定ページ\n");
		TFT_print_w("http://192.168.1.1/\n");
	}
	
	xTaskCreate(&touch_setting,"touch_setting",4086,NULL,5, NULL);
	//xTaskCreate(&http_server,"http_server",65536,NULL,5, NULL);
	
	http_server();
	
}

void print_morning(){
	TFT_reset();
	
	TFT_print_w(const_cast<char*>(talk_morning[rand()%talk_morning.size()].c_str()));
	TFT_print_w("\n");
	
	print_weather(0);
	
	int tx, ty = 0;

	vTaskDelay(500 / portTICK_PERIOD_MS);
	while (1) {
		if (TFT_read_touch(&tx, &ty, 0)) {
			//printf("%d,%d\n", tx, ty);
			if(ty >= 0 && ty <= 320){
				return;
			}
			vTaskDelay(500 / portTICK_PERIOD_MS);
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
		
	}
}

void print_evening(){
	TFT_reset();
	
	TFT_print_w(const_cast<char*>(talk_evening[rand()%talk_evening.size()].c_str()));
	TFT_print_w("\n");
	
	print_weather(1);
	
	int tx, ty = 0;

	vTaskDelay(500 / portTICK_PERIOD_MS);
	while (1) {
		if (TFT_read_touch(&tx, &ty, 0)) {
			//printf("%d,%d\n", tx, ty);
			if(ty >= 0 && ty <= 320){
				return;
			}
			vTaskDelay(500 / portTICK_PERIOD_MS);
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
		
	}
}

void print_menu(){
	
	time_t now;
	
	TFT_reset();

	TFT_print_w(const_cast<char*>(talk_menu[rand()%talk_menu.size()].c_str()));
	TFT_print_w("\n");
	int menu_y = TFT_Y;
	
	if(menu_y > 72){
		TFT_reset();

		TFT_print_w("メニューの文字が長すぎます\n");
		menu_y = TFT_Y;
	}
	
	TFT_print_w("\n");
	TFT_print_w("[何か話して]\n\n");
	TFT_print_w("[今日の天気]\n\n");
	TFT_print_w("[明日の天気]\n\n");
	TFT_print_w("[設定]\n\n");
	TFT_print_w("[おやすみ]\n");
	
	int tx, ty = 0;
	time_t t_cpu;
	
	vTaskDelay(500 / portTICK_PERIOD_MS);

	while (1) {
		
		time(&now);
		
		if(now > morning_time){
			print_morning();
			morning_time = get_next_time(morning_time_hour, morning_time_min);
			return;
		}
		if(now > evening_time){
			print_evening();
			evening_time = get_next_time(evening_time_hour, evening_time_min);
			return;
		}
		
		if (TFT_read_touch(&tx, &ty, 0)) {
			printf("%d,%d\n", tx, ty);
			if(ty >= (menu_y + FONTSIZE/2) && ty < (menu_y + FONTSIZE/2 + (FONTSIZE*2))){
				printf("何か話して\n");
				print_talk();
				return;
			}else if(ty >= (menu_y + FONTSIZE/2 + (FONTSIZE*2)) && ty < (menu_y + FONTSIZE/2 + (FONTSIZE*4))){
				printf("今日の天気\n");
				TFT_reset();
				print_weather(0);
				return;
			}else if(ty >= (menu_y + FONTSIZE/2 + (FONTSIZE*4)) && ty < (menu_y + FONTSIZE/2 + (FONTSIZE*6))){
				printf("明日の天気\n");
				TFT_reset();
				print_weather(1);
				return;
			}else if(ty >= (menu_y + FONTSIZE/2 + (FONTSIZE*6)) && ty < (menu_y + FONTSIZE/2 + (FONTSIZE*8))){
				printf("設定\n");
				print_setting();
			}else if(ty >= (menu_y + FONTSIZE/2 + (FONTSIZE*8)) && ty < (menu_y + FONTSIZE/2 + (FONTSIZE*10))){
				printf("おやすみ\n");
				print_goodbye();
				gpio_set_level(GPIO_OUTPUT_IO, HIGH);
				return;
			}
			vTaskDelay(500 / portTICK_PERIOD_MS);
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
		
	}
	
}

void print_goodbye(){
	TFT_reset();
	
	TFT_print_w(const_cast<char*>(talk_goodbye[rand()%talk_goodbye.size()].c_str()));
	TFT_print_w("\n");
	
	vTaskDelay(2000 / portTICK_PERIOD_MS);
	
	TFT_reset();

	gpio_set_level(GPIO_OUTPUT_IO, LOW);
	
	int tx, ty = 0;

	vTaskDelay(500 / portTICK_PERIOD_MS);
	
	while (1) {
		if (TFT_read_touch(&tx, &ty, 0)) {
			//printf("%d,%d\n", tx, ty);
			if(ty >= 0 && ty <= 320){
				return;
			}
			vTaskDelay(500 / portTICK_PERIOD_MS);
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
		
	}
}

void print_talk(){
	time_t next = 0;

	int tx, ty = 0;
	
	vTaskDelay(500 / portTICK_PERIOD_MS);
	while (1) {
		time_t now;
		time(&now);
		
		if(now > next){
			TFT_reset();
			
			TFT_print_w(const_cast<char*>(talk_random[rand()%talk_random.size()].c_str()));
			TFT_print_w("\n");
			
			next = now + interval_sec;
		}

		if (TFT_read_touch(&tx, &ty, 0)) {
			//printf("%d,%d\n", tx, ty);
			if(ty >= 0 && ty <= 320){
				return;
			}
			vTaskDelay(500 / portTICK_PERIOD_MS);
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
		
	}
}

void print_weather(int day){
	time_t now;
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);
	
	int unixday = mktime(&timeinfo)/86400;
	
	int weather_status = get_weather(unixday, day);
	
	if(weather_status){
		TFT_print_w("天気取得に失敗しました\n設定を確認してください\n");
		int menu_y = TFT_Y;
		TFT_print_w("\n");
		TFT_print_w("[設定]\n\n");
		TFT_print_w("[戻る]\n");
		
		int tx, ty = 0;

		while (1) {
			if (TFT_read_touch(&tx, &ty, 0)) {
				if(ty >= (menu_y + FONTSIZE/2) && ty < (menu_y + FONTSIZE/2 + (FONTSIZE*2))){
					print_setting();
				}else if(ty >= (menu_y + FONTSIZE/2 + (FONTSIZE*2)) && ty < (menu_y + FONTSIZE/2 + (FONTSIZE*4))){
					return;
				}
				vTaskDelay(500 / portTICK_PERIOD_MS);
			}
			vTaskDelay(100 / portTICK_PERIOD_MS);
			
		}
	}
	
	int tx, ty = 0;

	while (1) {
		if (TFT_read_touch(&tx, &ty, 0)) {
			//printf("%d,%d\n", tx, ty);
			if(ty >= 0 && ty <= 320){
				return;
			}
			vTaskDelay(500 / portTICK_PERIOD_MS);
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
		
	}
	
}

time_t get_next_time(int target_hour, int target_min) {
	time_t now;
	struct tm tm_time;
	time(&now);
	localtime_r(&now, &tm_time);
	
	struct tm tm_next = {0,0,0,0,0,0,0,0,0};
	tm_next.tm_year = tm_time.tm_year;
	tm_next.tm_mon = tm_time.tm_mon;
	tm_next.tm_mday = tm_time.tm_mday;
	tm_next.tm_hour = target_hour;
	tm_next.tm_min = target_min;
	tm_next.tm_sec = 0;

	if(((tm_time.tm_hour * 60) + tm_time.tm_min) >= target_hour * 60 + target_min){
		tm_next.tm_mday++;
	}
	
	printf("next=%d/%d/%d %d:%d:%d\n", tm_next.tm_year+1900, tm_next.tm_mon+1, tm_next.tm_mday, tm_next.tm_hour, tm_next.tm_min, tm_next.tm_sec);
	return mktime(&tm_next);
}



extern "C"
void app_main()
{
	gpio_start();
	TFT_initialize();
	TFT_reset();
	gpio_set_level(GPIO_OUTPUT_IO, HIGH);
	
	TFT_print_w("データ読込中...\n");
	
	wifi_initialize();
	spiffs_start();
	
	
	int setting_status = setting_read();
	if(setting_status){
		print_setting();
	}
	
	TFT_reset();
	TFT_print_w("wifi接続中...\n");
	
	int wifi_status = wifi_start();
	
	if(wifi_status){
		TFT_print_w("wifi接続に失敗しました\n設定を確認してください\n");
		int menu_y = TFT_Y;
		TFT_print_w("\n");
		TFT_print_w("[設定]\n\n");
		TFT_print_w("[wifiを使わない]\n");
		
		int tx, ty = 0;

		while (1) {
			if (TFT_read_touch(&tx, &ty, 0)) {
				if(ty >= (menu_y + FONTSIZE/2) && ty < (menu_y + FONTSIZE/2 + (FONTSIZE*2))){
					print_setting();
				}else if(ty >= (menu_y + FONTSIZE/2 + (FONTSIZE*2)) && ty < (menu_y + FONTSIZE/2 + (FONTSIZE*4))){
					break;
				}
				vTaskDelay(500 / portTICK_PERIOD_MS);
			}
			vTaskDelay(100 / portTICK_PERIOD_MS);
			
		}
	}
	
	TFT_reset();
	TFT_print_w("時刻取得中...\n");
	
	int ntp_status = set_ntp();
	
	if(ntp_status){
		TFT_print_w("時刻取得に失敗しました\n再起動してください\n");
		int menu_y = TFT_Y;
		TFT_print_w("\n");
		TFT_print_w("[再起動]\n\n");
		TFT_print_w("[アラームを使わない]\n");
		
		int tx, ty = 0;

		while (1) {
			if (TFT_read_touch(&tx, &ty, 0)) {
				if(ty >= (menu_y + FONTSIZE/2) && ty < (menu_y + FONTSIZE/2 + (FONTSIZE*2))){
					esp_restart();
				}else if(ty >= (menu_y + FONTSIZE/2 + (FONTSIZE*2)) && ty < (menu_y + FONTSIZE/2 + (FONTSIZE*4))){
					break;
				}
				vTaskDelay(500 / portTICK_PERIOD_MS);
			}
			vTaskDelay(100 / portTICK_PERIOD_MS);
			
		}
	}
	
	time_t now;
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);
	
	morning_time = get_next_time(morning_time_hour, morning_time_min);
	evening_time = get_next_time(evening_time_hour, evening_time_min);
	
	srand((unsigned)time(NULL));
	
	while(1){
		print_menu();
	}
}
