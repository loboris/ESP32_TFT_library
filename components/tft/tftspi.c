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
#include "soc/spi_reg.h"


// ====================================================
// ==== Global variables, default values ==============

// Converts colors to grayscale if set to 1
uint8_t gray_scale = 0;
// Spi clock for reading data from display memory in Hz
uint32_t max_rdclock = 16000000;

// Default display dimensions
int _width = DEFAULT_TFT_DISPLAY_WIDTH;
int _height = DEFAULT_TFT_DISPLAY_HEIGHT;

// Display type, DISP_TYPE_ILI9488 or DISP_TYPE_ILI9341
uint8_t tft_disp_type = DEFAULT_DISP_TYPE;

// Spi device handles for display and touch screen
spi_lobo_device_handle_t disp_spi = NULL;
spi_lobo_device_handle_t ts_spi = NULL;

// ====================================================


static color_t *trans_cline = NULL;
static uint8_t _dma_sending = 0;

#define DC_VAL (1 << PIN_NUM_DC)
#define DC_CMD	GPIO.out_w1tc = (DC_VAL)
#define DC_DATA	GPIO.out_w1ts = (DC_VAL)

// RGB to GRAYSCALE constants
// 0.2989  0.5870  0.1140
#define GS_FACT_R 0.2989
#define GS_FACT_G 0.4870
#define GS_FACT_B 0.2140



// ==== Functions =====================

//------------------------------------------------------
esp_err_t IRAM_ATTR wait_trans_finish(uint8_t free_line)
{
	// Wait for SPI bus ready
	while (disp_spi->host->hw->cmd.usr);
	if ((free_line) && (trans_cline)) {
		free(trans_cline);
		trans_cline = NULL;
	}
	if (_dma_sending) {
	    //Tell common code DMA workaround that our DMA channel is idle. If needed, the code will do a DMA reset.
	    if (disp_spi->host->dma_chan) spi_lobo_dmaworkaround_idle(disp_spi->host->dma_chan);

	    // Reset DMA
		disp_spi->host->hw->dma_conf.val |= SPI_OUT_RST|SPI_IN_RST|SPI_AHBM_RST|SPI_AHBM_FIFO_RST;
		disp_spi->host->hw->dma_out_link.start=0;
		disp_spi->host->hw->dma_in_link.start=0;
		disp_spi->host->hw->dma_conf.val &= ~(SPI_OUT_RST|SPI_IN_RST|SPI_AHBM_RST|SPI_AHBM_FIFO_RST);
		disp_spi->host->hw->dma_conf.out_data_burst_en=1;
		_dma_sending = 0;
	}
    return ESP_OK;
}

//-------------------------------
esp_err_t IRAM_ATTR disp_select()
{
	wait_trans_finish(1);
	return spi_lobo_device_select(disp_spi, 0);
}

//---------------------------------
esp_err_t IRAM_ATTR disp_deselect()
{
	wait_trans_finish(1);
	return spi_lobo_device_deselect(disp_spi);
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
    DC_CMD;

    disp_spi->host->hw->data_buf[0] = (uint32_t)cmd;
    disp_spi_transfer_start(8);
}

