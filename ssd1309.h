#ifndef SSD1306_H
#define SSD1306_H 100


#include "stm32f072xb.h"
#include "stdlib.h"
#include "string.h"
#include "fonts.h"



/* I2C address */
#ifndef SSD1306_I2C_ADDR
#define SSD1306_I2C_ADDR         0x78
#endif

/* SSD1306 settings */
/* SSD1306 width in pixels */
#ifndef SSD1306_WIDTH
#define SSD1306_WIDTH            128
#endif
/* SSD1306 LCD height in pixels */
#ifndef SSD1306_HEIGHT
#define SSD1306_HEIGHT           64
#endif


/**
 * @brief  Initialize SSD1309
 * @param  :None
 * @retval None
 */
void ssd1306_I2C_Init(void);
void ssd1306_Init_Display();
void ssd1306_I2C_Write(uint8_t address, uint8_t reg, uint8_t data);
void SSD1306_WRITECOMMAND(uint8_t command);
void SSD1306_Fill(uint8_t color);
void SSD1306_UpdateScreen(void);
void ssd1306_I2C_WriteMulti(uint8_t address, uint8_t reg, uint8_t* data, uint16_t count);
void transmit_Data_I2C2(uint8_t slvAddr, uint8_t noOfBits, uint8_t txdrData);
void ssd1306_I2C_WriteData(uint8_t data);
void SSD1306_DrawPixel(uint16_t x, uint16_t y, uint8_t color);
void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t c);
void SSD1306_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t c);
char SSD1306_Putc(char ch, FontDef_t* Font, uint8_t color);
void SSD1306_GotoXY(uint16_t x, uint16_t y);
char SSD1306_Puts(char* str, FontDef_t* Font, uint8_t color);
#endif
