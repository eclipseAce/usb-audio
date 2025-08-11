/*
 * lcd_st7789.c
 *
 *  Created on: Apr 10, 2024
 *      Author: Administrator
 */

#include "main.h"
#include "lcd.h"
#include "lcd_st7789.h"

#define FRAME_BUFFER_BYTES (TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t))

extern SPI_HandleTypeDef hspi1;

uint8_t frameBuffer[FRAME_BUFFER_BYTES] = { 0 };

volatile uint32_t nBytesSyncing = 0;
volatile uint32_t nBytesUnsync = 0;

static void begin_tft_write() {
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
}

static void end_tft_write() {
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

static void writecommand(uint8_t cmd) {
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
}

static void writedata(uint8_t data) {
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
}

static void writedata16(uint16_t data) {
  uint8_t buf[2] = { (data >> 8), (data & 0xFF) };
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  HAL_SPI_Transmit(&hspi1, buf, sizeof(buf), HAL_MAX_DELAY);
}

static void delay(uint32_t ms) {
  HAL_Delay(ms);
}

static void setwindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
  writecommand(TFT_CASET); writedata16(x0); writedata16(x1);
  writecommand(TFT_RASET); writedata16(y0); writedata16(y1);
  writecommand(TFT_RAMWR);
}

void LCD_Init(void) {
  HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
  delay(2000);
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
  delay(120);

  begin_tft_write();

  writecommand(ST7789_SLPOUT);   // Sleep out
  delay(120);

  writecommand(ST7789_NORON);    // Normal display mode on

  //------------------------------display and color format setting--------------------------------//
  writecommand(ST7789_MADCTL);
  //writedata(0x00);
  writedata(TFT_MAD_COLOR_ORDER);

  // JLX240 display datasheet
  writecommand(0xB6);
  writedata(0x0A);
  writedata(0x82);

  writecommand(ST7789_RAMCTRL);
  writedata(0x00);
  writedata(0xE0); // 5 to 6 bit conversion: r0 = r5, b0 = b5

  writecommand(ST7789_COLMOD);
  writedata(0x55);
  delay(10);

  //--------------------------------ST7789V Frame rate setting----------------------------------//
  writecommand(ST7789_PORCTRL);
  writedata(0x0c);
  writedata(0x0c);
  writedata(0x00);
  writedata(0x33);
  writedata(0x33);

  writecommand(ST7789_GCTRL);      // Voltages: VGH / VGL
  writedata(0x35);

  //---------------------------------ST7789V Power setting--------------------------------------//
  writecommand(ST7789_VCOMS);
  writedata(0x28);    // JLX240 display datasheet

  writecommand(ST7789_LCMCTRL);
  writedata(0x0C);

  writecommand(ST7789_VDVVRHEN);
  writedata(0x01);
  writedata(0xFF);

  writecommand(ST7789_VRHS);       // voltage VRHS
  writedata(0x10);

  writecommand(ST7789_VDVSET);
  writedata(0x20);

  writecommand(ST7789_FRCTR2);
  writedata(0x0f);

  writecommand(ST7789_PWCTRL1);
  writedata(0xa4);
  writedata(0xa1);

  //--------------------------------ST7789V gamma setting---------------------------------------//
  writecommand(ST7789_PVGAMCTRL);
  writedata(0xd0);
  writedata(0x00);
  writedata(0x02);
  writedata(0x07);
  writedata(0x0a);
  writedata(0x28);
  writedata(0x32);
  writedata(0x44);
  writedata(0x42);
  writedata(0x06);
  writedata(0x0e);
  writedata(0x12);
  writedata(0x14);
  writedata(0x17);

  writecommand(ST7789_NVGAMCTRL);
  writedata(0xd0);
  writedata(0x00);
  writedata(0x02);
  writedata(0x07);
  writedata(0x0a);
  writedata(0x28);
  writedata(0x31);
  writedata(0x54);
  writedata(0x47);
  writedata(0x0e);
  writedata(0x1c);
  writedata(0x17);
  writedata(0x1b);
  writedata(0x1e);

  writecommand(ST7789_INVON);

  writecommand(ST7789_CASET);    // Column address set
  writedata(0x00);
  writedata(0x00);
  writedata(0x00);
  writedata(0xEF);    // 239

  writecommand(ST7789_RASET);    // Row address set
  writedata(0x00);
  writedata(0x00);
  writedata(0x01);
  writedata(0x3F);    // 319

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  end_tft_write();
  delay(120);

  HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET);

  begin_tft_write();
  writecommand(ST7789_DISPON);    //Display on
  end_tft_write();
  delay(120);
}

void LCD_Sync(void) {
  if (nBytesSyncing > 0) {
    return;
  }
  if (nBytesUnsync <= 0) {
    end_tft_write();
    return;
  }
  if (nBytesUnsync == FRAME_BUFFER_BYTES) {
    begin_tft_write();
    setwindow(0, 0, TFT_WIDTH, TFT_HEIGHT);
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  }

  uint8_t *buf = frameBuffer + (FRAME_BUFFER_BYTES - nBytesUnsync);
  uint16_t size = (uint16_t) (nBytesUnsync > 0xFFFF ? 0xFFFF : nBytesUnsync);

  nBytesSyncing = size;
  nBytesUnsync -= size;
  HAL_SPI_Transmit_DMA(&hspi1, buf, size);
}

void LCD_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
  for (uint8_t i = x; i < x + w; i++) {
    for (uint8_t j = y; j < y + h; j++)  {
        ((uint16_t*) frameBuffer)[j * TFT_WIDTH + i] = color;
    }
  }
  nBytesUnsync = FRAME_BUFFER_BYTES;
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
  nBytesSyncing = 0;
  LCD_Sync();
}
