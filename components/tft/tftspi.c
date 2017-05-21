/*
 *  Author: LoBo (loboris@gmail.com, loboris.github)
 *
 *  Module supporting SPI TFT displays based on ILI9341 & ILI9488 controllers
 * 
 * HIGH SPEED LOW LEVEL DISPLAY FUNCTIONS
 * USING DIRECT or DMA SPI TRANSFER MODEs
 *
*/

#include <string.h>
#include "tftspi.h"
#include "esp_system.h"
#include "freertos/task.h"
#include "esp_heap_alloc_caps.h"

// RGB to GRAYSCALE constants
// 0.2989  0.5870  0.1140
#define GS_FACT_R 0.2989
#define GS_FACT_G 0.4870
#define GS_FACT_B 0.2140

// ==== Global variables, default values ==============

// Color mode; 24 or 16 (only for ILI9341
uint8_t COLOR_BITS = 24;
// Converts colors to grayscale if 1
uint8_t gray_scale = 0;
// Spi clock for reading data prom display memory in Hz
uint32_t max_rdclock = 16000000;

// Display dimensions
int _width = TFT_DISPLAY_WIDTH;
int _height = TFT_DISPLAY_HEIGHT;

// Display type, DISP_TYPE_ILI9488 or DISP_TYPE_ILI9341
uint8_t tft_disp_type = DISP_TYPE_ILI9488;

// Spi device handles for display and touch screen
spi_nodma_device_handle_t disp_spi = NULL;
spi_nodma_device_handle_t ts_spi = NULL;

static uint8_t tft_in_trans = 0;
static spi_nodma_transaction_t tft_trans;
// ====================================================

static color_t *trans_cline = NULL;

// ==== Functions =====================

//-------------------------------------
esp_err_t IRAM_ATTR wait_trans_finish()
{
    if (tft_in_trans == 0) return ESP_OK;

    spi_nodma_transaction_t *rtrans;
    // Wait for transaction to be done
    esp_err_t ret = spi_device_get_trans_result(disp_spi, &rtrans, 1000*portTICK_RATE_MS);
    tft_in_trans = 0;
    if (trans_cline) {
    	free(trans_cline);
    	trans_cline = NULL;
    }
    return ret;
}

//-------------------------------
esp_err_t IRAM_ATTR disp_select()
{
	esp_err_t ret = wait_trans_finish();
	if (ret != ESP_OK) return ret;

	return spi_nodma_device_select(disp_spi, 0);
}

//---------------------------------
esp_err_t IRAM_ATTR disp_deselect()
{
	esp_err_t ret = wait_trans_finish();
	if (ret != ESP_OK) return ret;

	return spi_nodma_device_deselect(disp_spi);
}

// Start spi bus transfer of given number of bits
//-------------------------------------------------------
static void IRAM_ATTR disp_spi_transfer_start(int bits) {
	// Load send buffer
	disp_spi->host->hw->user.usr_mosi_highpart = 0;
	disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = bits-1;
	disp_spi->host->hw->user.usr_mosi = 1;
	disp_spi->host->hw->miso_dlen.usr_miso_dbitlen = 0;
	disp_spi->host->hw->user.usr_miso = 0;
	// Start transfer
	disp_spi->host->hw->cmd.usr = 1;
    // Wait for SPI bus ready
	while (disp_spi->host->hw->cmd.usr);
}

// Send 1 byte display command, display must be selected
//------------------------------------------------
void IRAM_ATTR disp_spi_transfer_cmd(int8_t cmd) {
	// Wait for SPI bus ready
	while (disp_spi->host->hw->cmd.usr);

	// Set DC to 0 (command mode);
    gpio_set_level(PIN_NUM_DC, 0);

    disp_spi->host->hw->data_buf[0] = (uint32_t)cmd;
    disp_spi_transfer_start(8);
}

