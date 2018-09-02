#include <string>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "tft.h"
#include "parson.h"
}
#include "tftjp.h"

#include "weather.h"
#include "setting.h"

#define BUF_N 64

/* タイムゾーン */
int TIMEZONE = 3600*9;

/* 天気 */
std::string OWM_SERVER = "api.openweathermap.org";
std::string OWM_APIKEY = "1dd711d4aed49c9ada4924e086e5b58a";

static const char *TAG = "weather";

int get_weather(int unixday, int day){
	
	std::string request =	"GET /data/2.5/forecast?zip=" + 
							weather_zipcode +
							",JP&units=metric&APPID=" +
							OWM_APIKEY +
							" HTTP/1.0\r\n" +
							"Host: " +
							OWM_SERVER +
							"\r\n" + 
							"User-Agent: esp-idf/1.0 esp32\r\n" +
							"\r\n";
	printf("%s\n",request.c_str());
	/*
	char *request;
	request = (char *) malloc(REQUEST_N);
	memset(request, '\0', REQUEST_N);
	
	sprintf(request,"GET %s?zip=%s,JP&units=metric&APPID=%s HTTP/1.0\r\n"
			"Host: %s\r\n"
			"User-Agent: esp-idf/1.0 esp32\r\n"
			"\r\n",
			OWM_URL,weather_zipcode.c_str(),OWM_APIKEY,OWM_SERVER);
	printf("%s\n",request);
	*/
	
	struct addrinfo hints = {};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	struct addrinfo *res;
	struct in_addr *addr;
	int s, r;
	char *recv_buf;
	
	recv_buf = (char *) malloc(BUF_N);

	/* Wait for the callback to set the CONNECTED_BIT in the
	   event group.
	*/
	int err = getaddrinfo(OWM_SERVER.c_str(), "80", &hints, &res);

	if(err != 0 || res == NULL) {
		ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
		//vTaskDelay(1000 / portTICK_PERIOD_MS);
		return -1;
	}

	/* Code to print the resolved IP.
	   Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
	addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
	ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

	s = socket(res->ai_family, res->ai_socktype, 0);
	if(s < 0) {
		ESP_LOGE(TAG, "... Failed to allocate socket.");
		freeaddrinfo(res);
		//vTaskDelay(1000 / portTICK_PERIOD_MS);
		return -1;
	}
	ESP_LOGI(TAG, "... allocated socket");

	if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
		ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
		close(s);
		freeaddrinfo(res);
		//vTaskDelay(4000 / portTICK_PERIOD_MS);
		return -1;
	}

	ESP_LOGI(TAG, "... connected");
	freeaddrinfo(res);

	if (write(s, request.c_str(), strlen(request.c_str())) < 0) {
		ESP_LOGE(TAG, "... socket send failed");
		close(s);
		//vTaskDelay(4000 / portTICK_PERIOD_MS);
		return -1;
	}
	ESP_LOGI(TAG, "... socket send success");

	struct timeval receiving_timeout;
	receiving_timeout.tv_sec = 5;
	receiving_timeout.tv_usec = 0;
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
			sizeof(receiving_timeout)) < 0) {
		ESP_LOGE(TAG, "... failed to set socket receiving timeout");
		close(s);
		//vTaskDelay(4000 / portTICK_PERIOD_MS);
		return -1;
	}
	ESP_LOGI(TAG, "... set socket receiving timeout success");

	/* Read HTTP response */
	std::string recv_str = "";
	
	do {
		memset(recv_buf, '\0', BUF_N);
		r = read(s, recv_buf, sizeof(recv_buf)-1);
		recv_str = recv_str + std::string(recv_buf);
		
	} while(r > 0);
	
	int en;
	en = (int)recv_str.find("{\"cod\":");
	if(en == -1) return -1;
	
	std::string recv_str_sub = recv_str.substr(en, recv_str.size() - en);
	
	char *recv_char = (char *) malloc(recv_str_sub.size() + 1);
	memset(recv_char, '\0', recv_str_sub.size() + 1);
	strcpy(recv_char, recv_str_sub.c_str()); 
	
	/* JSON */
	int i;
	JSON_Value *root_value = json_parse_string(recv_char);
	JSON_Object *root_obj = json_value_get_object(root_value);
	JSON_Array *list = json_object_get_array(root_obj,"list");
	
	if(strcmp(json_object_dotget_string(root_obj, "cod"),"200")){
		return -1;
	}
	
	if(day){
		TFT_print_w(const_cast<char*>(talk_weather_tomorrow[rand()%talk_weather_tomorrow.size()].c_str()));
		TFT_print_w("\n");
		unixday++;
	}else{
		TFT_print_w(const_cast<char*>(talk_weather_today[rand()%talk_weather_today.size()].c_str()));
		TFT_print_w("\n");
	}
	TFT_print_w("\n");
	
	int cnt = 0;
	for ( i = 0; i < json_array_get_count(list); i++ ){
		JSON_Object *w_obj = json_array_get_object(list, i);
		JSON_Array *weather_array = json_object_get_array(w_obj,"weather");
		JSON_Object *weather = json_array_get_object(weather_array, 0);
		
		time_t dt = json_object_dotget_number(w_obj, "dt");
		int dt_day = (dt + TIMEZONE) / 86400;
		
	    if(dt_day > unixday){
	    	if(cnt == 0){
				TFT_print_w(const_cast<char*>(talk_weather_end[rand()%talk_weather_end.size()].c_str()));
				TFT_print_w("\n");
	    	}
	    	break;
	    }
	    if(dt_day == unixday){
	   	    struct tm tm_weather;
    		localtime_r(&dt,&tm_weather);
	    	
	    	char t[8];
			memset(t, '\0', 8);
	    	sprintf(t,"%2d時",tm_weather.tm_hour);
	    	TFT_print_w(t);
			TFT_print_w("　");
	    	
			int id = json_object_dotget_number(weather, "id");

	    	if(id >= 200 && id < 300){
				TFT_print_w("雷雨　");
	    	}else if(id >= 300 && id < 400){
				TFT_print_w("霧雨　");
	    	}else if(id >= 500 && id < 600){
				TFT_print_w("雨　　");
	    	}else if(id >= 600 && id < 700){
				TFT_print_w("雪　　");
	    	}else if(id == 800){
				TFT_print_w("晴れ　");
	    	}else if(id >= 801 && id < 805){
				TFT_print_w("くもり");
	    	}else{
				TFT_print_w("不明　");
	    	}
			TFT_print_w("　");
	    	
			int temp = json_object_dotget_number(w_obj, "main.temp");
	    	
	    	char k[8];
			memset(k, '\0', 8);
	    	sprintf(k,"%2d℃\n",temp);
	    	TFT_print_w(k);
	    	
	    	cnt++;
	    }
		
	}
	
	json_value_free(root_value);
	
	free(recv_buf);
	free(recv_char);
	ESP_LOGI(TAG, "... done reading from socket. Last read errno=%d\r\n", errno);
	close(s);
	
	return 0;
	
}
