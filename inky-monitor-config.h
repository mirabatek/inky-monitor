// Base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// Enable or disable GxEPD2_GFX base class
//#define ENABLE_GxEPD2_GFX 0

// WiFi Credentials
const char* ssid = "SSID";
const char* password = "PASSWORD";

// Refresh Rate in seconds
#define REFRESH_RATE_S (300)

// Uncomment if you want inverted colors
//#define INVERTED

#if (defined (INVERTED))
  #define TEXT_COLOR GxEPD_WHITE
  #define BACKGROUND_COLOR GxEPD_BLACK
#else
  #define TEXT_COLOR GxEPD_BLACK
  #define BACKGROUND_COLOR GxEPD_WHITE
#endif