// Send command with data to display, display must be selected
//----------------------------------------------------------------------------------
void IRAM_ATTR disp_spi_transfer_cmd_data(int8_t cmd, uint8_t *data, uint32_t len) {
	// Wait for SPI bus ready
	while (disp_spi->host->hw->cmd.usr);

    // Set DC to 0 (command mode);
    DC_CMD;

    disp_spi->host->hw->data_buf[0] = (uint32_t)cmd;
    disp_spi_transfer_start(8);

	if (len == 0) return;

    // Set DC to 1 (data mode);
	DC_DATA;
    //GPIO.out_w1ts = (1 << PIN_NUM_DC);

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

    taskDISABLE_INTERRUPTS();
	// Wait for SPI bus ready
	while (disp_spi->host->hw->cmd.usr);
    DC_CMD;

	disp_spi->host->hw->data_buf[0] = (uint32_t)TFT_CASET;
	disp_spi->host->hw->user.usr_mosi_highpart = 0;
	disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 7;
	disp_spi->host->hw->user.usr_mosi = 1;
	disp_spi->host->hw->miso_dlen.usr_miso_dbitlen = 0;
	disp_spi->host->hw->user.usr_miso = 0;

	disp_spi->host->hw->cmd.usr = 1; // Start transfer

	wd = (uint32_t)(x1>>8);
	wd |= (uint32_t)(x1&0xff) << 8;
	wd |= (uint32_t)(x2>>8) << 16;
	wd |= (uint32_t)(x2&0xff) << 24;

	while (disp_spi->host->hw->cmd.usr); // wait transfer end
	DC_DATA;
	disp_spi->host->hw->data_buf[0] = wd;
	disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 31;
	disp_spi->host->hw->cmd.usr = 1; // Start transfer

    while (disp_spi->host->hw->cmd.usr);
    DC_CMD;
    disp_spi->host->hw->data_buf[0] = (uint32_t)TFT_PASET;
	disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 7;
	disp_spi->host->hw->cmd.usr = 1; // Start transfer

	wd = (uint32_t)(y1>>8);
	wd |= (uint32_t)(y1&0xff) << 8;
	wd |= (uint32_t)(y2>>8) << 16;
	wd |= (uint32_t)(y2&0xff) << 24;

	while (disp_spi->host->hw->cmd.usr);
	DC_DATA;

	disp_spi->host->hw->data_buf[0] = wd;
	disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 31;
	disp_spi->host->hw->cmd.usr = 1; // Start transfer
	while (disp_spi->host->hw->cmd.usr);
    taskENABLE_INTERRUPTS();
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

// Set display pixel at given coordinates to given color
//------------------------------------------------------------------------
void IRAM_ATTR drawPixel(int16_t x, int16_t y, color_t color, uint8_t sel)
{
	if (!(disp_spi->cfg.flags & SPI_DEVICE_HALFDUPLEX)) return;

	if (sel) {
		if (disp_select()) return;
	}
	else wait_trans_finish(1);

	uint32_t wd = 0;
    color_t _color = color;
	if (gray_scale) _color = color2gs(color);

    taskDISABLE_INTERRUPTS();
	disp_spi_transfer_addrwin(x, x+1, y, y+1);

	// Send RAM WRITE command
    DC_CMD;
    disp_spi->host->hw->data_buf[0] = (uint32_t)TFT_RAMWR;
	disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 7;
	disp_spi->host->hw->cmd.usr = 1;		// Start transfer
	while (disp_spi->host->hw->cmd.usr);	// Wait for SPI bus ready

	wd = (uint32_t)_color.r;
	wd |= (uint32_t)_color.g << 8;
	wd |= (uint32_t)_color.b << 16;

    // Set DC to 1 (data mode);
	DC_DATA;

	disp_spi->host->hw->data_buf[0] = wd;
	disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 23;
	disp_spi->host->hw->cmd.usr = 1;		// Start transfer
	while (disp_spi->host->hw->cmd.usr);	// Wait for SPI bus ready

    taskENABLE_INTERRUPTS();
   if (sel) disp_deselect();
}

//-----------------------------------------------------------
static void IRAM_ATTR _dma_send(uint8_t *data, uint32_t size)
{
    //Fill DMA descriptors
    spi_lobo_dmaworkaround_transfer_active(disp_spi->host->dma_chan); //mark channel as active
    spi_lobo_setup_dma_desc_links(disp_spi->host->dmadesc_tx, size, data, false);
    disp_spi->host->hw->user.usr_mosi_highpart=0;
    disp_spi->host->hw->dma_out_link.addr=(int)(&disp_spi->host->dmadesc_tx[0]) & 0xFFFFF;
    disp_spi->host->hw->dma_out_link.start=1;
    disp_spi->host->hw->user.usr_mosi_highpart=0;

	disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = (size * 8) - 1;

	_dma_sending = 1;
	// Start transfer
	disp_spi->host->hw->cmd.usr = 1;
}

//---------------------------------------------------------------------------
static void IRAM_ATTR _direct_send(color_t *color, uint32_t len, uint8_t rep)
{
	uint32_t cidx = 0;	// color buffer index
	uint32_t wd = 0;
	int idx = 0;
	int bits = 0;
	int wbits = 0;

    taskDISABLE_INTERRUPTS();
	color_t _color = color[0];
	if ((rep) && (gray_scale)) _color = color2gs(color[0]);

	while (len) {
		// ** Get color data from color buffer **
		if (rep == 0) {
			if (gray_scale) _color = color2gs(color[cidx]);
			else _color = color[cidx];
		}

		wd |= (uint32_t)_color.r << wbits;
		wbits += 8;
		if (wbits == 32) {
			bits += wbits;
			wbits = 0;
			disp_spi->host->hw->data_buf[idx++] = wd;
			wd = 0;
		}
		wd |= (uint32_t)_color.g << wbits;
		wbits += 8;
		if (wbits == 32) {
			bits += wbits;
			wbits = 0;
			disp_spi->host->hw->data_buf[idx++] = wd;
			wd = 0;
		}
		wd |= (uint32_t)_color.b << wbits;
		wbits += 8;
		if (wbits == 32) {
			bits += wbits;
			wbits = 0;
			disp_spi->host->hw->data_buf[idx++] = wd;
			wd = 0;
		}
    	len--;					// Decrement colors counter
        if (rep == 0) cidx++;	// if not repeating color, increment color buffer index
    }
	if (bits) {
		while (disp_spi->host->hw->cmd.usr);						// Wait for SPI bus ready
		disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = bits-1;	// set number of bits to be sent
        disp_spi->host->hw->cmd.usr = 1;							// Start transfer
	}
    taskENABLE_INTERRUPTS();
}

// ================================================================
// === Main function to send data to display ======================
// If  rep==true:  repeat sending color data to display 'len' times
// If rep==false:  send 'len' color data from color buffer to display
// ** Device must already be selected and address window set **
// ================================================================
//----------------------------------------------------------------------------------------------
static void IRAM_ATTR _TFT_pushColorRep(color_t *color, uint32_t len, uint8_t rep, uint8_t wait)
{
	if (len == 0) return;
	if (!(disp_spi->cfg.flags & SPI_DEVICE_HALFDUPLEX)) return;

	// Send RAM WRITE command
    DC_CMD;
    disp_spi->host->hw->data_buf[0] = (uint32_t)TFT_RAMWR;
	disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 7;
	disp_spi->host->hw->cmd.usr = 1;		// Start transfer
	while (disp_spi->host->hw->cmd.usr);	// Wait for SPI bus ready

	DC_DATA;								// Set DC to 1 (data mode);

	if ((len*24) <= 512) {

		_direct_send(color, len, rep);

	}
	else if (rep == 0)  {
		// ==== use DMA transfer ====
		// ** Prepare data
		if (gray_scale) {
			for (int n=0; n<len; n++) {
				color[n] = color2gs(color[n]);
			}
	    }

	    _dma_send((uint8_t *)color, len*3);
	}
	else {
		// ==== Repeat color, more than 512 bits total ====

		color_t _color;
		uint32_t buf_colors;
		int buf_bytes, to_send;

		/*
		to_send = len;
		while (to_send > 0) {
			wait_trans_finish(0);
			_direct_send(color, ((to_send > 21) ? 21 : to_send), rep);
			to_send -= 21;
		}
		*/

		buf_colors = ((len > (_width*2)) ? (_width*2) : len);
		buf_bytes = buf_colors * 3;

		// Prepare color buffer of maximum 2 color lines
		trans_cline = pvPortMallocCaps(buf_bytes, MALLOC_CAP_DMA);
		if (trans_cline == NULL) return;

		// Prepare fill color
		if (gray_scale) _color = color2gs(color[0]);
		else _color = color[0];

		// Fill color buffer with fill color
		for (uint32_t i=0; i<buf_colors; i++) {
			trans_cline[i] = _color;
		}

		// Send 'len' colors
		to_send = len;
		while (to_send > 0) {
			wait_trans_finish(0);
			_dma_send((uint8_t *)trans_cline, ((to_send > buf_colors) ? buf_bytes : (to_send*3)));
			to_send -= buf_colors;
		}
	}

	if (wait) wait_trans_finish(1);
}

// Write 'len' color data to TFT 'window' (x1,y2),(x2,y2)
//-------------------------------------------------------------------------------------------
void IRAM_ATTR TFT_pushColorRep(int x1, int y1, int x2, int y2, color_t color, uint32_t len)
{
	if (disp_select() != ESP_OK) return;

	// ** Send address window **
	disp_spi_transfer_addrwin(x1, x2, y1, y2);

	_TFT_pushColorRep(&color, len, 1, 1);

	disp_deselect();
}

// Write 'len' color data to TFT 'window' (x1,y2),(x2,y2) from given buffer
// ** Device must already be selected **
//-----------------------------------------------------------------------------------
void IRAM_ATTR send_data(int x1, int y1, int x2, int y2, uint32_t len, color_t *buf)
{
	// ** Send address window **
	disp_spi_transfer_addrwin(x1, x2, y1, y2);
	_TFT_pushColorRep(buf, len, 0, 0);
}

// Reads 'len' pixels/colors from the TFT's GRAM 'window'
// 'buf' is an array of bytes with 1st byte reserved for reading 1 dummy byte
// and the rest is actually an array of color_t values
//--------------------------------------------------------------------------------------------
int IRAM_ATTR read_data(int x1, int y1, int x2, int y2, int len, uint8_t *buf, uint8_t set_sp)
{
	spi_lobo_transaction_t t;
	uint32_t current_clock = 0;

    memset(&t, 0, sizeof(t));  //Zero out the transaction
	memset(buf, 0, len*sizeof(color_t));

	if (set_sp) {
		if (disp_deselect() != ESP_OK) return -1;
		// Change spi clock if needed
		current_clock = spi_lobo_get_speed(disp_spi);
		if (max_rdclock < current_clock) spi_lobo_set_speed(disp_spi, max_rdclock);
	}

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

	spi_lobo_transfer_data(disp_spi, &t); // Receive using direct mode

	disp_deselect();

	if (set_sp) {
		// Restore spi clock if needed
		if (max_rdclock < current_clock) spi_lobo_set_speed(disp_spi, current_clock);
	}

    return 0;
}

// Reads one pixel/color from the TFT's GRAM at position (x,y)
//-----------------------------------------------
color_t IRAM_ATTR readPixel(int16_t x, int16_t y)
{
    uint8_t color_buf[sizeof(color_t)+1] = {0};

    read_data(x, y, x+1, y+1, 1, color_buf, 1);

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
	spi_lobo_transaction_t t;
	memset(&t, 0, sizeof(t));            //Zero out the transaction
	uint8_t rxdata[2] = {0};

	// send command byte & receive 2 byte response
    t.rxlength=8*2;
    t.rx_buffer=&rxdata;
	t.command = type;

	ret = spi_lobo_transfer_data(ts_spi, &t);    // Transmit using direct mode

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
	uint32_t max_speed = 1000000;
    uint32_t change_speed, cur_speed;
    int line_check;
    color_t *color_line = NULL;
    uint8_t *line_rdbuf = NULL;
    uint8_t gs = gray_scale;

    gray_scale = 0;
    cur_speed = spi_lobo_get_speed(disp_spi);

	color_line = pvPortMallocCaps(_width*3, MALLOC_CAP_DMA);
    if (color_line == NULL) goto exit;

    line_rdbuf = pvPortMallocCaps((_width*3)+1, MALLOC_CAP_DMA);
	if (line_rdbuf == NULL) goto exit;

	color_t *rdline = (color_t *)(line_rdbuf+1);

	// Fill test line with colors
	color = (color_t){0xEC,0xA8,0x74};
	for (int x=0; x<_width; x++) {
		color_line[x] = color;
	}

	// Find maximum read spi clock
	for (uint32_t speed=2000000; speed<=cur_speed; speed += 1000000) {
		change_speed = spi_lobo_set_speed(disp_spi, speed);
		if (change_speed == 0) goto exit;

		memset(line_rdbuf, 0, _width*sizeof(color_t)+1);

		if (disp_select()) goto exit;
		// Write color line
		send_data(0, _height/2, _width-1, _height/2, _width, color_line);
		if (disp_deselect()) goto exit;

		// Read color line
		ret = read_data(0, _height/2, _width-1, _height/2, _width, line_rdbuf, 0);

		// Compare
		line_check = 0;
		if (ret == ESP_OK) {
			for (int y=0; y<_width; y++) {
				if ((color_line[y].g & 0xFC) != (rdline[y].g & 0xFC)) line_check = 1;
				else if ((color_line[y].r & 0xFC) != (rdline[y].r & 0xFC)) line_check = 1;
				else if ((color_line[y].b & 0xFC) != (rdline[y].b & 0xFC)) line_check =  1;
				if (line_check) break;
			}
		}
		else line_check = ret;

		if (line_check) break;
		max_speed = speed;
	}

exit:
    gray_scale = gs;
	if (line_rdbuf) free(line_rdbuf);
	if (color_line) free(color_line);

	// restore spi clk
	change_speed = spi_lobo_set_speed(disp_spi, cur_speed);

	return max_speed;
}

//---------------------------------------------------------------------------
// Companion code to the initialization table.
// Reads and issues a series of LCD commands stored in byte array
//---------------------------------------------------------------------------
static void commandList(spi_lobo_device_handle_t spi, const uint8_t *addr) {
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

//==================================
void _tft_setRotation(uint8_t rot) {
	uint8_t rotation = rot & 3; // can't be higher than 3
	uint8_t send = 1;
	uint8_t madctl = 0;
	uint16_t tmp;

    if ((rotation & 1)) {
        // in landscape modes width > height
        if (_width < _height) {
            tmp = _width;
            _width  = _height;
            _height = tmp;
        }
    }
    else {
        // in portrait modes width < height
        if (_width > _height) {
            tmp = _width;
            _width  = _height;
            _height = tmp;
        }
    }
    #if TFT_INVERT_ROTATION
    switch (rotation) {
        case PORTRAIT:
        madctl = (MADCTL_MV | TFT_RGB_BGR);
        break;
        case LANDSCAPE:
        madctl = (MADCTL_MX | TFT_RGB_BGR);
        break;
        case PORTRAIT_FLIP:
        madctl = (MADCTL_MV | TFT_RGB_BGR);
        break;
        case LANDSCAPE_FLIP:
        madctl = (MADCTL_MY | TFT_RGB_BGR);
        break;
    }
    #elif TFT_INVERT_ROTATION1
    switch (rotation) {
        case PORTRAIT:
        madctl = (MADCTL_MY | MADCTL_MX | TFT_RGB_BGR);
        break;
        case LANDSCAPE:
        madctl = (MADCTL_MY | MADCTL_MV | TFT_RGB_BGR);
        break;
        case PORTRAIT_FLIP:
        madctl = (TFT_RGB_BGR);
        break;
        case LANDSCAPE_FLIP:
        madctl = (MADCTL_MX | MADCTL_MV | TFT_RGB_BGR);
        break;
    }
    #else
    switch (rotation) {
        case PORTRAIT:
        madctl = (MADCTL_MX | TFT_RGB_BGR);
        break;
        case LANDSCAPE:
        madctl = (MADCTL_MV | TFT_RGB_BGR);
        break;
        case PORTRAIT_FLIP:
        madctl = (MADCTL_MY | TFT_RGB_BGR);
        break;
        case LANDSCAPE_FLIP:
        madctl = (MADCTL_MX | MADCTL_MY | MADCTL_MV | TFT_RGB_BGR);
        break;
    }
    #endif
	if (send) {
		if (disp_select() == ESP_OK) {
			disp_spi_transfer_cmd_data(TFT_MADCTL, &madctl, 1);
			disp_deselect();
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
		ret = disp_select();
		assert(ret==ESP_OK);
		commandList(disp_spi, ILI9488_init);
	}
	else if (tft_disp_type == DISP_TYPE_ST7789V) {
		ret = disp_select();
		assert(ret==ESP_OK);
		commandList(disp_spi, ST7789V_init);
	}
	else assert(0);

    ret = disp_deselect();
	assert(ret==ESP_OK);

	// Clear screen
    _tft_setRotation(PORTRAIT);
	TFT_pushColorRep(0, 0, _width-1, _height-1, (color_t){0,0,0}, (uint32_t)(_height*_width));

	///Enable backlight
#if PIN_NUM_BCKL
    gpio_set_level(PIN_NUM_BCKL, PIN_BCKL_ON);
#endif
}


