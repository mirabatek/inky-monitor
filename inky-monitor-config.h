/* WiFi Credentials */
const char* ssid = "SSID";
const char* password = "PASSWORD";

/* Number of seconds after reset during which a
   subseqent reset will be considered a double reset. */
#define DRD_TIMEOUT 10

/* RTC Memory Address for the DoubleResetDetector to use */
#define DRD_ADDRESS 0

/* Refresh Rate in seconds */
#define REFRESH_RATE_S (300)

/* Inverted colors */
#define INVERTED 1

/* SLEEP MODE */
#define SLEEP_MODE 0

#if (INVERTED)
  #define TEXT_COLOR GxEPD_WHITE
  #define BACKGROUND_COLOR GxEPD_BLACK
#else
  #define TEXT_COLOR GxEPD_BLACK
  #define BACKGROUND_COLOR GxEPD_WHITE
#endif

/* Button debounce delay in ms */
#define BUTTON_DEBOUNCE_DELAY 20