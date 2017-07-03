
### TFT library for ESP32

---


#### Features

* Full support for **ILI9341** & **ILI9488** based TFT modules in 4-wire SPI mode. Support for other controllers will be added later
* **18-bit (RGB)** color mode used
* **SPI displays oriented SPI driver library** based on *spi-master* driver
* Combined **DMA SPI** transfer mode and **direct SPI** for maximal speed
* **Grayscale mode** can be selected during runtime which converts all colors to gray scale
* SPI speeds up to **40 MHz** are tested and works without problems
* **Demo application** included which demonstrates most of the library features


* **Graphics drawing functions**:
  * **TFT_drawPixel**  Draw pixel at given x,y coordinates
  * **TFT_drawLine**  Draw line between two points
  * **TFT_drawFastVLine**, **TFT_drawFastHLine**  Draw vertical or horizontal line of given lenght
  * **TFT_drawLineByAngle**  Draw line on screen from (x,y) point at given angle
  * **TFT_drawRect**, **TFT_fillRect**  Draw rectangle on screen or fill given rectangular screen region with color
  * **TFT_drawRoundRect**, **TFT_fillRoundRect**  Draw rectangle on screen or fill given rectangular screen region with color with rounded corners
  * **TFT_drawCircle**, **TFT_fillCircle**  Draw or fill circle on screen
  * **TFT_drawEllipse**, **TFT_fillEllipse**  Draw or fill ellipse on screen
  * **TFT_drawTriangel**, **TFT_fillTriangle**  Draw or fill triangle on screen
  * **TFT_drawArc**  Draw circle arc on screen, from ~ to given angles, with given thickness. Can be outlined with different color
  * **TFT_drawPolygon**  Draw poligon on screen with given number of sides (3~60). Can be outlined with different color and rotated by given angle.
* **Fonts**:
  * **fixed** width and proportional fonts are supported; 8 fonts embeded
  * unlimited number of **fonts from file**
  * **7-segment vector font** with variable width/height is included (only numbers and few characters)
  * Proportional fonts can be used in fixed width mode.
  * Related functions:
    * **TFT_setFont**  Set current font from one of embeded fonts or font file
    * **TFT_getfontsize**  Returns current font height & width in pixels.
    * **TFT_getfontheight**  Returns current font height in pixels.
    * **set_7seg_font_atrib**  Set atributes for 7 segment vector font
    * **getFontCharacters**  Get all font's characters to buffer
* **String write function**:
  * **TFT_print**  Write text to display.
    * Strings can be printed at **any angle**. Rotation of the displayed text depends on *font_ratate* variable (0~360)
    * if *font_transparent* variable is set to 1, no background pixels will be printed
    * If the text does not fit the screen/window width it will be clipped ( if *text_wrap=0* ), or continued on next line ( if *text_wrap=1* )
    * Two special characters are allowed in strings: *\r* CR (0x0D), clears the display to EOL, *\n* LF (ox0A), continues to the new line, x=0
    * Special values can be entered for X position:
      * *CENTER*  centers the text
      * *RIGHT*   right justifies the text horizontaly
      * *LASTX*   continues from last X position; offset can be used: *LASTX+n*
    * Special values can be entered for Y:
      * *CENTER*  centers the text verticaly
      * *BOTTOM*  bottom justifies the text
      * *LASTY*   continues from last Y position; offset can be used: *LASTY+n*
  * **TFT_getStringWidth** Returns the string width in pixels based on current font characteristics. Useful for positioning strings on the screen.
  * **TFT_clearStringRect** Fills the rectangle occupied by string with current background color
