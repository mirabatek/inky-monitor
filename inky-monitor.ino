#include "inky-monitor-config.h"
#include <WiFi.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <NTPClient.h> 
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <TimeLib.h>
#include "jura_regular8pt7b.h"
#include "jura_bold12pt7b.h"
#include "jura_bold30pt7b.h"
#include "wifi_off.h"

/* 2.9'' EPD Module */
GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display(GxEPD2_290_BS(/*CS=D1*/ 3, /*DC=D3*/ 5, /*RES=D0*/ 2, /*BUSY=D5*/ 7)); // DEPG0290BS 128x296, SSD1680

const int httpsPort = 443;
const String url_usd = "https://api.coinbase.com/v2/prices/BTC-USD/spot";
const String timeURL = "https://api.coinbase.com/v2/time";
const String basehistoryURL = "https://api.exchange.coinbase.com/products/BTC-USD/candles";
const String cryptoCode = "BTC";

double btcusd;
double percentChange;

double pricehistory[32][4];
uint8_t historylength;
double history_min, history_max;

WiFiClient client;
HTTPClient http;

String formattedDate;
String dayStamp;
String timeStamp;
String lastUpdated;
int64_t lastUpdatedTime;

void setup()
{
  int i = 0;
  Serial.begin(115200);

  display.init(115200, true, 50, false);

  WiFi.begin(ssid, password);

  Serial.println();
  Serial.print("Connecting to WiFi ");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    
    Serial.print(".");
    
    i = i + 1;
    
    if (i == 20) 
    {
      Serial.println();
      displayError();
    }

    if (i == 40)
    {
      Serial.println();
      i = 21;
    }
  }
  Serial.println();

  Serial.print("Connected to: ");
  Serial.print(ssid);
  Serial.println();
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    getTime();
    getCurrentBitcoinPrice();
    getCurrentBitcoinCandle();
    getBitcoinHistory();
    updateDisplay();
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }

  Serial.println();

#if (SLEEP_MODE)
  esp_sleep_enable_timer_wakeup(REFRESH_RATE_S * 1000000);
  esp_deep_sleep_start();
#else
  delay(REFRESH_RATE_S * 1000);
#endif
}

void getCurrentBitcoinPrice()
{
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
  lastUpdatedTime = doc["data"]["epoch"].as<int>();
  http.end();

  tmElements_t tm;
  parseISO8601(lastUpdated, tm);
  time_t utcTime = makeTime(tm);
  time_t localTime = applyTimezoneOffset(utcTime, TIME_ZONE_OFFSET);
  createTimeString(localTime, lastUpdated);
}

void getCurrentBitcoinCandle()
{
  String currentCandleURL;
  
  int64_t start = (lastUpdatedTime / 86400) * 86400; /* start of the day */
  int64_t end = lastUpdatedTime; /* now */
  
  /* Construct the Coinbase API URL */
  currentCandleURL = basehistoryURL + "?start=" + String(start) + "&end=" + String(end) + "&granularity=86400";
  
  Serial.print("Getting currentl candle ... ");
  Serial.println(currentCandleURL);

  http.begin(currentCandleURL);
  int httpCode = http.GET();
  StaticJsonDocument<1000> doc;
  DeserializationError error = deserializeJson(doc, http.getString());

  if (error)
  {
    Serial.print(F("deserializeJson(History) Failed "));
    Serial.println(error.f_str());
    delay(2500);
    return;
  }

  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);

  pricehistory[30][0] = doc[0][1].as<double>(); /* Low */
  pricehistory[30][1] = doc[0][2].as<double>(); /* High */
  pricehistory[30][2] = doc[0][3].as<double>(); /* Open */
  pricehistory[30][3] = doc[0][4].as<double>(); /* Close */
    
  http.end();

  Serial.print("BTCUSD Price Current: ");
  Serial.print(pricehistory[30][3]);
  Serial.println();
}

void getBitcoinHistory()
{
  String historyURL;
  
  int64_t start = lastUpdatedTime - (31 * 24 * 60 * 60); /* 30 days ago */
  int64_t end = lastUpdatedTime - (1 * 24 * 60 * 60); /* 1 day ago */
  
  /* Construct the Coinbase API URL */
  historyURL = basehistoryURL + "?start=" + String(start) + "&end=" + String(end) + "&granularity=86400";
  
  Serial.print("Getting history... ");
  Serial.println(historyURL);

  http.begin(historyURL);
  int httpCode = http.GET();
  StaticJsonDocument<5000> doc;
  DeserializationError error = deserializeJson(doc, http.getString());

  if (error)
  {
    Serial.print(F("deserializeJson(History) Failed "));
    Serial.println(error.f_str());
    delay(2500);
    return;
  }

  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);

  /* Coinbase returns an array of arrays [timestamp, low, high, open, close, volume] */
  historylength = 30;

  for (JsonArray candle : doc.as<JsonArray>()) 
  {
    historylength = historylength - 1;

    pricehistory[historylength][0] = candle[1].as<double>(); /* Low */
    pricehistory[historylength][1] = candle[2].as<double>(); /* High */
    pricehistory[historylength][2] = candle[3].as<double>(); /* Open */
    pricehistory[historylength][3] = candle[4].as<double>(); /* Close */
  }
    
  http.end();

  historylength = 31;

  history_min = 999999;
  history_max = 0;

  /* Find min and max price */
  if (GRAPH_MODE == 1)
  {
    for (int i = 0; i < historylength ; i++)
    {
      if (pricehistory[i][1] > history_max) history_max = pricehistory[i][1];
      if (pricehistory[i][0] < history_min) history_min = pricehistory[i][0];
    }
  }
  else
  {
    for (int i = 0; i < historylength; i++)
    {
      if (pricehistory[i][3] > history_max) history_max = pricehistory[i][3];
      if (pricehistory[i][3] < history_min) history_min = pricehistory[i][3];
    }
  }

  Serial.print("BTCUSD Price History: ");
  for (int i = 0; i < historylength; i++)
  {
    Serial.print(pricehistory[i][3]);
    Serial.print(", ");
  }
  Serial.println();
}

