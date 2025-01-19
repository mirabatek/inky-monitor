#include "inky-monitor-config.h"
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <InputDebounce.h>
#include <TimeLib.h>
#include "jura_regular8pt7b.h"
#include "jura_bold12pt7b.h"
#include "jura_bold30pt7b.h"
#include "wifi_images.h"

/* 2.9'' EPD Module */
GxEPD2_3C<GxEPD2_290_C90c, GxEPD2_290_C90c::HEIGHT> display(GxEPD2_290_C90c(/*CS=D1*/ 3, /*DC=D3*/ 5, /*RES=D0*/ 2, /*BUSY=D5*/ 7)); // DEPG0290BS 128x296, SSD1680

static const int button = 9;
static InputDebounce buttonDebounce; 

const int httpsPort = 443;
const String url_usd = "https://api.coinbase.com/v2/prices/BTC-USD/spot";
const String timeURL = "https://api.coinbase.com/v2/time";
const String historyURL = "https://api.coindesk.com/v1/bpi/historical/close.json";
const String cryptoCode = "BTC";

double btcusd;
String lastUpdated;

double pricehistory[32];
uint8_t historylength;
double history_min, history_max;

WiFiClient client;
HTTPClient http;

typedef enum  {
  WIFI_CONFIGURE,
  WIFI_CONNECTING,
  WIFI_CONNECTED
} state_t;

state_t state;
WiFiManager wm;

/* Config values */
char refresh_rate[5] = "300";
char inverted_mode[2] = "0";
char time_zone_offset[4] = "1";

int text_color = GxEPD_BLACK;
int background_color = GxEPD_WHITE;
int bitcoin_price_color = GxEPD_RED;
int refresh_rate_s = REFRESH_RATE_S_DEFAULT;
int time_zone_offset_s = TIME_ZONE_OFFSET_DEFAULT;

bool save_config = false;

WiFiManagerParameter custom_refresh_rate("custom_refresh_rate", "Refresh rate [30s-3600s]:", refresh_rate, 5);
WiFiManagerParameter custom_inverted_mode("custom_inverted_mode", "Inverted mode [0/1]:", inverted_mode, 2);
WiFiManagerParameter custom_time_zone_offset("custom_time_sone_offser", "Time Zone Offset:", time_zone_offset, 4);

void setup() 
{
  Serial.begin(115200);
  
  if (!SPIFFS.begin(true))
  {
    Serial.println("Failed to mount FS");
  }
  
  display.init(115200, true, 50, false);

  buttonDebounce.registerCallbacks(buttonTest_pressedCallback, buttonTest_releasedCallback, buttonTest_pressedDurationCallback, buttonTest_releasedDurationCallback);
  buttonDebounce.setup(button, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES, 5000);

  state = WIFI_CONFIGURE;
}

void loop() 
{
  bool res;
  unsigned long now;

  switch (state) 
  {
    case WIFI_CONFIGURE:
      readConfiguration();
      applyConfiguration();
      wm.setSaveConfigCallback(saveConfigCallback);
      wm.setConfigPortalTimeout(180);
      wm.addParameter(&custom_refresh_rate);
      wm.addParameter(&custom_inverted_mode);
      wm.addParameter(&custom_time_zone_offset);
      state = WIFI_CONNECTING;
      break;

    case WIFI_CONNECTING:
      displayInfo();

      res = wm.autoConnect("Inky Monitor");
      if (!res) 
      {
        Serial.println("Configuration failed");
      } 
      else 
      {
        Serial.println("Connected");
          
        strcpy(refresh_rate, custom_refresh_rate.getValue());
        strcpy(inverted_mode, custom_inverted_mode.getValue());
        strcpy(time_zone_offset, custom_time_zone_offset.getValue());
   
        if (save_config) 
        {
          applyConfiguration();
          writeConfiguration();
        }

        Serial.println("Configuration parameters:");
        Serial.println("  Refresh Rate: " + String(refresh_rate));
        Serial.println("  Inverted Mode: " + String(inverted_mode));
        Serial.println("  Time Zone Offset: " + String(time_zone_offset));

        state = WIFI_CONNECTED;
      }
      break;

    case WIFI_CONNECTED:
      if (WiFi.status() == WL_CONNECTED)
      {
        getCurrentBitcoinPrice();
        getBitcoinHistory();
        getTime();
        updateDisplay();
        Serial.println();

        for (int i = 0; i < (refresh_rate_s * 100); i++)
        {
          now = millis();
          buttonDebounce.process(now);
          delay(10);
        }
      }
      else
      {
        Serial.println("WiFi Disconnected. Restarting ...");
        ESP.restart();
      }
      break;

    default:
      break;
  }

  now = millis();
  buttonDebounce.process(now);
}

