# Inky Monitor - Bitcoin Price Ticker

## Overview:

Bitcoin price ticker that runs on ESP32-C3 microcontroller connected to a 2.9" monochrome e-ink display. The device is connected to WiFi and periodically takes data from CoinDesk. Present bitcoin price in USD and 30 days history chart are randered on the display. You can specify the refresh rate, color scheme (standard or inverted).

The device is power by 5V power supply through USB-C connector.

![Inky Monitor](/images/IMG_7124.JPG)

![Inky Monitor](/images/IMG_7119.JPG)

## Programming:

1. Install Arduino IDE
2. Install esp32 by Espressif Systems in Boards Manager
3. Install NTPClient by Fabrice Weinber in Library Manager
4. Install ArduinoJson by Benoit Blanchon in Library Manager
5. Install GxEPD2 Library by Jean-Marc Zingg (with dependencies Adafruit BusIO Library, Adafruit GFX Library) in Library Manager
6. Download the source code or clone it from GitHub
7. Update WiFi credentials
8. Select XIAO_ESP32C3 board
9. Verify 
10. Upload