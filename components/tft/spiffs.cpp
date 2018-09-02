#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <sstream>  
#include <string>
#include <vector>
#include <algorithm>
#include <stdlib.h>
extern "C"{
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
}
#include "wifi.h"
#include "setting.h"

static const char *TAG = "spiffs";

std::string wifi_ssid_s = "wifi_ssid=";
std::string wifi_pass_s = "wifi_pass=";
std::string talk_menu_s = "talk_menu=";
std::string morning_time_hour_s = "morning_time_hour=";
std::string morning_time_min_s = "morning_time_min=";
std::string talk_morning_s = "talk_morning=";
std::string evening_time_hour_s = "evening_time_hour=";
std::string evening_time_min_s = "evening_time_min=";
std::string talk_evening_s = "talk_evening=";
std::string weather_zipcode_s = "weather_zipcode=";
std::string talk_weather_today_s = "talk_weather_today=";
std::string talk_weather_tomorrow_s = "talk_weather_tomorrow=";
std::string talk_weather_end_s = "talk_weather_end=";
std::string interval_sec_s = "interval_sec=";
std::string talk_random_s = "talk_random=";
std::string talk_goodbye_s = "talk_goodbye=";

void spiffs_start(void)
{
	esp_vfs_spiffs_conf_t conf = {};
	
	conf.base_path = "/spiffs";
	conf.partition_label = NULL;
	conf.max_files = 5;
	conf.format_if_mount_failed = true;
	
	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%d)", ret);
		}
		return;
	}
	
	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information");
	} else {
		ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	}
}

void spiffs_end(void)
{
	esp_vfs_spiffs_unregister(NULL);
	ESP_LOGI(TAG, "SPIFFS unmounted");
}

void setting_initialize(void){
	/* wifi */
	wifi_ssid = std::string("");
	wifi_pass = std::string("");
	
	/* weather */
	weather_zipcode = std::string("");

	/* time */
	morning_time_hour = 6;
	morning_time_min = 0;
	evening_time_hour = 22;
	evening_time_min = 0;

	interval_sec = 60;

	/* talk */
	talk_random_str = std::string("");
	talk_menu_str = std::string("");
	talk_morning_str = std::string("");
	talk_evening_str = std::string("");
	talk_goodbye_str = std::string("");
	talk_weather_today_str = std::string("");
	talk_weather_tomorrow_str = std::string("");	
	talk_weather_end_str = std::string("");
	
	talk_random.clear();
	talk_menu.clear();
	talk_morning.clear();
	talk_evening.clear();
	talk_goodbye.clear();
	talk_weather_today.clear();
	talk_weather_tomorrow.clear();
	talk_weather_end.clear();
}


std::string urlDecode(std::string str){
	std::string ret;
	char ch;
	int i, ii, len = str.length();

	for (i=0; i < len; i++){
		if(str[i] != '%'){
			if(str[i] == '+')
				ret += ' ';
			else
				ret += str[i];
		}else{
			sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
			ch = static_cast<char>(ii);
			ret += ch;
			i = i + 2;
		}
	}
	return ret;
}

void talk_split(std::string str, std::string name){
	std::cout << urlDecode(str) << std::endl;
	
	std::vector<std::string> v;
	std::stringstream ss(urlDecode(str).substr(name.length()));
	std::string buffer;
	while( std::getline(ss, buffer, '\n') ) {
		buffer.erase(remove(buffer.begin(), buffer.end(),'\r'), buffer.end());
		std::cout << buffer << std::endl;
		if(buffer.length() > 0){
			if(name == talk_menu_s) talk_menu.push_back(buffer);
			if(name == talk_morning_s) talk_morning.push_back(buffer);
			if(name == talk_evening_s) talk_evening.push_back(buffer);
			if(name == talk_weather_today_s) talk_weather_today.push_back(buffer);
			if(name == talk_weather_tomorrow_s) talk_weather_tomorrow.push_back(buffer);
			if(name == talk_weather_end_s) talk_weather_end.push_back(buffer);
			if(name == talk_random_s) talk_random.push_back(buffer);
			if(name == talk_goodbye_s) talk_goodbye.push_back(buffer);
		}
	}
}

