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
#include "jura_regular8pt7b.h"
#include "jura_bold12pt7b.h"
#include "jura_bold30pt7b.h"
#include "wifi_images.h"

/* 2.9'' EPD Module */
GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display(GxEPD2_290_BS(/*CS=D1*/ 3, /*DC=D3*/ 5, /*RES=D0*/ 2, /*BUSY=D5*/ 7));  // DEPG0290BS 128x296, SSD1680

static const int button = 9;
static InputDebounce buttonDebounce; 

const int httpsPort = 443;
const String url = "http://api.coindesk.com/v1/bpi/currentprice/BTC.json";
const String historyURL = "http://api.coindesk.com/v1/bpi/historical/close.json";
const String cryptoCode = "BTC";

double btcusd;
double percentChange;

double pricehistory[32];
uint8_t historylength;
double history_min, history_max;

WiFiClient client;
HTTPClient http;

String formattedDate;
String dayStamp;
String timeStamp;
String lastUpdated;

typedef enum  {
  WIFI_CONFIGURE,
  WIFI_CONNECTING,
  WIFI_CONNECTED
} state_t;

state_t state;
WiFiManager wm;

void setup() 
{
  Serial.begin(115200);
  SPIFFS.begin(true);
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
      wm.setConfigPortalTimeout(60);
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
        state = WIFI_CONNECTED;
      }
      break;

    case WIFI_CONNECTED:
      if (WiFi.status() == WL_CONNECTED)
      {
        getCurrentBitcoinPrice();
        getBitcoinHistory();
        updateDisplay();
        Serial.println();

        for (int i = 0; i < (REFRESH_RATE_S * 100); i++)
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
  Serial.println(url);

  http.begin(url);
  int httpCode = http.GET();
  StaticJsonDocument<2000> doc;
  DeserializationError error = deserializeJson(doc, http.getString());

  if (error) 
  {
    Serial.print(F("deserializeJson Failed"));
    Serial.println(error.f_str());
    delay(2500);
    return;
  }

  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);

  String BTCUSDPrice = doc["bpi"]["USD"]["rate_float"].as<String>();
  lastUpdated = doc["time"]["updated"].as<String>();
  http.end();

  btcusd = BTCUSDPrice.toDouble();

  Serial.print("BTCUSD Price: ");
  Serial.println(BTCUSDPrice.toDouble());
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
    Serial.print(F("deserializeJson(History) failed"));
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
    display.fillScreen(BACKGROUND_COLOR);
    display.setTextColor(TEXT_COLOR);

    /* Draw current Bitcoin value */
    display.setFont(&Jura_Bold30pt7b);
    str = String(btcusd, 0);
    //str = 999999;
    display.getTextBounds(str, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = ((display.width() - tbw) / 2) - tbx + 30;
    y = y_offset + 24;
    display.setCursor(x, y);
    display.print(str);

    /* Draw BTC/USD symbols */
    display.setFont(&Jura_Bold12pt7b);
    display.setCursor(10, y_offset + 0);
    display.print("BTC");
    display.setCursor(8, y_offset + 26);
    display.print("USD");
    display.fillRoundRect(7, y_offset + 5, 52, 3, 1, TEXT_COLOR);

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
      display.drawPixel(graph_x + 2 * i, graph_y, TEXT_COLOR);
      display.drawPixel(graph_x + 2 * i, graph_y + graph_h, TEXT_COLOR);
    }

    for (int i = 0; i <= (graph_h / 2); i++) 
    {
      display.drawPixel(graph_x, graph_y + 2 * i, TEXT_COLOR);
      display.drawPixel(graph_x + graph_w, graph_y + 2 * i, TEXT_COLOR);
    }

    /* Graph line */
    for (int i = 0; i < (historylength - 1); i++) 
    {
      x0 = (uint16_t)(graph_x + 1 + i * graph_step);
      x1 = (uint16_t)(graph_x + 1 + (i + 1) * graph_step);
      y0 = (uint16_t)(graph_y + graph_h - 1 - ((pricehistory[i] - history_min) * graph_delta));
      y1 = (uint16_t)(graph_y + graph_h - 1 - ((pricehistory[i + 1] - history_min) * graph_delta));

      display.drawLine(x0, y0, x1, y1, TEXT_COLOR);
      display.drawLine(x0, y0 - 1, x1, y1 - 1, TEXT_COLOR);
      display.drawLine(x0, y0 + 1, x1, y1 + 1, TEXT_COLOR);
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
    display.fillScreen(BACKGROUND_COLOR);
    display.setTextColor(TEXT_COLOR);
    display.drawInvertedBitmap((296-64)/2, (128-64)/2 - 20, epd_bitmap_allArray[1], 64, 64, TEXT_COLOR);

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

void buttonTest_pressedCallback(uint8_t pinIn)
{
  /* Handle pressed state */
  //Serial.print("HIGH (pin: ");
  //Serial.print(pinIn);
  //Serial.println(")");
}

void buttonTest_releasedCallback(uint8_t pinIn)
{
  /* Handle released state */
  //Serial.print("LOW (pin: ");
  //Serial.print(pinIn);
  //Serial.println(")");
}

void buttonTest_pressedDurationCallback(uint8_t pinIn, unsigned long duration)
{
  /* Handle still pressed state */
  //Serial.print("HIGH (pin: ");
  //Serial.print(pinIn);
  //Serial.print(") still pressed, duration ");
  //Serial.print(duration);
  //Serial.println("ms");

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
  //Serial.print("LOW (pin: ");
  //Serial.print(pinIn);
  //Serial.print("), duration ");
  //Serial.print(duration);
}
