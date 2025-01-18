/* WiFi Credentials */
const char* ssid = "SSID";
const char* password = "PASSWORD";

/* Refresh Rate in seconds */
#define REFRESH_RATE_S (300)

/* Inverted colors */
#define INVERTED 0

/* SLEEP MODE */
#define SLEEP_MODE 0

/* Time Zone Offset */
#define TIME_ZONE_OFFSET (+1)

#if (INVERTED)
  #define TEXT_COLOR GxEPD_WHITE
  #define BACKGROUND_COLOR GxEPD_BLACK
#else
  #define TEXT_COLOR GxEPD_BLACK
  #define BACKGROUND_COLOR GxEPD_WHITE
#endif