* **Images**:
  * **TFT_jpg_image**  Decodes and displays JPG images
    * Limits:
      * Baseline only. Progressive and Lossless JPEG format are not supported.
      * Image size: Up to 65520 x 65520 pixels
      * Color space: YCbCr three components only. Gray scale image is not supported.
      * Sampling factor: 4:4:4, 4:2:2 or 4:2:0.
    * Can display the image **from file** or **memory buffer**
    * Image can be **scaled** by factor 0 ~ 3  (1/1, 1/2, 1/4 or 1/8)
    * Image is displayed from X,Y position on screen/window:
      * X: image left position; constants CENTER & RIGHT can be used; *negative* value is accepted
      * Y: image top position;  constants CENTER & BOTTOM can be used; *negative* value is accepted
  * **TFT_bmp_image**  Decodes and displays BMP images
    * Only uncompressed RGB 24-bit with no color space information BMP images can be displayed
    * Can display the image **from file** or **memory buffer**
    * Image can be **scaled** by factor 0 ~ 7; if scale>0, image is scaled by factor 1/(scale+1)
    * Image is displayed from X,Y position on screen/window:
      * X: image left position; constants CENTER & RIGHT can be used; *negative* value is accepted
      * Y: image top position;  constants CENTER & BOTTOM can be used; *negative* value is accepted
* **Window functions**:
  * Drawing on screen can be limited to rectangular *window*, smaller than the full display dimensions
  * When defined, all graphics, text and image coordinates are translated to *window* coordinates
  * Related functions
    * **TFT_setclipwin**  Sets the *window* area coordinates
    * **TFT_resetclipwin**  Reset the *window* to full screen dimensions
    * **TFT_saveClipWin**  Save current *window* to temporary variable
    * **TFT_restoreClipWin**  Restore current *window* from temporary variable
    * **TFT_fillWindow**  Fill *window* area with color
* **Touch screen** supported (for now only **XPT2046** controllers)
  * **TFT_read_touch**  Detect if touched and return X,Y coordinates. **Raw** touch screen or **calibrated** values can be returned.
    * calibrated coordinates are adjusted for screen orientation.
* **Read from display memory** supported
  * **TFT_readPixel**  Read pixel color value from display GRAM at given x,y coordinates.
  * **TFT_readData**  Read color data from rectangular screen area
* **Other display functions**:
  * **TFT_fillScreen**  Fill the whole screen with color
  * **TFT_setRotation**  Set screen rotation; PORTRAIT, PORTRAIT_FLIP, LANDSCAPE and LANDSCAPE_FLIP are supported
  * **TFT_invertDisplay**  Set inverted/normal colors
  * **TFT_compare_colors**  Compare two color structures
  * **disp_select()**  Activate display's CS line
  * **disp_deselect()**  Deactivate display's CS line
  * **find_rd_speed()**  Find maximum spi clock for successful read from display RAM
  * **TFT_display_init()**  Perform display initialization sequence. Sets orientation to landscape; clears the screen. SPI interface must already be setup, *tft_disp_type*, *_width*, *_height* variables must be set.
  * **HSBtoRGB**  Converts the components of a color, as specified by the HSB model to an equivalent set of values for the default RGB model.
  * **TFT_setGammaCurve()** Select one of 4 Gamma curves
* **compile_font_file**  Function which compiles font c source file to font file which can be used in *TFT_setFont()* function to select external font. Created file have the same name as source file and extension *.fnt*


* **Global wariables**
  * **orientation**  current screen orientation
  * **font_ratate**  current font rotate angle (0~395)
  * **font_transparent**  if not 0 draw fonts transparent
  * **font_forceFixed**  if not zero force drawing proportional fonts with fixed width
  * **text_wrap**  if not 0 wrap long text to the new line, else clip
  * **_fg**  current foreground color for fonts
  * **_bg**  current background for non transparent fonts
  * **dispWin** current display clip window
  * **_angleOffset**  angle offset for arc, polygon and line by angle functions
  * **image_debug**  print debug messages during image decode if set to 1
  * **cfont**  Currently used font structure
  * **TFT_X**  X position of the next character after TFT_print() function
  * **TFT_Y**  Y position of the next character after TFT_print() function
  * **tp_calx**  touch screen X calibration constant
  * **tp_caly**  touch screen Y calibration constant
  * **gray_scale**  convert all colors to gray scale if set to 1
  * **max_rdclock**  current spi clock for reading from display RAM
  * **_width** screen width (smaller dimension) in pixels
  * **_height** screen height (larger dimension) in pixels
  * **tft_disp_type**  current display type (DISP_TYPE_ILI9488 or DISP_TYPE_ILI9341)

