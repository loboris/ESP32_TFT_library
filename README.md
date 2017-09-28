
### TFT library for ESP32

---

**This library must be built with the latest esp-idf master branch and xtensa toolchain**

If you are using the esp-idf v2.1, checkout the commit *0518df81a6566820352dad7bf6c539995d41ad18*

---

#### Features

* Full support for **ILI9341**, **ILI9488**, **ST7789V** and **ST7735** based TFT modules in 4-wire SPI mode. Support for other controllers will be added later
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

| ESP32 pin | Display module | Notes |
| - | - | - |
| Any output pin | MOSI | SPI input on Display module |
| Any pin | MISO | SPI output from Display module, optional |
| Any output pin | SCK | SPI clock input on Display module |
| Any output pin | CS  | SPI CS input on Display module |
| Any output pin | DC  | DC (data/command) input on Display module |
| Any output pin | TCS  | Touch pannel CS input (if touch panel is used |
| Any output pin | RST  | **optional**, reset input of the display module, if not used **pullup the reset input** to Vcc |
| Any output pin | BL  | **optional**, backlight input of the display module, if not used connect to +3.3V (or +5V) |
| GND | GND  | Power supply ground |
| 3.3V or +5V | Vcc  | Power supply positive |

**Make shure the display module has 3.3V compatible interface, if not you must use level shifter!**

---

To run the demo, attach ILI9341, ILI9488 or ST7735 based display module to ESP32. Default pins used are:
* mosi: 23
* miso: 19
*  sck: 18
*   CS:  5 (display CS)
*   DC: 26 (display DC)
*  TCS: 25 (touch screen CS)

---

*To run the demo on* **ESP-WROWER-KIT v3** *select the following pin configuration:*
*  mosi: 23
*  miso: 25
*   sck: 19
*    CS: 22 (display CS)
*    DC: 21 (display DC)
*   TCS:  0 (touch screen CS), not used
*   RST: 18 (display RESET)
* BKLIT:  5 (Display Back light)

Also set **TFT_RGB_BGR** to 0x00 and **TFT_INVERT_ROTATION1** to 1 in *tftspi.h*

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

I (0) cpu_start: App cpu up.
I (312) heap_init: Initializing. RAM available for dynamic allocation:
I (319) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (325) heap_init: At 3FFBB0B8 len 00024F48 (147 KiB): DRAM
I (331) heap_init: At 3FFE0440 len 00003BC0 (14 KiB): D/IRAM
I (338) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (344) heap_init: At 40091F94 len 0000E06C (56 KiB): IRAM
I (350) cpu_start: Pro cpu start user code
I (144) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.

==============================
TFT display DEMO, LoBo 09/2017
==============================

SPI: display device added to spi bus (2)
SPI: attached display device, speed=8000000
SPI: bus uses native pins: false
SPI: display init...
OK
SPI: Changed speed to 26666666

---------------------
Graphics demo started
---------------------
I (2815) [TFT Demo]: Time is not set yet. Connecting to WiFi and getting time over NTP.
I (2845) wifi: wifi firmware version: ee52423
I (2846) wifi: config NVS flash: enabled
I (2846) wifi: config nano formating: disabled
I (2846) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (2856) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (2890) wifi: Init dynamic tx buffer num: 32
I (2890) wifi: Init data frame dynamic rx buffer num: 32
I (2890) wifi: Init management frame dynamic rx buffer num: 32
I (2894) wifi: wifi driver task: 3ffc83d8, prio:23, stack:4096
I (2899) wifi: Init static rx buffer num: 10
I (2903) wifi: Init dynamic rx buffer num: 32
I (2907) wifi: Init rx ampdu len mblock:7
I (2911) wifi: Init lldesc rx ampdu entry mblock:4
I (2916) wifi: wifi power manager task: 0x3ffcd844 prio: 21 stack: 2560
I (2922) [TFT Demo]: Setting WiFi configuration SSID LoBoInternet...
I (2951) phy: phy_version: 359.0, e79c19d, Aug 31 2017, 17:06:07, 0, 0
I (2951) wifi: mode : sta (24:0a:c4:11:a4:0c)
I (3073) wifi: n:11 0, o:1 0, ap:255 255, sta:11 0, prof:1
I (3731) wifi: state: init -> auth (b0)
I (3734) wifi: state: auth -> assoc (0)
I (3738) wifi: state: assoc -> run (10)
I (3776) wifi: connected with LoBoInternet, channel 11
I (5827) event: ip: 192.168.0.21, mask: 255.255.255.0, gw: 192.168.0.1
I (5828) [TFT Demo]: Initializing SNTP
I (6331) [TFT Demo]: System time is set.
I (6331) wifi: state: run -> init (0)
I (6332) wifi: n:11 0, o:11 0, ap:255 255, sta:11 0, prof:1
I (6344) wifi: flush txq
I (6344) wifi: stop sw txq
I (6344) wifi: lmac stop hw txq
E (6344) wifi: esp_wifi_connect 836 wifi not start


I (8441) [SPIFFS]: Registering SPIFFS file system
I (8441) [SPIFFS]: Mounting SPIFFS files system
I (8441) [SPIFFS]: Start address: 0x280000; Size 1024 KB
I (8447) [SPIFFS]:   Work buffer: 2048 B
I (8451) [SPIFFS]:    FDS buffer: 384 B
I (8456) [SPIFFS]:    Cache size: 2048 B
I (8500) [SPIFFS]: Mounted

==========================================
Display: ILI9488: PORTRAIT 240,320 Color

     Clear screen time: 60 ms
Send color buffer time: 228 us (240 pixels)
       JPG Decode time: 287 ms
    BMP time, scale: 5: 422 ms
    BMP time, scale: 4: 431 ms
    BMP time, scale: 3: 430 ms
    BMP time, scale: 2: 434 ms
    BMP time, scale: 1: 442 ms
    BMP time, scale: 0: 335 ms

==========================================
Display: ILI9488: LANDSCAPE 320,240 Color

     Clear screen time: 57 ms
Send color buffer time: 301 us (320 pixels)
I (126333) event: station ip lost
       JPG Decode time: 286 ms
    BMP time, scale: 5: 422 ms
    BMP time, scale: 4: 431 ms
    BMP time, scale: 3: 433 ms
    BMP time, scale: 2: 435 ms
    BMP time, scale: 1: 444 ms
    BMP time, scale: 0: 260 ms

==========================================
Display: ILI9488: PORTRAIT FLIP 240,320 Color

     Clear screen time: 60 ms
Send color buffer time: 228 us (240 pixels)
       JPG Decode time: 287 ms
    BMP time, scale: 5: 420 ms
    BMP time, scale: 4: 430 ms
    BMP time, scale: 3: 429 ms
    BMP time, scale: 2: 436 ms
    BMP time, scale: 1: 446 ms
    BMP time, scale: 0: 338 ms

==========================================
Display: ILI9488: PORTRAIT FLIP 240,320 Color

     Clear screen time: 60 ms
Send color buffer time: 228 us (240 pixels)
       JPG Decode time: 287 ms
    BMP time, scale: 5: 420 ms
    BMP time, scale: 4: 430 ms
    BMP time, scale: 3: 429 ms
    BMP time, scale: 2: 436 ms
    BMP time, scale: 1: 446 ms
    BMP time, scale: 0: 338 ms


```

---

### Tested on

ESP32-WROVER-KIT v3, ST7789V controller, 240x320
![Tested on](https://raw.githubusercontent.com/loboris/MicroPython_ESP32_psRAM_LoBo/master/Documents/disp_wrower-kit.jpg)

2.4" 240x320 ILI9341 conroller with Touch panel from eBay
![Tested on](https://raw.githubusercontent.com/loboris/MicroPython_ESP32_psRAM_LoBo/master/Documents/disp_ili9341.jpg)

3.5" 320x480 ILI9844 controller with Touch panel from BuyDisplay
![Tested on](https://raw.githubusercontent.com/loboris/MicroPython_ESP32_psRAM_LoBo/master/Documents/disp_9488.jpg)

1.8" 128x160 ST7735 conroller from eBay
![Tested on](https://raw.githubusercontent.com/loboris/MicroPython_ESP32_psRAM_LoBo/master/Documents/disp_7735.jpg)

