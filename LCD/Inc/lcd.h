/*
 * lcd.h
 *
 *  Created on: Apr 10, 2024
 *      Author: Administrator
 */

#ifndef INC_LCD_H_
#define INC_LCD_H_

#include <inttypes.h>

void LCD_Init(void);
void LCD_Sync(void);
void LCD_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);

#endif /* INC_LCD_H_ */