---

Full functions **syntax and descriptions** can be found in *tft.h* and *tftspi.h* files.

Full **demo application**, well documented, is included, please **analyze it** to learn how to use the library functions.

---

#### Connecting the display

To run the demo, attach ILI9341 or ILI9488 based display module to ESP32. Default pins used are:
* mosi: 23
* miso: 19
*  sck: 18
*   CS:  5 (display CS)
*   DC: 26 (display DC)
*  TCS: 25 (touch screen CS)

---

*To run the demo on* **ESP-WROWER-KIT** *select the following pin configuration:*
*  mosi: 23
*  miso: 25
*   sck: 19
*    CS: 22 (display CS)
*    DC: 21 (display DC)
*   TCS:  0 (touch screen CS), not used
*   RST: 18 (display RESET)
* BKLIT:  5 (Display Back light)

Also set **TFT_RGB_BGR** to 0x00 and **TFT_INVERT_ROTATION** to 1 in *tftspi.h*

**You can also select EXAMPLE_ESP_WROVER_KIT in menuconfig to automaticaly define correct configuration**

---

**If you want to use different pins, change them in** *tftspi.h*

**if you want to use the touch screen functions, set** `#define USE_TOUCH 1` in *tftspi.h*

Using *make menuconfig* **select tick rate 1000** ( → Component config → FreeRTOS → Tick rate (Hz) ) to get more accurate timings

---

#### How to build

Configure your esp32 build environment as for **esp-idf examples**

Clone the repository

`git clone https://github.com/loboris/ESP32_TFT_library.git`

Execute menuconfig and configure your Serial flash config and other settings. Included *sdkconfig.defaults* sets some defaults to be used.

Navigate to **TFT Display DEMO Configuration** and set **SPIFFS** options.

Select if you want to use **wifi** (recommended) to get the time from **NTP** server and set your WiFi SSID and password.

`make menuconfig`

Make and flash the example.

`make all && make flash`

---

#### Prepare **SPIFFS** image

*The demo uses some image and font files and it is necessary to flash the spiffs image*

**To flash already prepared image to flash** execute:

`make copyfs`

---

You can also prepare different SFPIFFS **image** and flash it to ESP32. *This feature is only tested on Linux.*