// Send command with data to display, display must be selected
//----------------------------------------------------------------------------------
void IRAM_ATTR disp_spi_transfer_cmd_data(int8_t cmd, uint8_t *data, uint32_t len) {
	// Wait for SPI bus ready
	while (disp_spi->host->hw->cmd.usr);

    // Set DC to 0 (command mode);
    gpio_set_level(PIN_NUM_DC, 0);

    disp_spi->host->hw->data_buf[0] = (uint32_t)cmd;
    disp_spi_transfer_start(8);

	if (len == 0) return;

    // Set DC to 1 (data mode);
	gpio_set_level(PIN_NUM_DC, 1);

	uint8_t idx=0, bidx=0;
	uint32_t bits=0;
	uint32_t count=0;
	uint32_t wd = 0;
	while (count < len) {
		// get data byte from buffer
		wd |= (uint32_t)data[count] << bidx;
    	count++;
    	bits += 8;
		bidx += 8;
    	if (count == len) {
    		disp_spi->host->hw->data_buf[idx] = wd;
    		break;
    	}
		if (bidx == 32) {
			disp_spi->host->hw->data_buf[idx] = wd;
			idx++;
			bidx = 0;
			wd = 0;
		}
    	if (idx == 16) {
    		// SPI buffer full, send data
			disp_spi_transfer_start(bits);
    		
			bits = 0;
    		idx = 0;
			bidx = 0;
    	}
    }
    if (bits > 0) disp_spi_transfer_start(bits);
}

// Set the address window for display write & read commands, display must be selected
//---------------------------------------------------------------------------------------------------
static void IRAM_ATTR disp_spi_transfer_addrwin(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2) {
	uint32_t wd;

	// Wait for SPI bus ready
	while (disp_spi->host->hw->cmd.usr);
	disp_spi_transfer_cmd(TFT_CASET);

	wd = (uint32_t)(x1>>8);
	wd |= (uint32_t)(x1&0xff) << 8;
	wd |= (uint32_t)(x2>>8) << 16;
	wd |= (uint32_t)(x2&0xff) << 24;

    // Set DC to 1 (data mode);
	gpio_set_level(PIN_NUM_DC, 1);

	disp_spi->host->hw->data_buf[0] = wd;
    disp_spi_transfer_start(32);

	disp_spi_transfer_cmd(TFT_PASET);

	wd = (uint32_t)(y1>>8);
	wd |= (uint32_t)(y1&0xff) << 8;
	wd |= (uint32_t)(y2>>8) << 16;
	wd |= (uint32_t)(y2&0xff) << 24;

    // Set DC to 1 (data mode);
	gpio_set_level(PIN_NUM_DC, 1);

	disp_spi->host->hw->data_buf[0] = wd;
    disp_spi_transfer_start(32);
}

// Convert color to gray scale
//----------------------------------------------
static color_t IRAM_ATTR color2gs(color_t color)
{
	color_t _color;
    float gs_clr = GS_FACT_R * color.r + GS_FACT_G * color.g + GS_FACT_B * color.b;
    if (gs_clr > 255) gs_clr = 255;

    _color.r = (uint8_t)gs_clr;
    _color.g = (uint8_t)gs_clr;
    _color.b = (uint8_t)gs_clr;

    return _color;
}

// Pack RGB 18-bit 3 byte colors {R,G,B}, to RGB565 16-bit integer
//---------------------------------------------------
static IRAM_ATTR uint16_t pack_color16(color_t color)
{
	uint16_t _color;

	_color  = (uint16_t)(color.r & 0xF8);
	_color |= (uint16_t)((color.g & 0xE0) >> 5);
	_color |= (uint16_t)((color.g & 0x1C) << 3) << 8;
	_color |= (uint16_t)((color.b & 0xF8) >> 3) << 8;

	return _color;
}

// Pack RGB 18-bit 3 byte colors {R,G,B}, to RGB565 32-bit integer
//---------------------------------------------------
static IRAM_ATTR uint32_t pack_color32(color_t color)
{
	uint32_t _color;

	_color  = (uint32_t)(color.r & 0xF8);
	_color |= (uint32_t)((color.g & 0xE0) >> 5);
	_color |= (uint32_t)((color.g & 0x1C) << 3) << 8;
	_color |= (uint32_t)((color.b & 0xF8) >> 3) << 8;

	return _color;
}