int setting_read(void)
{
	setting_initialize();
	
	std::ifstream ifs("/spiffs/settings");
	std::string str;
	if (ifs.fail()){
		talk_random.push_back(std::string("木曾路はすべて山の中である"));
		for(int i=0; i<talk_random.size(); i++){
			std::cout << "talk_random=" << talk_random[i] << "\n";
			talk_random_str.append(talk_random[i]);
			talk_random_str.append("\n");
		}
		 
		talk_menu.push_back(std::string("メニュー"));
		for(int j=0; j<talk_menu.size(); j++){
			std::cout << "menu=" << talk_menu[j] << "\n";
			talk_menu_str.append(talk_menu[j]);
			talk_menu_str.append("\n");
		}
 
		talk_morning.push_back(std::string("おはようございます"));
		for(int j=0; j<talk_morning.size(); j++){
			std::cout << "morning=" << talk_morning[j] << "\n";
			talk_morning_str.append(talk_morning[j]);
				talk_morning_str.append("\n");
		}
 
		talk_evening.push_back(std::string("おやすみなさい"));
		for(int j=0; j<talk_evening.size(); j++){
			std::cout << "evening=" << talk_evening[j] << "\n";
			talk_evening_str.append(talk_evening[j]);
			talk_evening_str.append("\n");
		}
 
		talk_weather_today.push_back(std::string("今日の天気"));
		for(int j=0; j<talk_weather_today.size(); j++){
			std::cout << "weather_today=" << talk_weather_today[j] << "\n";
			talk_weather_today_str.append(talk_weather_today[j]);
			talk_weather_today_str.append("\n");
		}
 
		talk_weather_tomorrow.push_back(std::string("明日の天気"));
		for(int j=0; j<talk_weather_tomorrow.size(); j++){
			std::cout << "weather_tomorrow=" << talk_weather_tomorrow[j] << "\n";
			talk_weather_tomorrow_str.append(talk_weather_tomorrow[j]);
			talk_weather_tomorrow_str.append("\n");
		}
 
		talk_weather_end.push_back(std::string("天気予報はありません"));
		for(int j=0; j<talk_weather_end.size(); j++){
			std::cout << "weather_end=" << talk_weather_end[j] << "\n";
			talk_weather_end_str.append(talk_weather_end[j]);
			talk_weather_end_str.append("\n");
		}
		 
		talk_goodbye.push_back(std::string("それでは"));
		for(int j=0; j<talk_goodbye.size(); j++){
			std::cout << "talk_goodbye=" << talk_goodbye[j] << "\n";
			talk_goodbye_str.append(talk_goodbye[j]);
			talk_goodbye_str.append("\n");
		}
		
		return -1;
	}

	
	while(getline(ifs, str)){
		std::cout << "'" << str << "'" << std::endl;
		
		//wifi
		if(std::equal(std::begin(wifi_ssid_s), std::end(wifi_ssid_s), std::begin(str))){
			std::cout << urlDecode(str) << std::endl;
			wifi_ssid = urlDecode(str).substr(wifi_ssid_s.length());
			wifi_ssid.erase(remove(wifi_ssid.begin(), wifi_ssid.end(),'\n'), wifi_ssid.end());
		}
		if(std::equal(std::begin(wifi_pass_s), std::end(wifi_pass_s), std::begin(str))){
			std::cout << urlDecode(str) << std::endl;
			wifi_pass = urlDecode(str).substr(wifi_pass_s.length());
			wifi_pass.erase(remove(wifi_pass.begin(), wifi_pass.end(),'\n'), wifi_pass.end());
		}
		
		//menu
		if(std::equal(std::begin(talk_menu_s), std::end(talk_menu_s), std::begin(str))){
			talk_split(str, talk_menu_s);
		}
		
		//morning
		if(std::equal(std::begin(morning_time_hour_s), std::end(morning_time_hour_s), std::begin(str))){
			std::cout << urlDecode(str) << std::endl;
			std::string morning_time_hour_i = urlDecode(str).substr(morning_time_hour_s.length());
			morning_time_hour_i.erase(remove(morning_time_hour_i.begin(), morning_time_hour_i.end(),'\n'), morning_time_hour_i.end());
			morning_time_hour = atoi(morning_time_hour_i.c_str());
		}
		if(std::equal(std::begin(morning_time_min_s), std::end(morning_time_min_s), std::begin(str))){
			std::cout << urlDecode(str) << std::endl;
			std::string morning_time_min_i = urlDecode(str).substr(morning_time_min_s.length());
			morning_time_min_i.erase(remove(morning_time_min_i.begin(), morning_time_min_i.end(),'\n'), morning_time_min_i.end());
			morning_time_min = atoi(morning_time_min_i.c_str());
		}
		if(std::equal(std::begin(talk_morning_s), std::end(talk_morning_s), std::begin(str))){
			talk_split(str, talk_morning_s);
		}
		
		//evening
		if(std::equal(std::begin(evening_time_hour_s), std::end(evening_time_hour_s), std::begin(str))){
			std::cout << urlDecode(str) << std::endl;
			std::string evening_time_hour_i = urlDecode(str).substr(evening_time_hour_s.length());
			evening_time_hour_i.erase(remove(evening_time_hour_i.begin(), evening_time_hour_i.end(),'\n'), evening_time_hour_i.end());
			evening_time_hour = atoi(evening_time_hour_i.c_str());
		}
		if(std::equal(std::begin(evening_time_min_s), std::end(evening_time_min_s), std::begin(str))){
			std::cout << urlDecode(str) << std::endl;
			std::string evening_time_min_i = urlDecode(str).substr(evening_time_min_s.length());
			evening_time_min_i.erase(remove(evening_time_min_i.begin(), evening_time_min_i.end(),'\n'), evening_time_min_i.end());
			evening_time_min = atoi(evening_time_min_i.c_str());
		}
		if(std::equal(std::begin(talk_evening_s), std::end(talk_evening_s), std::begin(str))){
			talk_split(str, talk_evening_s);
		}
		
		//weather
		if(std::equal(std::begin(weather_zipcode_s), std::end(weather_zipcode_s), std::begin(str))){
			std::cout << urlDecode(str) << std::endl;
			weather_zipcode = urlDecode(str).substr(weather_zipcode_s.length());
			weather_zipcode.erase(remove(weather_zipcode.begin(), weather_zipcode.end(),'\n'), weather_zipcode.end());
		}
		if(std::equal(std::begin(talk_weather_today_s), std::end(talk_weather_today_s), std::begin(str))){
			talk_split(str, talk_weather_today_s);
		}
		if(std::equal(std::begin(talk_weather_tomorrow_s), std::end(talk_weather_tomorrow_s), std::begin(str))){
			talk_split(str, talk_weather_tomorrow_s);
		}
		if(std::equal(std::begin(talk_weather_end_s), std::end(talk_weather_end_s), std::begin(str))){
			talk_split(str, talk_weather_end_s);
		}
		
		//random talk
		if(std::equal(std::begin(interval_sec_s), std::end(interval_sec_s), std::begin(str))){
			std::cout << urlDecode(str) << std::endl;
			std::string interval_sec_i = urlDecode(str).substr(interval_sec_s.length());
			interval_sec_i.erase(remove(interval_sec_i.begin(), interval_sec_i.end(),'\n'), interval_sec_i.end());
			interval_sec = atoi(interval_sec_i.c_str());
		}
		if(std::equal(std::begin(talk_random_s), std::end(talk_random_s), std::begin(str))){
			talk_split(str, talk_random_s);
		}
		
		//sleep
		if(std::equal(std::begin(talk_goodbye_s), std::end(talk_goodbye_s), std::begin(str))){
			talk_split(str, talk_goodbye_s);
		}
	}

	if(talk_random.size()==0){
		talk_random.push_back(std::string("[設定]でトークを登録してください"));
	}
	for(int i=0; i<talk_random.size(); i++){
		std::cout << "talk_random=" << talk_random[i] << "\n";
		talk_random_str.append(talk_random[i]);
		talk_random_str.append("\n");
	}
	
	if(talk_menu.size()==0){
		talk_menu.push_back(std::string("メニュー"));
	}
	for(int j=0; j<talk_menu.size(); j++){
		std::cout << "menu=" << talk_menu[j] << "\n";
		talk_menu_str.append(talk_menu[j]);
		talk_menu_str.append("\n");
	}

	if(talk_morning.size()==0){
		talk_morning.push_back(std::string("おはようございます"));
	}
	for(int j=0; j<talk_morning.size(); j++){
		std::cout << "morning=" << talk_morning[j] << "\n";
		talk_morning_str.append(talk_morning[j]);
		talk_morning_str.append("\n");
	}

	if(talk_evening.size()==0){
		talk_evening.push_back(std::string("おやすみなさい"));
	}
	for(int j=0; j<talk_evening.size(); j++){
		std::cout << "evening=" << talk_evening[j] << "\n";
		talk_evening_str.append(talk_evening[j]);
		talk_evening_str.append("\n");
	}

	if(talk_weather_today.size()==0){
		talk_weather_today.push_back(std::string("今日の天気"));
	}
	for(int j=0; j<talk_weather_today.size(); j++){
		std::cout << "weather_today=" << talk_weather_today[j] << "\n";
		talk_weather_today_str.append(talk_weather_today[j]);
		talk_weather_today_str.append("\n");
	}

	if(talk_weather_tomorrow.size()==0){
		talk_weather_tomorrow.push_back(std::string("明日の天気"));
	}
	for(int j=0; j<talk_weather_tomorrow.size(); j++){
		std::cout << "weather_tomorrow=" << talk_weather_tomorrow[j] << "\n";
		talk_weather_tomorrow_str.append(talk_weather_tomorrow[j]);
		talk_weather_tomorrow_str.append("\n");
	}

	if(talk_weather_end.size()==0){
		talk_weather_end.push_back(std::string("天気予報はありません"));
	}
	for(int j=0; j<talk_weather_end.size(); j++){
		std::cout << "weather_end=" << talk_weather_end[j] << "\n";
		talk_weather_end_str.append(talk_weather_end[j]);
		talk_weather_end_str.append("\n");
	}

	if(talk_goodbye.size()==0){
		talk_goodbye.push_back(std::string("それでは"));
	}
	for(int j=0; j<talk_goodbye.size(); j++){
		std::cout << "talk_goodbye=" << talk_goodbye[j] << "\n";
		talk_goodbye_str.append(talk_goodbye[j]);
		talk_goodbye_str.append("\n");
	}
	
	ifs.close();

	return 0;
}
