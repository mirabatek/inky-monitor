# Inky Monitor - Bitcoin Price Ticker

## Overview:
Bitcoin price ticker that runs on ESP32-C3 microcontroller connected to a 2.9" monochrome e-ink display. The device is connected to WiFi and periodically takes data from CoinDesk. Present bitcoin price in USD and 30 days history chart are rendered on the display. You can specify the refresh rate, color scheme (standard or inverted).

The device is power by 5V power supply through USB-C connector.

![Inky Monitor](/images/IMG_7124.JPG)

![Inky Monitor](/images/IMG_7119.JPG)

## Bill of Material:

- ![XIAO ESP32-C3 Module](https://s.click.aliexpress.com/e/_DdLF88J)
- ![WeAct 2.9'' E-Ink Black-White Display](https://s.click.aliexpress.com/e/_DdcPZuF)
- 4x 2.6x10mm Screw (![Set of Screws](https://s.click.aliexpress.com/e/_Dlb471V))
- 3x 2.6x6mm Screw (![Set of Screws](https://s.click.aliexpress.com/e/_Dlb471V))
- 2x ![7-Pin 90° Header](https://s.click.aliexpress.com/e/_DBxgWcf) (optional)

## Case
See my 3D model on ![Printables](https://www.printables.com/@mirabatek/models).

## Assembly Instruction:
1. Clean the prints.
2. Put the display into the body and fix it using the Crossbar1 and 6mm screw.
3. Solder 90 degree headers to the XIAO-ESP32-C3 development board. This is optional you can solder the wires directly to the board.
4. Connect the display to the ESP32-C3 board according to following wiring diagram.
5. Connect the WiFi antenna.
6. Put the ESP32-C3 board into the body and fix it using Crossbar2 and two 6mm screws.
7. Arrange the wires and antenna, close the box and fix the bottom part by four 10mm screws.

![Wiring](/images/Inky_Monitor_Schematic.png)

## Programming:
1. Download the source code or clone it from GitHub
2. Install Arduino IDE
3. Open the project in Arduino IDE
4. Install esp32 by Espressif Systems in Boards Manager
5. Install NTPClient by Fabrice Weinber in Library Manager
6. Install ArduinoJson by Benoit Blanchon in Library Manager
7. Install GxEPD2 Library by Jean-Marc Zingg (with dependencies Adafruit BusIO Library, Adafruit GFX Library) in Library Manager
8. Select XIAO_ESP32C3 board
9. Update WiFi credentials (ssid, password), refresh rate, inverted mode in inky-monitor-config.h file
10. Click on Verify
11. Connect the ESP32C3 over the USB cable to the PC
12. Click on Upload