// Set display pixel at given coordinates to given color
//------------------------------------------------------------------------
void IRAM_ATTR drawPixel(int16_t x, int16_t y, color_t color, uint8_t sel)
{
	if (!(disp_spi->cfg.flags & SPI_DEVICE_HALFDUPLEX)) return;

	if (sel) {
		if (disp_select()) return;
	}
	else wait_trans_finish();

	uint32_t wd = 0;
    color_t _color = color;
	if (gray_scale) _color = color2gs(color);

	disp_spi_transfer_addrwin(x, x+1, y, y+1);
	disp_spi_transfer_cmd(TFT_RAMWR);

	if (COLOR_BITS == 16) wd = pack_color32(_color);
	else {
		wd = (uint32_t)_color.r;
		wd |= (uint32_t)_color.g << 8;
		wd |= (uint32_t)_color.b << 16;
	}

    // Set DC to 1 (data mode);
	gpio_set_level(PIN_NUM_DC, 1);

	disp_spi->host->hw->data_buf[0] = wd;
    disp_spi_transfer_start(COLOR_BITS);

    if (sel) disp_deselect();
}

// If rep==true  repeat sending color data to display 'len' times
// If rep==false send 'len' color data from color buffer to display
// ** Device must already be selected and address window set **
//--------------------------------------------------------------------------------
static void IRAM_ATTR _TFT_pushColorRep(color_t *color, uint32_t len, uint8_t rep)
{
	if (!(disp_spi->cfg.flags & SPI_DEVICE_HALFDUPLEX)) return;

	uint32_t count = 0;	// sent color counter
	uint32_t cidx = 0;	// color buffer index
	uint32_t wd = 0;	// used to place color data to 32-bit register in hw spi buffer

	color_t _color = color[0];
	if ((rep) && (gray_scale)) _color = color2gs(color[0]);

	disp_spi_transfer_cmd(TFT_RAMWR);

	gpio_set_level(PIN_NUM_DC, 1);						// Set DC to 1 (data mode);

	while (count < len) {
    	// ==== Push color data to spi buffer ====
		// ** Get color data from color buffer **
		if (rep == 0) {
			if (gray_scale) _color = color2gs(color[cidx]);
			else _color = color[cidx];
		}

		if (COLOR_BITS == 16) wd = pack_color32(_color);
		else {
			wd = (uint32_t)_color.r;
			wd |= (uint32_t)_color.g << 8;
			wd |= (uint32_t)_color.b << 16;
		}
		while (disp_spi->host->hw->cmd.usr);							// Wait for SPI bus ready

		disp_spi->host->hw->data_buf[0] = wd;
		disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = COLOR_BITS-1;	// set number of bits to be sent
        disp_spi->host->hw->cmd.usr = 1;								// Start transfer

    	count++;				// Increment sent colors counter
        if (rep == 0) cidx++;	// if not repeating color, increment color buffer index
    }
	while (disp_spi->host->hw->cmd.usr);  // Wait for SPI bus ready after last color was sent
}