void getCurrentBitcoinPrice() 
{
  String str;

  Serial.print("Connecting to ");
  Serial.println(url_usd);

  http.begin(url_usd);
  int httpCode = http.GET();
  StaticJsonDocument<2000> doc;
  DeserializationError error = deserializeJson(doc, http.getString());

  if (error)
  {
    Serial.print(F("deserializeJson Failed "));
    Serial.println(error.f_str());
    delay(2500);
    return;
  }

  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);

  String BTCUSDPrice = doc["data"]["amount"].as<String>();
  http.end();

  btcusd = BTCUSDPrice.toDouble();

  Serial.print("BTCUSD Price: ");
  Serial.println(BTCUSDPrice.toDouble());
}

void getTime()
{
  String str;

  Serial.print("Connecting to ");
  Serial.println(timeURL);

  http.begin(timeURL);
  int httpCode = http.GET();
  StaticJsonDocument<2000> doc;
  DeserializationError error = deserializeJson(doc, http.getString());

  if (error)
  {
    Serial.print(F("deserializeJson Failed "));
    Serial.println(error.f_str());
    delay(2500);
    return;
  }

  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);

  lastUpdated = doc["data"]["iso"].as<String>();
  http.end();

  tmElements_t tm;
  parseISO8601(lastUpdated, tm);
  time_t utcTime = makeTime(tm);
  time_t localTime = applyTimezoneOffset(utcTime, time_zone_offset_s);
  createTimeString(localTime, lastUpdated);
}

void getBitcoinHistory() 
{
  String str;

  Serial.print("Getting history... ");
  Serial.println(historyURL);

  http.begin(historyURL);
  int httpCode = http.GET();
  StaticJsonDocument<2000> doc;
  DeserializationError error = deserializeJson(doc, http.getString());

  if (error) 
  {
    Serial.print(F("deserializeJson Failed "));
    Serial.println(error.f_str());
    delay(2500);
    return;
  }

  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);

  JsonObject bpi = doc["bpi"].as<JsonObject>();

  double price;

  historylength = 0;

  for (JsonPair kv : bpi) 
  {
    price = kv.value().as<double>();
    pricehistory[historylength] = price;
    historylength = historylength + 1;
  }
  http.end();

  pricehistory[historylength] = btcusd;
  historylength = historylength + 1;

  history_min = 999999;
  history_max = 0;

  for (int i = 0; i < historylength; i++) 
  {
    if (pricehistory[i] > history_max) history_max = pricehistory[i];
    if (pricehistory[i] < history_min) history_min = pricehistory[i];
  }

  Serial.print("BTCUSD Price History: ");
  for (int i = 0; i < historylength; i++) 
  {
    Serial.print(pricehistory[i]);
    Serial.print(", ");
  }
  Serial.println();
}