void updateDisplay()
{
  int16_t tbx, tby; 
  uint16_t tbw, tbh;
  uint16_t x, y, h;
  String str;

  uint16_t y_offset = 43;

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
    uint16_t graph_y = 80;
    uint16_t graph_w = 210;
    uint16_t graph_h = 38;
    uint16_t x0, x1, y0, y1;
    double graph_step, graph_delta;

    graph_step = ((double)graph_w - 2) / (double)(historylength - 1);
    graph_delta = ((double)graph_h - 2) / (history_max - history_min);

    /* 30-days maximum */
    str = String(history_max, 0);
    display.getTextBounds(str, 0, 0, &tbx, &tby, &tbw, &tbh);
    display.setCursor(graph_x - tbw - 10, graph_y + 9);
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

    if (GRAPH_MODE == 1)
    {
      /* Candle graph */
      graph_step = ((double)graph_w - 2) / (double)(historylength);
      graph_delta = ((double)graph_h - 2) / (history_max - history_min);

      for (int i = 0; i < (historylength); i++)
      {
        x0 = (uint16_t)(graph_x + graph_step - 2 + i * graph_step);
        x1 = (uint16_t)(graph_x + graph_step - 2 + i * graph_step);
        y0 = (uint16_t)(graph_y + graph_h - 1 - ((pricehistory[i][0] - history_min) * graph_delta));
        y1 = (uint16_t)(graph_y + graph_h - 1 - ((pricehistory[i][1] - history_min) * graph_delta));
        display.drawLine(x0, y0, x1, y1, TEXT_COLOR);

        if (pricehistory[i][3] > pricehistory[i][2])
        {
          /* Close > Open */
          y0 = (uint16_t)(graph_y + graph_h - 1 - ((pricehistory[i][3] - history_min) * graph_delta));
          h = (uint16_t)((pricehistory[i][3] - pricehistory[i][2]) * graph_delta);
          display.drawRect(x0 - 2, y0, 5, h, TEXT_COLOR);
          if (h > 2)
          {
            display.fillRect(x0 - 1, y0 + 1 , 3, h - 2, BACKGROUND_COLOR);
          }
        }
        else
        {
          /* Open > Close */
          y0 = (uint16_t)(graph_y + graph_h - 1 - ((pricehistory[i][2] - history_min) * graph_delta));
          h = (uint16_t)((pricehistory[i][2] - pricehistory[i][3]) * graph_delta);
          display.fillRect(x0 - 2, y0, 5, h, TEXT_COLOR);
        }
      }
    }
    else
    {
      /* Graph line */
      graph_step = ((double)graph_w - 2) / (double)(historylength - 1);
      graph_delta = ((double)graph_h - 2) / (history_max - history_min);

      for (int i = 0; i < (historylength - 1); i++)
      {
        x0 = (uint16_t)(graph_x + 1 + i * graph_step);
        x1 = (uint16_t)(graph_x + 1 + (i + 1) * graph_step);
        y0 = (uint16_t)(graph_y + graph_h - 1 - ((pricehistory[i][3] - history_min) * graph_delta));
        y1 = (uint16_t)(graph_y + graph_h - 1 - ((pricehistory[i + 1][3] - history_min) * graph_delta));

        display.drawLine(x0, y0, x1, y1, TEXT_COLOR);
        display.drawLine(x0, y0 - 1, x1, y1 - 1, TEXT_COLOR);
        display.drawLine(x0, y0 + 1, x1, y1 + 1, TEXT_COLOR);
      }
    }
  }
  while (display.nextPage());
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

void createTimeString(const time_t time, String &timeString)
{
  char buffer[20];
  sprintf(buffer, "%d-%02d-%02d %02d:%02d:%02d", year(time), month(time), day(time), hour(time), minute(time), second(time));
  timeString = String(buffer);
}

void displayError()
{
  int16_t tbx, tby; 
  uint16_t tbw, tbh;
  uint16_t x, y;

  String error1 = "WiFi not available";
  String error2 = "or wrong credentials.";
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  
  do
  {
    display.fillScreen(BACKGROUND_COLOR);
    display.setTextColor(TEXT_COLOR);
    display.drawInvertedBitmap((296 - 64) / 2, (128 - 64) / 2 - 20, epd_bitmap_allArray[0], 64, 64, TEXT_COLOR);

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