// ** Send color data using DMA mode **
// ** Device must already be selected and address window set **
//------------------------------------------------------------------------
static void IRAM_ATTR _TFT_pushColorRep_trans(color_t color, uint32_t len)
{
    uint32_t size, tosend;
    esp_err_t ret = ESP_OK;
    spi_nodma_transaction_t *rtrans;

    color_t _color = color;
	if (gray_scale) _color = color2gs(color);

	size = len;
    if (size > _width) size = _width;

    trans_cline = pvPortMallocCaps((size * 3), MALLOC_CAP_DMA);
    if (trans_cline == NULL) return;

	uint16_t *buf16 = (uint16_t *)trans_cline;
    for (uint32_t n=0; n<size;n++) {
	    if (COLOR_BITS == 16) buf16[n] = pack_color16(_color);
	    else trans_cline[n] = _color;
    }

	// ** RAM write command
	// Set DC to 0 (command mode);
    gpio_set_level(PIN_NUM_DC, 0);
    disp_spi->host->hw->data_buf[0] = (uint32_t)TFT_RAMWR;
    disp_spi_transfer_start(8);

	// Set DC to 1 (data mode);
	gpio_set_level(PIN_NUM_DC, 1);

	tosend = len;
	while (tosend > 0) {
	    memset(&tft_trans, 0, sizeof(spi_nodma_transaction_t));
		size = tosend;
	    if (size > _width) size = _width;

	    tft_trans.tx_buffer = (uint8_t *)trans_cline;
	    //Set data length, in bits
	    if (COLOR_BITS == 16) tft_trans.length = size * 2 * 8;
	    else  tft_trans.length = size * 3 * 8;

	    //Queue transaction.
		ret = spi_device_transmit(disp_spi, &tft_trans, 1000*portTICK_RATE_MS);
		if (ret != ESP_OK) {
		    tft_in_trans = 0;
		    esp_err_t ret = spi_device_get_trans_result(disp_spi, &rtrans, 2000*portTICK_RATE_MS);
		    tft_in_trans = 0;
			break;
		}

	    tosend -= size;
    }
	free(trans_cline);
	trans_cline = NULL;
}

// Write 'len' color data to TFT 'window' (x1,y2),(x2,y2)
//-------------------------------------------------------------------------------------------
void IRAM_ATTR TFT_pushColorRep(int x1, int y1, int x2, int y2, color_t color, uint32_t len)
{
	if (disp_select() != ESP_OK) return;

	// ** Send address window **
	disp_spi_transfer_addrwin(x1, x2, y1, y2);

	if (((COLOR_BITS == 24) && (len > 21)) || ((COLOR_BITS == 16) && (len > 32))) _TFT_pushColorRep_trans(color, len);
	else _TFT_pushColorRep(&color, len, 1);

	disp_deselect();
}

// Write 'len' color data to TFT 'window' (x1,y2),(x2,y2) from given buffer
// ** Device must already be selected **
//-----------------------------------------------------------------------------------
void IRAM_ATTR send_data(int x1, int y1, int x2, int y2, uint32_t len, color_t *buf)
{
	// ** Send address window **
	disp_spi_transfer_addrwin(x1, x2, y1, y2);

	if (((COLOR_BITS == 24) && (len > 21)) || ((COLOR_BITS == 16) && (len > 32))) {
	    // ** Send color data using transaction mode **
		// ** RAM write command
		// Set DC to 0 (command mode);
	    gpio_set_level(PIN_NUM_DC, 0);
	    disp_spi->host->hw->data_buf[0] = (uint32_t)TFT_RAMWR;
	    disp_spi_transfer_start(8);

		//Transaction descriptors
	    memset(&tft_trans, 0, sizeof(spi_nodma_transaction_t));

		uint32_t size = len * 3;

	    if ((gray_scale) || (COLOR_BITS == 16)) {
	    	trans_cline = pvPortMallocCaps(len*3, MALLOC_CAP_DMA);
	    	if (trans_cline == NULL) return;
	    	if (gray_scale) {
	    		for (int n=0; n<len; n++) {
	    			trans_cline[n] = color2gs(buf[n]);
	    		}
	    	}
	    	if (COLOR_BITS == 16) {
				uint16_t *buf16 = (uint16_t *)trans_cline;
				for (int n=0; n<len; n++) {
			    	if (gray_scale) buf16[n] = pack_color16(trans_cline[n]);
			    	else buf16[n] = pack_color16(buf[n]);
				}
		    	size = len * 2;
		    }
		    tft_trans.tx_buffer = (uint8_t *)trans_cline;
	    }
	    else tft_trans.tx_buffer = (uint8_t *)buf;

	    tft_trans.length = size * 8;  //Data length, in bits

	    // Set DC to 1 (data mode);
		gpio_set_level(PIN_NUM_DC, 1);

	    //Queue transaction.
	    spi_device_queue_trans(disp_spi, &tft_trans, 1000*portTICK_RATE_MS);
	    tft_in_trans = 1;
	}
	else {
		// ** Send color data using direct mode **
		_TFT_pushColorRep(buf, len, 0);
	}
}

