#include "ssd1309.h"
#include "fonts.h"



static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
static uint8_t temp[SSD1306_WIDTH * SSD1306_HEIGHT / 8];



/* Private SSD1306 structure */
typedef struct {
	uint16_t CurrentX;
	uint16_t CurrentY;
	uint8_t Inverted;
	uint8_t Initialized;
} SSD1306_t;

/* Private variable */
static SSD1306_t SSD1306;
volatile uint8_t rwCnt = 0;

/* Initialize SSD1306 on I2C2 */
void ssd1306_I2C_Init(void) {
    
  // Enable I2C2 Clock
  RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;  //Enable I2C2 Clock

	// Set PB11 - AF1
	GPIOB->MODER |= (GPIO_MODER_MODER11_1);          
	GPIOB->OTYPER |= (GPIO_OTYPER_OT_11);            
	GPIOB->AFR[1] |= (0x1 << GPIO_AFRH_AFSEL11_Pos); 
	
	// Set PB13 - AF5
	GPIOB->MODER |= (GPIO_MODER_MODER13_1);          
	GPIOB->OTYPER |= (GPIO_OTYPER_OT_13);            
	GPIOB->AFR[1] |= (0x5 << GPIO_AFRH_AFSEL13_Pos); 
	
	// Set PB14 - initialize high
	GPIOB->MODER |= (GPIO_MODER_MODER14_0);              
	GPIOB->OTYPER &= ~(GPIO_OTYPER_OT_14);  
	GPIOB->ODR |= (1<<14);	

	// Set PC0 - initialize high
	GPIOC->MODER |= (GPIO_MODER_MODER0_0);              
	GPIOC->OTYPER &= ~(GPIO_OTYPER_OT_0);               
	GPIOC->ODR |= (1<<0);

    // Set I2C2 to 400Khz
	I2C2->TIMINGR |= (0x0  << I2C_TIMINGR_PRESC_Pos);  
	I2C2->TIMINGR |= (0x13 << I2C_TIMINGR_SCLL_Pos);   
	I2C2->TIMINGR |= (0x0F << I2C_TIMINGR_SCLH_Pos);   
	I2C2->TIMINGR |= (0x02  << I2C_TIMINGR_SDADEL_Pos); 
	I2C2->TIMINGR |= (0x04  << I2C_TIMINGR_SCLDEL_Pos); 
	
	
	I2C2->CR1 |= I2C_CR1_PE; 
	
	I2C2->CR2 &= ~((0x7F << 16) | (0x3FF << 0));
}
void ssd1306_Init_Display() {
	//Initialize I2C2 for SSD1309
	ssd1306_I2C_Init();


	// A little delay
	uint32_t p = 2500;
	while(p>0)
		p--;


	// Init LCD
	SSD1306_WRITECOMMAND(0xAE); //display off
	SSD1306_WRITECOMMAND(0x20); //Set Memory Addressing Mode   
	SSD1306_WRITECOMMAND(0x00); //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
	SSD1306_WRITECOMMAND(0xB0); //Set Page Start Address for Page Addressing Mode,0-7
	SSD1306_WRITECOMMAND(0xC8); //Set COM Output Scan Direction
	SSD1306_WRITECOMMAND(0x00); //---set low column address
	SSD1306_WRITECOMMAND(0x10); //---set high column address
	SSD1306_WRITECOMMAND(0x40); //--set start line address
	SSD1306_WRITECOMMAND(0x81); //--set contrast control register
	SSD1306_WRITECOMMAND(0xFF);
	SSD1306_WRITECOMMAND(0xA1); //--set segment re-map 0 to 127
	SSD1306_WRITECOMMAND(0xA6); //--set normal display
	SSD1306_WRITECOMMAND(0xA8); //--set multiplex ratio(1 to 64)
	SSD1306_WRITECOMMAND(0x3F); //
	SSD1306_WRITECOMMAND(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
	SSD1306_WRITECOMMAND(0xD3); //-set display offset
	SSD1306_WRITECOMMAND(0x00); //-not offset
	SSD1306_WRITECOMMAND(0xD5); //--set display clock divide ratio/oscillator frequency
	SSD1306_WRITECOMMAND(0xF0); //--set divide ratio
	SSD1306_WRITECOMMAND(0xD9); //--set pre-charge period
	SSD1306_WRITECOMMAND(0x22); //
	SSD1306_WRITECOMMAND(0xDA); //--set com pins hardware configuration
	SSD1306_WRITECOMMAND(0x12);
	SSD1306_WRITECOMMAND(0xDB); //--set vcomh
	SSD1306_WRITECOMMAND(0x20); //0x20,0.77xVcc
	SSD1306_WRITECOMMAND(0x8D); //--set DC-DC enable
	SSD1306_WRITECOMMAND(0x14); //
	SSD1306_WRITECOMMAND(0xAF); //--turn on SSD1306 panel
	//SSD1306_WRITECOMMAND(0xC0|(1<<3));
	//SSD1306_WRITECOMMAND(0xA0|1);
	//SSD1306_WRITECOMMAND(0xA7);
	// Clear screen

	SSD1306.CurrentX = 0;
	SSD1306.CurrentY = 0;
	
	/* Initialized OK */
	SSD1306.Initialized = 1;
	

	SSD1306_Fill(0);
	SSD1306_UpdateScreen();
}


/*USART3 Send String*/
void UART3_SendStr(char str[]){
    uint8_t send = 0;
		while(str[send] != '\0'){
			if ((USART3->ISR & USART_ISR_TC) == USART_ISR_TC){
				USART3->TDR = str[send++];
			}
		}
		USART3->TDR = '\n';
	USART3->ICR |= USART_ICR_TCCF;
}


/* Write I2C2 Command */
void SSD1306_WRITECOMMAND(uint8_t command){
	ssd1306_I2C_Write(SSD1306_I2C_ADDR, 0x00,  command);
}



/* Write I2C2 Data */
void ssd1306_I2C_Write(uint8_t address, uint8_t reg, uint8_t data) {

	I2C2->CR2 &= ~((0x7F << 16) | (0x3FF << 0));
	
	I2C2->CR2 |= (address << I2C_CR2_SADD_Pos);		// Set the L3GD20 slave address = slvAddr
	I2C2->CR2 |= (0x2  << I2C_CR2_NBYTES_Pos); 		// Set the number of bytes to transmit = noOfBits
	I2C2->CR2 &= ~(I2C_CR2_RD_WRN_Msk);        		// Set the RD_WRN to write operation
	I2C2->CR2 |= (I2C_CR2_START_Msk);          		// Set START bit
	
	
	
	// Wait until TXIS or NACKF flags are set
	while(1) {
		// Continue if TXIS flag is set
		if (I2C2->ISR & I2C_ISR_TXIS) {
			I2C2->TXDR = reg;
			break;
		}
		// Light ORANGE LED if NACKF flag is set (slave didn't respond)
		if (I2C2->ISR & I2C_ISR_NACKF) {
		}
	}
	
	
	// Wait again until TXIS or NACKF flags are set (2)
	while(1) {
		// Continue if TXIS flag is set
		if (I2C2->ISR & I2C_ISR_TXIS) {
			I2C2->TXDR = data;
			break;
		}
		// Light ORANGE LED if NACKF flag is set (slave didn't respond)
		if (I2C2->ISR & I2C_ISR_NACKF) {
		}
	}

}


/* Fill the Display Buffer */
void SSD1306_Fill(uint8_t color) {
	memset(SSD1306_Buffer, (color == 0) ? 0x00 : 0xFF, sizeof(SSD1306_Buffer));
}






/* Update Screen */
void SSD1306_UpdateScreen(void) {
	uint8_t m;
	for (m = 0; m < 8; m++) {
		rwCnt = m;
		SSD1306_WRITECOMMAND(0XB0+m);	// Set Page Address Of The Display
		SSD1306_WRITECOMMAND(0x00);		//Set Low Column Address Of The Display
		SSD1306_WRITECOMMAND(0x10);		//Set High Column Address Of The Display
		ssd1306_I2C_WriteMulti(SSD1306_I2C_ADDR, 0x40, &SSD1306_Buffer[SSD1306_WIDTH * m], 127);	//Write The Buffer Data To Display
	}
}


/* Write The Buffer Data To Display */
void ssd1306_I2C_WriteMulti(uint8_t address, uint8_t reg, uint8_t* data, uint16_t count) {
	uint8_t j;
	// Clear the NBYTES and SADD bit fields
	I2C2->CR2 &= ~((0x7F << 16) | (0x3FF << 0));
	
	I2C2->CR2 |= (address << I2C_CR2_SADD_Pos);		// Set the L3GD20 slave address = slvAddr
	I2C2->CR2 |= (count  << I2C_CR2_NBYTES_Pos);  	// Set the number of bytes to transmit = noOfBits
	I2C2->CR2 &= ~(I2C_CR2_RD_WRN_Msk);        		// Set the RD_WRN to write operation
	I2C2->CR2 |= (I2C_CR2_START_Msk);          		// Set START bit
	
	// Wait until TXIS or NACKF flags are set
	while(1) {
		// Continue if TXIS flag is set
		if (I2C2->ISR & I2C_ISR_TXIS) {
			I2C2->TXDR = reg;
			break;
		}
		// Light ORANGE LED if NACKF flag is set (slave didn't respond)
		if (I2C2->ISR & I2C_ISR_NACKF) {
		}
	}
	//GPIOC->ODR &= ~(1<<6);
	for(j=0;j<count-1;j++){
		// Wait again until TXIS or NACKF flags are set (2)
		while(1) {
			// Continue if TXIS flag is set
			if (I2C2->ISR & I2C_ISR_TXIS) {
				I2C2->TXDR = data[count - 1 - j];	//Write The Data To I2C2
				break;
			}
			// Light ORANGE LED if NACKF flag is set (slave didn't respond)
			if (I2C2->ISR & I2C_ISR_NACKF) {
			}
		}
	}
}


/*Write I2C2 Data*/
void ssd1306_I2C_WriteData(uint8_t data){
	transmit_Data_I2C2(SSD1306_I2C_ADDR, 0x1, data);
}


/* Transmit I2C2 Data */
void transmit_Data_I2C2(uint8_t slvAddr, uint8_t noOfBits, uint8_t txdrData){
	// Clear the NBYTES and SADD bit fields
	I2C2->CR2 &= ~((0x7F << 16) | (0x3FF << 0));
	
	I2C2->CR2 |= (slvAddr << I2C_CR2_SADD_Pos);   	// Set the L3GD20 slave address = slvAddr
	I2C2->CR2 |= (noOfBits  << I2C_CR2_NBYTES_Pos); // Set the number of bytes to transmit = noOfBits
	I2C2->CR2 &= ~(I2C_CR2_RD_WRN_Msk);        			// Set the RD_WRN to write operation
	I2C2->CR2 |= (I2C_CR2_START_Msk);          			// Set START bit

	// Wait until TXIS or NACKF flags are set
	while(1) {
		// Continue if TXIS flag is set
		if ((I2C2->ISR & I2C_ISR_TXIS)) {
			I2C2->TXDR = txdrData; // Set I2C2->TXDR = txdrData
			//UART3_SendStr("DATA SENT");
			break;
		}
		
		// Light ORANGE LED if NACKF flag is set (slave didn't respond)
		if ((I2C2->ISR & I2C_ISR_NACKF)) {
			//UART3_SendStr("NACKF");
			continue;
		}
	}
}


/* Draw Pixel On Display */
void SSD1306_DrawPixel(uint16_t x, uint16_t y, uint8_t color) {
	if (
		x >= SSD1306_WIDTH ||
		y >= SSD1306_HEIGHT
	) {
		/* Error */
		return;
	}
	
	/* Set color */
	if (color == 1) {
		SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
	} else {
		SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
	}
}


/* Draw Circle On The Display */
void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t c) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

    SSD1306_DrawPixel(x0, y0 + r, c);
    SSD1306_DrawPixel(x0, y0 - r, c);
    SSD1306_DrawPixel(x0 + r, y0, c);
    SSD1306_DrawPixel(x0 - r, y0, c);
    SSD1306_DrawLine(x0 - r, y0, x0 + r, y0, c);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        SSD1306_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, c);
        SSD1306_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, c);

        SSD1306_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, c);
        SSD1306_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, c);
    }
}