Files to be included on spiffs are already in **components/spiffs_image/image/** directory. You can add or remove the files you want to include.

Then execute:

`make makefs`

to create **spiffs image** in *build* directory **without flashing** to ESP32

Or execute:

`make flashfs`

to create **spiffs image** in *build* directory and **flash** it to ESP32

---


---

**Example output:**

```

I (1133) cpu_start: Pro cpu up.
I (1145) cpu_start: Starting app cpu, entry point is 0x40080f5c
I (0) cpu_start: App cpu up.
I (1177) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (1200) heap_alloc_caps: At 3FFAE2A0 len 00001D60 (7 KiB): DRAM
I (1220) heap_alloc_caps: At 3FFBA708 len 000258F8 (150 KiB): DRAM
I (1241) heap_alloc_caps: At 3FFE0440 len 00003BC0 (14 KiB): D/IRAM
I (1263) heap_alloc_caps: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (1284) heap_alloc_caps: At 40095894 len 0000A76C (41 KiB): IRAM
I (1305) cpu_start: Pro cpu start user code
I (1359) cpu_start: Starting scheduler on PRO CPU.
I (196) cpu_start: Starting scheduler on APP CPU.

==============================
TFT display DEMO, LoBo 05/2017
==============================

SPI: display device added to spi bus
SPI: attached display device, speed=8000000
SPI: bus uses native pins: true
SPI: display init...
OK

---------------------
Graphics demo started
---------------------
I (1686) [TFT Demo]: Time is not set yet. Connecting to WiFi and getting time over NTP.
I (1726) wifi: wifi firmware version: 8a1ad86
I (1726) wifi: config NVS flash: enabled
I (1726) wifi: config nano formating: disabled
I (1726) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (1736) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (1776) wifi: Init dynamic tx buffer num: 32
I (1776) wifi: Init dynamic rx buffer num: 64
I (1776) wifi: wifi driver task: 3ffc1cd4, prio:23, stack:4096
I (1776) wifi: Init static rx buffer num: 10
I (1776) wifi: Init dynamic rx buffer num: 64
I (1786) wifi: Init rx ampdu len mblock:7
I (1786) wifi: Init lldesc rx ampdu entry mblock:4
I (1786) wifi: wifi power manager task: 0x3ffc709c prio: 21 stack: 2560
I (1796) [TFT Demo]: Setting WiFi configuration SSID LoBoInternet...
I (1806) wifi: wifi timer task: 3ffc811c, prio:22, stack:3584
I (1836) phy: phy_version: 354.0, b2dba61, May 16 2017, 21:41:06, 1, 0
I (1836) wifi: mode : sta (24:0a:c4:00:97:c0)
I (2556) wifi: n:6 0, o:1 0, ap:255 255, sta:6 0, prof:1
I (3546) wifi: state: init -> auth (b0)
I (3546) wifi: state: auth -> assoc (0)
I (3546) wifi: state: assoc -> run (10)
I (3586) wifi: connected with LoBoInternet, channel 6
I (5686) event: ip: 192.168.0.16, mask: 255.255.255.0, gw: 192.168.0.1
I (5686) [TFT Demo]: Initializing SNTP
I (6186) [TFT Demo]: System time is set.
I (6186) wifi: state: run -> init (0)
I (6186) wifi: n:6 0, o:6 0, ap:255 255, sta:6 0, prof:1
E (6186) wifi: esp_wifi_connect 816 wifi not start


I (9296) [SPIFFS]: Registering SPIFFS file system
I (9296) [SPIFFS]: Mounting SPIFFS files system
I (9296) [SPIFFS]: Start address: 0x180000; Size 1024 KB
I (9296) [SPIFFS]:   Work buffer: 2048 B
I (9306) [SPIFFS]:    FDS buffer: 384 B
I (9306) [SPIFFS]:    Cache size: 2048 B
I (9366) [SPIFFS]: Mounted

==========================================
Display: ILI9488: PORTRAIT 320,480 Color

     Clear screen time: 90 ms
Send color buffer time: 200 us (320 pixels)
       JPG Decode time: 420 ms
    BMP time, scale: 5: 530 ms
    BMP time, scale: 4: 540 ms
    BMP time, scale: 3: 550 ms
    BMP time, scale: 2: 550 ms
    BMP time, scale: 1: 560 ms
    BMP time, scale: 0: 550 ms

==========================================
Display: ILI9488: LANDSCAPE 480,320 Color

     Clear screen time: 80 ms
Send color buffer time: 300 us (480 pixels)
       JPG Decode time: 440 ms
    BMP time, scale: 5: 530 ms
    BMP time, scale: 4: 540 ms
    BMP time, scale: 3: 540 ms
    BMP time, scale: 2: 550 ms
    BMP time, scale: 1: 550 ms
    BMP time, scale: 0: 430 ms

==========================================
Display: ILI9488: PORTRAIT FLIP 320,480 Color

     Clear screen time: 90 ms
Send color buffer time: 200 us (320 pixels)
       JPG Decode time: 410 ms
    BMP time, scale: 5: 520 ms
    BMP time, scale: 4: 540 ms
    BMP time, scale: 3: 540 ms
    BMP time, scale: 2: 550 ms
    BMP time, scale: 1: 560 ms
    BMP time, scale: 0: 540 ms

==========================================
Display: ILI9488: LANDSCAPE FLIP 480,320 Color

     Clear screen time: 80 ms
Send color buffer time: 300 us (480 pixels)
       JPG Decode time: 440 ms
    BMP time, scale: 5: 530 ms
    BMP time, scale: 4: 530 ms
    BMP time, scale: 3: 550 ms
    BMP time, scale: 2: 550 ms
    BMP time, scale: 1: 560 ms
    BMP time, scale: 0: 430 ms

```
