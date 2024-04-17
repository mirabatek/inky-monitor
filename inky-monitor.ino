#include "inky-monitor-config.h"
#include <WiFi.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <NTPClient.h> 
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include "jura_regular8pt7b.h"
#include "jura_bold12pt7b.h"
#include "jura_bold30pt7b.h"

// 2.9'' EPD Module
GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display(GxEPD2_290_BS(/*CS=D1*/ 3, /*DC=D3*/ 5, /*RES=D0*/ 2, /*BUSY=D5*/ 7)); // DEPG0290BS 128x296, SSD1680

// Powered by CoinDesk - https://www.coindesk.com/price/bitcoin
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

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

String lastUpdated;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("Connected to: ");
  Serial.print(ssid);

  display.init(115200, true, 50, false);
}

void loop() {
  String str;

  Serial.print("Connecting to ");
  Serial.println(url);

  http.begin(url);
  int httpCode = http.GET();
  StaticJsonDocument<2000> doc;
  DeserializationError error = deserializeJson(doc, http.getString());

  if (error) {
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

  Serial.print("Getting history...");
  StaticJsonDocument<2000> historyDoc;
  http.begin(historyURL);
  int historyHttpCode = http.GET();
  DeserializationError historyError = deserializeJson(historyDoc, http.getString());

  if (historyError) {
    Serial.print(F("deserializeJson(History) failed"));
    Serial.println(historyError.f_str());
    delay(2500);
    return;
  }

  Serial.print("History HTTP Status Code: ");
  Serial.println(historyHttpCode);
  JsonObject bpi = historyDoc["bpi"].as<JsonObject>();
  double yesterdayPrice, price;

  historylength = 0;

  for (JsonPair kv : bpi) {
    price = kv.value().as<double>();
    pricehistory[historylength] = price;
    if (price > history_max) history_max = price;
    if (price < history_min) history_min = price;
    historylength = historylength + 1;
  }

  yesterdayPrice = pricehistory[historylength - 1];
  pricehistory[historylength] = btcusd;
  historylength = historylength + 1;

  history_min = 999999;
  history_max = 0;
  
  for (int i=0; i<historylength; i++)
  {
    if (pricehistory[i] > history_max) history_max = pricehistory[i];
    if (pricehistory[i] < history_min) history_min = pricehistory[i];
  }

  Serial.print("BTCUSD Price: ");
  Serial.println(BTCUSDPrice.toDouble());

  Serial.print("Yesterday's Price: ");
  Serial.println(yesterdayPrice);

  bool isUp = BTCUSDPrice.toDouble() > yesterdayPrice;
  
  if (isUp) {
    percentChange = ((BTCUSDPrice.toDouble() - yesterdayPrice) / yesterdayPrice) * 100;
  } else {
    percentChange = ((yesterdayPrice - BTCUSDPrice.toDouble()) / yesterdayPrice) * 100;
  }

  Serial.print("Percent Change: ");
  Serial.println(percentChange);

  http.end();

  updateValues();
  delay(REFRESH_RATE_S * 1000);
}

void updateValues()
{
  int16_t tbx, tby; uint16_t tbw, tbh;
  uint16_t x, y;
  String str;

  uint16_t y_offset = 45;

  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  
  do
  {
    display.fillScreen(BACKGROUND_COLOR);
    display.setTextColor(TEXT_COLOR);

    // Draw bitcoin value
    display.setFont(&Jura_Bold30pt7b);
    str = String(btcusd, 0);
    //str = 999999;
    display.getTextBounds(str, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = ((display.width() - tbw) / 2) - tbx + 30;
    y = y_offset + 24;
    display.setCursor(x, y);
    display.print(str);

    // Draw bitcoing logo
    //y = (int16_t)(((display.height() - 50) / 2));
    //display.drawInvertedBitmap(10, y, epd_bitmap_allArray[0], 33, 50, TEXT_COLOR);

    display.setFont(&Jura_Bold12pt7b);
    display.setCursor(8, y_offset + 0);
    display.print("BTC");
    display.setCursor(6, y_offset + 26);
    display.print("USD");
    //display.drawFastHLine(10, 70, 50, GxEPD_BLACK);
    display.fillRoundRect(5, y_offset + 5, 52, 3, 1, TEXT_COLOR);

    // Draw last update date and time
    display.setFont(&Jura_Regular8pt7b);
    display.getTextBounds(lastUpdated, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = ((display.width() - tbw) / 2) - tbx;
    y = 15;
    display.setCursor(x, y);
    display.print(lastUpdated);

    /*
    // Draw daily change
    str = String(percentChange, 2);
    str = str + "% (day)";
    if (percentChange > 0) str = "+" + str; 
    display.setCursor(10, 115);
    display.print(str);
    */

    str = String(history_max, 0);
    display.setCursor(15, 95);
    display.print(str);

    str = String(history_min, 0);
    display.setCursor(15, 120);
    display.print(str);

    uint16_t graph_x = 75;
    uint16_t graph_y = 90;
    uint16_t graph_w = 205;
    uint16_t graph_h = 30;
    double graph_step = (double)graph_w / (double)(historylength);
    double graph_delta = (history_max - history_min) / graph_h;
    uint16_t x0, x1, y0, y1;

    display.drawRect(graph_x, graph_y - 1, graph_w, graph_h + 1, TEXT_COLOR);

    for (int i=0; i<(historylength-1); i++)
    {
      x0 = (uint16_t)(graph_x + i * graph_step);
      x1 = (uint16_t)(graph_x + (i + 1) * graph_step);
      y0 = (uint16_t)(graph_y + graph_h - ((pricehistory[i] - history_min) / graph_delta));
      y1 = (uint16_t)(graph_y + graph_h - ((pricehistory[i+1] - history_min) / graph_delta));
   
      display.drawLine(x0, y0, x1, y1, TEXT_COLOR);
      display.drawLine(x0 + 1, y0, x1 + 1, y1, TEXT_COLOR);
      display.drawLine(x0 + 2, y0, x1 + 2, y1, TEXT_COLOR);
    }
  }
  while (display.nextPage());
}