/* Draw Line On The Display */
void SSD1306_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t c) {
	int16_t dx, dy, sx, sy, err, e2, i, tmp; 
	
	/* Check for overflow */
	if (x0 >= SSD1306_WIDTH) {
		x0 = SSD1306_WIDTH - 1;
	}
	if (x1 >= SSD1306_WIDTH) {
		x1 = SSD1306_WIDTH - 1;
	}
	if (y0 >= SSD1306_HEIGHT) {
		y0 = SSD1306_HEIGHT - 1;
	}
	if (y1 >= SSD1306_HEIGHT) {
		y1 = SSD1306_HEIGHT - 1;
	}
	
	dx = (x0 < x1) ? (x1 - x0) : (x0 - x1); 
	dy = (y0 < y1) ? (y1 - y0) : (y0 - y1); 
	sx = (x0 < x1) ? 1 : -1; 
	sy = (y0 < y1) ? 1 : -1; 
	err = ((dx > dy) ? dx : -dy) / 2; 

	if (dx == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}
		
		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}
		
		/* Vertical line */
		for (i = y0; i <= y1; i++) {
			SSD1306_DrawPixel(x0, i, c);
		}
		
		/* Return from function */
		return;
	}
	
	if (dy == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}
		
		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}
		
		/* Horizontal line */
		for (i = x0; i <= x1; i++) {
			SSD1306_DrawPixel(i, y0, c);
		}
		
		/* Return from function */
		return;
	}
	
	while (1) {
		SSD1306_DrawPixel(x0, y0, c);
		if (x0 == x1 && y0 == y1) {
			break;
		}
		e2 = err; 
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		} 
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		} 
	}
}


