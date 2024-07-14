# Inky Monitor - Bitcoin Price Ticker

## Overview:
A Bitcoin price ticker that runs on an ESP32-C3 microcontroller connected to a 2.9" monochrome e-ink display. The device connects to WiFi and periodically retrieves data from CoinDesk. The current Bitcoin price in USD and a 30-day history chart are rendered on the display. You can specify the refresh rate and color scheme (standard or inverted).

![Inky Monitor](/images/Refresh.gif)

The device is powered by a 5V power supply through a USB-C connector.

The device has a width of 105mm, a depth of 60mm, and a height of 68mm.

There are two options for the case smooth one (Inky_Monitor_Body.stl) and one with embossed bitcoin logo (Inky_Monitor_Body_Bitcoin.stl).

![Inky Monitor](/images/IMG_7270.JPG)

## Bill of Material:

- [XIAO ESP32-C3 Module](https://s.click.aliexpress.com/e/_DdLF88J)
- [WeAct 2.9'' E-Ink Black-White Display](https://s.click.aliexpress.com/e/_DdcPZuF)
- 4x 2.6x10mm Screw ([Set of Screws](https://s.click.aliexpress.com/e/_Dlb471V))
- 3x 2.6x6mm Screw ([Set of Screws](https://s.click.aliexpress.com/e/_Dlb471V))
- 2x [7-Pin 90° Header](https://s.click.aliexpress.com/e/_DBxgWcf) (optional)

## Case:
See my 3D model on [Printables](https://www.printables.com/@mirabatek/models).

## Assembly Instruction:
1. Clean the printed parts.
2. Insert the display into the body and secure it using Crossbar1 and a 6mm screw.
3. Solder 90-degree headers to the XIAO-ESP32-C3 development board. This step is optional, you can also solder the wires directly to the board.
4. Connect the display to the ESP32-C3 board according to following wiring diagram.
5. Connect the WiFi antenna.
6. Place the ESP32-C3 board into the body and secure it using Crossbar2 and two 6mm screws.
7. Arrange the wires and antenna, close the box, and secure the bottom part with four 10mm screws.

![Wiring](/images/Inky_Monitor_Schematic.png)

## Programming:
### Option 1 - Build from source code with hard coded settings:
1. Download the source code or clone it from GitHub (main branche)
2. Install **Arduino IDE**
3. Open the project in Arduino IDE
4. Install **esp32** by Espressif Systems in Boards Manager
5. Install **NTPClient** by Fabrice Weinber in Library Manager
6. Install **ArduinoJson** by Benoit Blanchon in Library Manager
7. Install **GxEPD2** Library by Jean-Marc Zingg (with dependencies **Adafruit BusIO** Library, **Adafruit GFX** Library) in Library Manager
8. Select **XIAO_ESP32-C3** board
9. Update WiFi credentials (ssid, password), refresh rate, inverted mode in inky-monitor-config.h file
10. Click on Verify button
11. Connect the ESP32-C3 over the USB cable to the PC
12. Click on Upload button
13. Wait until all data are written
14. Device should be now properly configured and display actual data

### Option 2 - Prebuild binary with configuration portal:
1. Download compiled firmware from GitHub Releases section
2. Open ESP Tool - a serial flasher utility
3. Connect the ESP32C3 over the USB cable to the PC
4. Click on Connect button

![Connect](/images/esp_tool_connect.png) 

5. Change Flash Address to 0x0
6. Select the binary file (inky-monitor.ino.merged.bin)
7. Click on Program button

![Program](/images/esp_tool_program.png) 

8. Wait until all data are written

![Programming done](/images/esp_tool_done.png)

9. Disconnect and connect the ESP32-C3 again
10. WiFi configuration portal is now active. Connect to Inky Monitor WiFi network from your mobile or PC.
11. Configure your WiFi connection and settings parameters

![Configuration](/images/IMG_7285_86.png) 

12. Device should be now properly configured and display actual data
13. If connection to WiFi is not successful the WiFi configuration portal is automaticaly activated
14. Configuration erase can be forced by pressing and holding the BOOT button for 5 seconds. Use the pin to press the button. From the front view, the BOOT button is on the left side.

## Debugging:
If something is not working as expected you can see some details what is going on in the device by connecting to serial port using a serial terminal. The baud rate is 115200.

![Serial Monitor](/images/Serial_Monitor.png) 
