#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <stdio.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <stdlib.h>

extern "C"{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_spiffs.h"

#include "lwip/err.h"
#include "string.h"

#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "tft.h"
#include "parson.h"
}
#include "http.h"
#include "tftjp.h"
#include "setting.h"
#include "spiffs.h"

#define delay(ms) (vTaskDelay(ms/portTICK_RATE_MS))

const static char http_html_hdr[] =
	"HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";

const static char http_complete_hml[] = "<!DOCTYPE html>"
	"<html>\n"
	"<head>\n"
	"	<meta http-equiv='Content-Type' content='text/html; charset=UTF-8'>\n"
	"	<title>設定完了</title>\n"
	"</head>\n"
	"<body>\n"
	"	<h1>設定完了</h1>\n"
	"	本体画面の[設定完了]を押して再起動してください\n"
	"</body>\n"
	"</html>\n";
	  
static const char *TAG = "http";

static void http_server_netconn_serve(struct netconn *conn){
	struct netbuf *inbuf;
	char *buf;
	u16_t buflen;
	err_t err;
	
	std::string buf_str = std::string("");
	
	netconn_set_recvtimeout (conn, 1000);
	while(1){
		err = netconn_recv(conn, &inbuf);
		if(err != ERR_OK){
			//printf("netconn_recv err!\n");
			break;
		}
		netbuf_data(inbuf, (void**)&buf, &buflen);
		buf[buflen] = '\0';
		buf_str = buf_str + buf;
	}
	
	if(!buf_str.compare(0, 6, "GET / ")){
		printf("***GET***\n");
		
		char tmp[3];
		
		std::string hour_m = std::string("");
		
		for(int i=0;i<24;i++){
			sprintf(tmp,"%02d",i);
			hour_m.append("<option value='");
			hour_m.append(tmp);
			if(i == morning_time_hour){
				hour_m.append("' selected>");
			}else{
				hour_m.append("'>");
			}
			hour_m.append(tmp);
			hour_m.append("</option>");
		}
		
		std::string min_m = std::string("");
		
		for(int j=0;j<60;j++){
			sprintf(tmp,"%02d",j);
			min_m.append("<option value='");
			min_m.append(tmp);
			if(j == morning_time_min){
				min_m.append("' selected>");
			}else{
				min_m.append("'>");
			}
			min_m.append(tmp);
			min_m.append("</option>");
		}
		
		std::string hour_e = std::string("");
		
		for(int k=0;k<24;k++){
			sprintf(tmp,"%02d",k);
			hour_e.append("<option value='");
			hour_e.append(tmp);
			if(k == evening_time_hour){
				hour_e.append("' selected>");
			}else{
				hour_e.append("'>");
			}
			hour_e.append(tmp);
			hour_e.append("</option>");
		}
		
		std::string min_e = std::string("");
		
		for(int m=0;m<60;m++){
			sprintf(tmp,"%02d",m);
			min_e.append("<option value='");
			min_e.append(tmp);
			if(m == evening_time_min){
				min_e.append("' selected>");
			}else{
				min_e.append("'>");
			}
			min_e.append(tmp);
			min_e.append("</option>");
		}
		
		
		int interval_list_n[] = {30,60,120,180,240,300,600,900,1800,3600};
		std::string interval_list_c[] = {"30秒","1分","2分","3分","4分","5分","10分","15分","30分","1時間"};
		
		std::string sec_i = std::string("");
		
		for(int n=0;n<10;n++){
			sprintf(tmp,"%d",interval_list_n[n]);
			sec_i.append("<option value='");
			sec_i.append(tmp);
			if(interval_list_n[n] == interval_sec){
				sec_i.append("' selected>");
			}else{
				sec_i.append("'>");
			}
			sec_i.append(interval_list_c[n]);
			sec_i.append("</option>");
		}
	
		std::string http_setting_hml = std::string("<!DOCTYPE html>"
			"<html>\n"
			"<head>\n"
			"	<meta http-equiv='Content-Type' content='text/html; charset=UTF-8'>\n"
			"	<title>設定</title>\n"
			"</head>\n"
			"<body>\n"
			"<h1>設定</h1>\n"
			"<form method='post' accept-charset='UTF-8'>\n"
			"	<hr>\n"
			"	<h2>WiFi</h2>\n"
			"	<h3>SSID</h3>\n"
			"	<input type='text' name='wifi_ssid' value='") + wifi_ssid + "'><br>\n"
			"	<h3>パスワード</h3>\n"
			"	<input type='password' name='wifi_pass' value='" + wifi_pass + "'><br><br>\n"
			"	<hr>\n"
			"	<h2>メニュー</h2>\n"
			"	<h3>メニュー</h3>\n"
			"	<textarea name='talk_menu' cols='100' rows='10' maxlength='200'>" + talk_menu_str + "</textarea><br>\n"
			"	<hr>\n"
			"	<h2>アラーム</h2>\n"
			"	<h3>おはようの時間</h3>\n"
			"	<select name='morning_time_hour'>" + hour_m + "</select>時"
			"	<select name='morning_time_min'>" + min_m + "</select>分<br>\n"
			"	<h3>朝のあいさつ</h3>\n"
			"	<textarea name='talk_morning' cols='100' rows='10' maxlength='200'>" + talk_morning_str + "</textarea><br>\n"
			"	<h3>おやすみの時間</h3>\n"
			"	<select name='evening_time_hour'>" + hour_e + "</select>時"
			"	<select name='evening_time_min'>" + min_e + "</select>分<br>\n"
			"	<h3>夜のあいさつ</h3>\n"
			"	<textarea name='talk_evening' cols='100' rows='10' maxlength='200'>" + talk_evening_str + "</textarea><br>\n"
			"	<hr>\n"
			"	<h2>天気予報</h2>\n"
			"	<h3>天気予報地点（郵便番号を「xxx-xxxx」の形で入力）</h3>\n"
			"	<input type='text' name='weather_zipcode'  maxlength='8' value='" + weather_zipcode + "'><br>\n"
			"	<h3>今日の天気予報</h3>\n"
			"	<textarea name='talk_weather_today' cols='100' rows='10' maxlength='200'>" + talk_weather_today_str + "</textarea><br>\n"
			"	<h3>明日の天気予報</h3>\n"
			"	<textarea name='talk_weather_tomorrow' cols='100' rows='10' maxlength='200'>" + talk_weather_tomorrow_str + "</textarea><br>\n"
			"	<h3>天気予報が終わった場合</h3>\n"
			"	<textarea name='talk_weather_end' cols='100' rows='10' maxlength='200'>" + talk_weather_end_str + "</textarea><br>\n"
			"	<hr>\n"
			"	<h2>ランダムトーク</h2>\n"
			"	<h3>ランダムトーク間隔</h3>\n"
			"	<select name='interval_sec'>" + sec_i + "</select><br>\n"
			"	<h3>ランダムトーク</h3>\n"
			"	<textarea name='talk_random' cols='100' rows='10' maxlength='1000'>" + talk_random_str + "</textarea><br>\n"
			"	<hr>\n"
			"	<h2>スリープ</h2>\n"
			"	<h3>おやすみのあいさつ</h3>\n"
			"	<textarea name='talk_goodbye' cols='100' rows='10' maxlength='200'>" + talk_goodbye_str + "</textarea><br>\n"
			"	<hr>\n"
			"	<input type='submit'><br>\n"
			"</form>\n"
			"</body>\n"
			"</html>\n";
		
		netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
		netconn_write(conn, http_setting_hml.c_str(), http_setting_hml.size()-1, NETCONN_NOCOPY);
	}
	
	if(!buf_str.compare(0, 7, "POST / ")){
		
		printf("***POST***\n");
		
		int en;
		en = (int)buf_str.find("\r\n\r\n");
		if(en == -1){
			en = (int)buf_str.find("\n\n");
			if(en == -1){
				std::cout << "NULL\n";
				return;
			}
			en = en + 2;
		}else{
			en = en + 4;
		}
		
		buf_str = buf_str.substr(en, buf_str.size()-en);
		
		std::replace(buf_str.begin(), buf_str.end(), '&', '\n');
		
		std::cout << buf_str << "\n";
		
		std::ofstream ofs;
		ofs.open("/spiffs/settings");
		ofs << buf_str << "\n";

		ofs.close();

		netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
		netconn_write(conn, http_complete_hml, sizeof(http_complete_hml)-1, NETCONN_NOCOPY);
		
	}
	
	netconn_close(conn);
	netbuf_delete(inbuf);
}

void http_server(){
	struct netconn *conn, *newconn;
	err_t err;
	
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, 80);
	netconn_listen(conn);
	do {
		err = netconn_accept(conn, &newconn);
		if (err == ERR_OK) {
			http_server_netconn_serve(newconn);
			netconn_delete(newconn);
		}
		
	} while(err == ERR_OK);
	netconn_close(conn);
	netconn_delete(conn);
}