/* Set Cursor To XY In Display */
void SSD1306_GotoXY(uint16_t x, uint16_t y) {
	/* Set write pointers */
	SSD1306.CurrentX = x;
	SSD1306.CurrentY = y;
}



/* Print Charector On Display */
char SSD1306_Putc(char ch, FontDef_t* Font, uint8_t color) {
	uint32_t i, b, j;
	Font = &Font_7x10;
	/* Check available space in LCD */
	if (
		SSD1306_WIDTH <= (SSD1306.CurrentX + Font->FontWidth) ||
		SSD1306_HEIGHT <= (SSD1306.CurrentY + Font->FontHeight)
	) {
		/* Error */
		return 0;
	}
	
	/* Go through font */
	for (i = 0; i < Font->FontHeight; i++) {
		b = Font->data[(ch - 32) * Font->FontHeight + i];
		for (j = 0; j < Font->FontWidth; j++) {
			if ((b << j) & 0x8000) {
				SSD1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), color);
			} else {
				SSD1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), ~color);
			}
		}
	}
	
	/* Increase pointer */
	SSD1306.CurrentX += Font->FontWidth;
	
	/* Return character written */
	return ch;
}

/* Print String On Display */
char SSD1306_Puts(char* str, FontDef_t* Font, uint8_t color) {
	/* Write characters */
	while (*str) {
		/* Write character by character */
		if (SSD1306_Putc(*str, Font, color) != *str) {
			/* Return error */
			return *str;
		}
		
		/* Increase string pointer */
		str++;
	}
	
	/* Everything OK, zero should be returned */
	return *str;
}