// Reads 'len' pixels/colors from the TFT's GRAM 'window'
// 'buf' is an array of bytes with 1st byte reserved for reading 1 dummy byte
// and the rest is actually an array of color_t values
//----------------------------------------------------------------------------
int IRAM_ATTR read_data(int x1, int y1, int x2, int y2, int len, uint8_t *buf)
{
	spi_nodma_transaction_t t;
    memset(&t, 0, sizeof(t));  //Zero out the transaction
	memset(buf, 0, len*sizeof(color_t));

	if (disp_deselect() != ESP_OK) return -1;

	// Change spi clock if needed
	uint32_t current_clock = spi_nodma_get_speed(disp_spi);
	if (max_rdclock < current_clock) spi_nodma_set_speed(disp_spi, max_rdclock);

	if (disp_select() != ESP_OK) return -2;

	// ** Send address window **
	disp_spi_transfer_addrwin(x1, x2, y1, y2);

    // ** GET pixels/colors **
	disp_spi_transfer_cmd(TFT_RAMRD);

    t.length=0;                //Send nothing
    t.tx_buffer=NULL;
    t.rxlength=8*((len*3)+1);  //Receive size in bits
    t.rx_buffer=buf;
    t.user = (void*)1;

	spi_nodma_transfer_data(disp_spi, &t); // Receive using direct mode

	disp_deselect();

	// Restore spi clock if needed
	if (max_rdclock < current_clock) spi_nodma_set_speed(disp_spi, current_clock);

    return 0;
}

// Reads one pixel/color from the TFT's GRAM at position (x,y)
//-----------------------------------------------
color_t IRAM_ATTR readPixel(int16_t x, int16_t y)
{
    uint8_t color_buf[sizeof(color_t)+1] = {0};

    read_data(x, y, x+1, y+1, 1, color_buf);

    color_t color;
	color.r = color_buf[1];
	color.g = color_buf[2];
	color.b = color_buf[3];
	return color;
}

// get 16-bit data from touch controller for specified type
// ** Touch device must already be selected **
//----------------------------------------
int IRAM_ATTR touch_get_data(uint8_t type)
{
	esp_err_t ret;
	spi_nodma_transaction_t t;
	memset(&t, 0, sizeof(t));            //Zero out the transaction
	uint8_t rxdata[2] = {0};

	// send command byte & receive 2 byte response
    t.rxlength=8*2;
    t.rx_buffer=&rxdata;
	t.command = type;

	ret = spi_nodma_transfer_data(ts_spi, &t);    // Transmit using direct mode

	if (ret != ESP_OK) return -1;

	return (((int)(rxdata[0] << 8) | (int)(rxdata[1])) >> 4);
}

// Find maximum spi clock for successful read from display RAM
// ** Must be used AFTER the display is initialized **
//======================
uint32_t find_rd_speed()
{
	esp_err_t ret;
	color_t color;
	uint32_t max_speed = 8000000;
    uint32_t change_speed, cur_speed;
    int line_check;
    uint8_t ceq;

    cur_speed = spi_nodma_get_speed(disp_spi);

    color_t *color_line = malloc((_width*3) + 1);
    assert(color_line);

    uint8_t *line_rdbuf = malloc(_width*sizeof(color_t)+1);
	assert(line_rdbuf);

	color_t *rdline = (color_t *)(line_rdbuf+1);

	// Fill test line with colors
	color = (color_t){0xEC,0xA8,0x74};
	for (int x=0; x<_width; x++) {
		color_line[x] = color;
		rdline[x] = color;
	}

	// Find maximum read spi clock
	for (uint32_t speed=8000000; speed<=40000000; speed += 2000000) {
		change_speed = spi_nodma_set_speed(disp_spi, speed);
		assert(change_speed > 0 );

		ret = disp_select();
		assert(ret==ESP_OK);

		// Write color line
		send_data(0, _height/2, _width-1, _height/2, _width, color_line);

		ret = disp_deselect();
		assert(ret==ESP_OK);

		// Read color line
		ret = read_data(0, _height/2, _width-1, _height/2, _width, line_rdbuf);

		// Compare
		line_check = 0;
		if (ret == ESP_OK) {
		    uint8_t ceq = 0;
		    uint8_t mask;
			for (int y=0; y<_width; y++) {
				mask = ((COLOR_BITS == 24) ? 0xFC : 0xF8);
				if ((color_line[y].g & 0xFC) != (rdline[y].g & 0xFC)) ceq = 1;
				else {
					if ((color_line[y].r & mask) != (rdline[y].r & mask)) ceq = 1;
					else if ((color_line[y].b & mask) != (rdline[y].b & mask)) ceq =  1;
				}
				if (ceq) {
					line_check = y+1;
					break;
				}
			}
		}
		else line_check = ret;

		if (line_check) break;
		max_speed = speed;
	}

	free(line_rdbuf);
	free(color_line);

	// restore spi clk
	change_speed = spi_nodma_set_speed(disp_spi, cur_speed);

	return max_speed;
}

