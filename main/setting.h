#ifndef _setting_h_
#define _setting_h_

#include <string>
#include <vector>

/* wifi */
extern std::string wifi_ssid;
extern std::string wifi_pass;
extern std::string ip_address;

/* weather */
extern std::string weather_zipcode;

/* time */

extern int morning_time_hour;
extern int morning_time_min;
extern int evening_time_hour;
extern int evening_time_min;
extern int interval_sec;

/* talk */
extern std::vector<std::string> talk_random;
extern std::vector<std::string> talk_menu;
extern std::vector<std::string> talk_morning;
extern std::vector<std::string> talk_evening;
extern std::vector<std::string> talk_weather_today;
extern std::vector<std::string> talk_weather_tomorrow;
extern std::vector<std::string> talk_weather_end;
extern std::vector<std::string> talk_goodbye;

extern std::string talk_random_str;
extern std::string talk_menu_str;
extern std::string talk_morning_str;
extern std::string talk_evening_str;
extern std::string talk_weather_today_str;
extern std::string talk_weather_tomorrow_str;
extern std::string talk_weather_end_str;
extern std::string talk_goodbye_str;

#endif