void updateDisplay() 
{
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  uint16_t x, y;
  String str;

  uint16_t y_offset = 47;

  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();

  do 
  {
    display.fillScreen(background_color);
    
    /* Draw current Bitcoin value */
    display.setTextColor(bitcoin_price_color);
    display.setFont(&Jura_Bold30pt7b);
    str = String(btcusd, 0);
    //str = 999999;
    display.getTextBounds(str, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = ((display.width() - tbw) / 2) - tbx + 30;
    y = y_offset + 24;
    display.setCursor(x, y);
    display.print(str);

    /* Draw BTC/USD symbols */
    display.setTextColor(text_color);
    display.setFont(&Jura_Bold12pt7b);
    display.setCursor(10, y_offset + 0);
    display.print("BTC");
    display.setCursor(8, y_offset + 26);
    display.print("USD");
    display.fillRoundRect(7, y_offset + 5, 52, 3, 1, text_color);

    /* Draw last update date and time */
    display.setFont(&Jura_Regular8pt7b);
    display.getTextBounds(lastUpdated, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = ((display.width() - tbw) / 2) - tbx;
    y = 17;
    display.setCursor(x, y);
    display.print(lastUpdated);

    /* Draw 30-days history chart */
    uint16_t graph_x = 72;
    uint16_t graph_y = 88;
    uint16_t graph_w = 205;
    uint16_t graph_h = 30;
    uint16_t x0, x1, y0, y1;

    double graph_step = ((double)graph_w - 2) / (double)(historylength - 1);
    double graph_delta = ((double)graph_h - 2) / (history_max - history_min);

    /* 30-days maximum */
    str = String(history_max, 0);
    display.getTextBounds(str, 0, 0, &tbx, &tby, &tbw, &tbh);
    display.setCursor(graph_x - tbw - 10, graph_y + 7);
    display.print(str);

    /* 30-days minimum */
    str = String(history_min, 0);
    display.getTextBounds(str, 0, 0, &tbx, &tby, &tbw, &tbh);
    display.setCursor(graph_x - tbw - 10, graph_y + graph_h);
    display.print(str);

    /* Doted border */
    for (int i = 0; i <= ((graph_w) / 2); i++) 
    {
      display.drawPixel(graph_x + 2 * i, graph_y, text_color);
      display.drawPixel(graph_x + 2 * i, graph_y + graph_h, text_color);
    }

    for (int i = 0; i <= (graph_h / 2); i++) 
    {
      display.drawPixel(graph_x, graph_y + 2 * i, text_color);
      display.drawPixel(graph_x + graph_w, graph_y + 2 * i, text_color);
    }

    /* Graph line */
    for (int i = 0; i < (historylength - 1); i++) 
    {
      x0 = (uint16_t)(graph_x + 1 + i * graph_step);
      x1 = (uint16_t)(graph_x + 1 + (i + 1) * graph_step);
      y0 = (uint16_t)(graph_y + graph_h - 1 - ((pricehistory[i] - history_min) * graph_delta));
      y1 = (uint16_t)(graph_y + graph_h - 1 - ((pricehistory[i + 1] - history_min) * graph_delta));

      display.drawLine(x0, y0, x1, y1, text_color);
      display.drawLine(x0, y0 - 1, x1, y1 - 1, text_color);
      display.drawLine(x0, y0 + 1, x1, y1 + 1, text_color);
    }

  } while (display.nextPage());
}

void displayInfo()
{
  int16_t tbx, tby; 
  uint16_t tbw, tbh;
  uint16_t x, y;

  String error1 = "Connect to Inky Monitor WiFi";
  String error2 = "and configure WiFi connection.";
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  
  do
  {
    display.fillScreen(background_color);
    display.setTextColor(text_color);
    display.drawInvertedBitmap((296-64)/2, (128-64)/2 - 20, epd_bitmap_allArray[1], 64, 64, text_color);

    display.setFont(&Jura_Regular8pt7b);
    display.getTextBounds(error1, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = ((display.width() - tbw) / 2) - tbx;
    y = 90;
    display.setCursor(x, y);
    display.print(error1);

    display.getTextBounds(error2, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = ((display.width() - tbw) / 2) - tbx;
    y = 110;
    display.setCursor(x, y);
    display.print(error2);
  }
  while (display.nextPage());
}

void saveConfigCallback() 
{
  save_config = true;
}

void readConfiguration()
{
  if (SPIFFS.exists("/config.json")) 
  {
    Serial.println("Reading config file");
    File configFile = SPIFFS.open("/config.json", "r");
    
    if (configFile) 
    {
      Serial.println("Open config file");
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      
      DynamicJsonDocument json(1024);
      auto deserializeError = deserializeJson(json, buf.get());
      serializeJson(json, Serial);
      Serial.println("");
      
      if (!deserializeError)
      {
          strcpy(refresh_rate, json["refresh_rate"]);
          strcpy(inverted_mode, json["inverted_mode"]);
          strcpy(time_zone_offset, json["time_zone_offset"]);
          custom_refresh_rate.setValue(refresh_rate, 5);
          custom_inverted_mode.setValue(inverted_mode, 2);
          custom_time_zone_offset.setValue(time_zone_offset, 4);
          applyConfiguration();
          
          Serial.println("Configuration loaded from json");
      }
      else
      {
        Serial.println("Failed to load configurtaion from json");
      }
      
      configFile.close();
    }   
  }
  else
  {
    Serial.println("Config file doesn't exist");
  } 
}

void writeConfiguration()
{
  DynamicJsonDocument json(1024);

  json["refresh_rate"] = refresh_rate;
  json["inverted_mode"] = inverted_mode;
  json["time_zone_offset"] = time_zone_offset;

  File configFile = SPIFFS.open("/config.json", "w");
  
  if (configFile) 
  {
      serializeJson(json, Serial);
      serializeJson(json, configFile);
      Serial.println("");
      Serial.println("Configuration saved to json");

      configFile.close();
  }
  else
  {
    Serial.println("Failed to open config file for writing configuration");
  }
}

void applyConfiguration()
{
  if (strcmp(inverted_mode, "1") == 0)
  {
    text_color = GxEPD_WHITE;
    background_color = GxEPD_BLACK;
    sprintf(inverted_mode, "%d", 1);
  }
  else
  {
    text_color = GxEPD_BLACK;
    background_color = GxEPD_WHITE;
    sprintf(inverted_mode, "%d", 0);
  }

  refresh_rate_s = atoi(refresh_rate);
  
  if (refresh_rate_s < REFRESH_RATE_S_MIN)
  {
    refresh_rate_s = REFRESH_RATE_S_DEFAULT;
    sprintf(refresh_rate, "%d", refresh_rate_s);
  }

  if (refresh_rate_s > REFRESH_RATE_S_MAX)
  {
    refresh_rate_s = REFRESH_RATE_S_DEFAULT;
    sprintf(refresh_rate, "%d", refresh_rate_s);
  }

  time_zone_offset_s = atoi(time_zone_offset);

    if (refresh_rate_s < REFRESH_RATE_S_MIN)
  {
    refresh_rate_s = REFRESH_RATE_S_DEFAULT;
    sprintf(refresh_rate, "%d", refresh_rate_s);
  }

  if (refresh_rate_s > REFRESH_RATE_S_MAX)
  {
    refresh_rate_s = REFRESH_RATE_S_DEFAULT;
    sprintf(refresh_rate, "%d", refresh_rate_s);
  }
}

/* Function to parse ISO 8601 date-time string (e.g., "2025-01-17T12:30:45Z") into a tmElements_t structure. */
void parseISO8601(const String &isoTime, tmElements_t &tm) 
{
  if (isoTime.length() < 19) return;

  tm.Year = CalendarYrToTm(isoTime.substring(0, 4).toInt());
  tm.Month = isoTime.substring(5, 7).toInt();
  tm.Day = isoTime.substring(8, 10).toInt();
  tm.Hour = isoTime.substring(11, 13).toInt();
  tm.Minute = isoTime.substring(14, 16).toInt();
  tm.Second = isoTime.substring(17, 19).toInt();
}

/* Function to apply a timezone offset (in hours) to the given time. */
time_t applyTimezoneOffset(const time_t utcTime, const int timezoneOffset) 
{
  return utcTime + timezoneOffset * SECS_PER_HOUR;
}

/* Convert time variable to string */
void createTimeString(const time_t time, String &timeString)
{
  char buffer[20];
  sprintf(buffer, "%d-%02d-%02d %02d:%02d:%02d", year(time), month(time), day(time), hour(time), minute(time), second(time));
  timeString = String(buffer);
}

void buttonTest_pressedCallback(uint8_t pinIn)
{
  /* Handle pressed state */
}

void buttonTest_releasedCallback(uint8_t pinIn)
{
  /* Handle released state */
}

void buttonTest_pressedDurationCallback(uint8_t pinIn, unsigned long duration)
{
  /* Handle still pressed state */

  if (duration >= 5000) 
  {
    Serial.println("Configuration erase and restart");
    wm.resetSettings();
    ESP.restart();
  }
}

void buttonTest_releasedDurationCallback(uint8_t pinIn, unsigned long duration)
{
  /* Handle released state */
}