//-------------------------------
void set_color_bits(uint8_t bits)
{
	if (tft_disp_type == DISP_TYPE_ILI9488) return;

	uint8_t tft_pix_fmt;

	if (bits == 16) tft_pix_fmt = DISP_COLOR_BITS_16;
    else if (bits == 24) tft_pix_fmt = DISP_COLOR_BITS_24;
    else return;

	disp_select();
	disp_spi_transfer_cmd_data(TFT_CMD_PIXFMT, &tft_pix_fmt, 1);
	disp_deselect();

	COLOR_BITS = bits;
}

//---------------------------------------------------------------------------
// Companion code to the initialization table.
// Reads and issues a series of LCD commands stored in byte array
//---------------------------------------------------------------------------
static void commandList(spi_nodma_device_handle_t spi, const uint8_t *addr) {
  uint8_t  numCommands, numArgs, cmd;
  uint16_t ms;

  numCommands = *addr++;				// Number of commands to follow
  while(numCommands--) {				// For each command...
    cmd = *addr++;						// save command
    numArgs  = *addr++;					// Number of args to follow
    ms       = numArgs & TFT_CMD_DELAY;	// If high bit set, delay follows args
    numArgs &= ~TFT_CMD_DELAY;			// Mask out delay bit

	disp_spi_transfer_cmd_data(cmd, (uint8_t *)addr, numArgs);

	addr += numArgs;

    if(ms) {
      ms = *addr++;              // Read post-command delay time (ms)
      if(ms == 255) ms = 500;    // If 255, delay for 500 ms
	  vTaskDelay(ms / portTICK_RATE_MS);
    }
  }
}

// Initialize the display
// ====================
void TFT_display_init()
{
    esp_err_t ret;

    //Initialize non-SPI GPIOs
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);

#if PIN_NUM_BCKL
    gpio_set_direction(PIN_NUM_BCKL, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_BCKL, PIN_BCKL_OFF);
#endif

#if PIN_NUM_RST
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    //Reset the display
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(100 / portTICK_RATE_MS);
#endif

    //Send all the initialization commands
	if (tft_disp_type == DISP_TYPE_ILI9341) {
		ret = disp_select();
		assert(ret==ESP_OK);
		commandList(disp_spi, ILI9341_init);
	}
	else if (tft_disp_type == DISP_TYPE_ILI9488) {
		COLOR_BITS = 24;  // only 18-bit color format supported
		ret = disp_select();
		assert(ret==ESP_OK);
		commandList(disp_spi, ILI9488_init);
	}
	else assert(0);

    ret = disp_deselect();
	assert(ret==ESP_OK);

	set_color_bits(COLOR_BITS);

	// Clear screen
	TFT_pushColorRep(0, 0, _width-1, _height-1, (color_t){0,0,0}, (uint32_t)(_height*_width));

	///Enable backlight
#if PIN_NUM_BCKL
    gpio_set_level(PIN_NUM_BCKL, PIN_BCKL_ON);
#endif
}


