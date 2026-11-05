/*
 * LCD_GFX.c
 *
 * Created: 9/20/2021 6:54:25 PM
 *  Author: You
 */ 

#include "LCD_GFX.h"
#include "ST7735.h"

/******************************************************************************
* Local Functions
******************************************************************************/

static void LCD_drawPixelClipped(int16_t x, int16_t y, uint16_t color)
{
    if (x >= 0 && x < LCD_WIDTH &&
        y >= 0 && y < LCD_HEIGHT) {
        LCD_drawPixel((uint8_t)x, (uint8_t)y, color);
    }
}



/******************************************************************************
* Global Functions
******************************************************************************/

/**************************************************************************//**
* @fn			uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
* @brief		Convert RGB888 value to RGB565 16-bit color data
* @note
*****************************************************************************/
uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
	return ((((31*(red+4))/255)<<11) | (((63*(green+2))/255)<<5) | ((31*(blue+4))/255));
}

/**************************************************************************//**
* @fn			void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color)
* @brief		Draw a single pixel of 16-bit rgb565 color to the x & y coordinate
* @note
*****************************************************************************/
void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color) {
	LCD_setAddr(x,y,x,y);
	SPI_ControllerTx_16bit(color);
}

/**************************************************************************//**
* @fn			void LCD_drawChar(uint8_t x, uint8_t y, uint16_t character, uint16_t fColor, uint16_t bColor)
* @brief		Draw a character starting at the point with foreground and background colors
* @note
*****************************************************************************/
void LCD_drawChar(uint8_t x, uint8_t y, uint16_t character, uint16_t fColor, uint16_t bColor){
	uint16_t row = character - 0x20;		//Determine row of ASCII table starting at space
	int i, j;
	if ((LCD_WIDTH-x>7)&&(LCD_HEIGHT-y>7)){
		for(i=0;i<5;i++){
			uint8_t pixels = ASCII[row][i]; //Go through the list of pixels
			for(j=0;j<8;j++){
				if ((pixels>>j)&1==1){
					LCD_drawPixel(x+i,y+j,fColor);
				}
				else {
					LCD_drawPixel(x+i,y+j,bColor);
				}
			}
		}
	}
}


/******************************************************************************
* LAB 4 TO DO. COMPLETE THE FUNCTIONS BELOW.
* You are free to create and add any additional files, libraries, and/or
*  helper function. All code must be authentically yours.
******************************************************************************/

/**************************************************************************//**
* @fn			void LCD_drawCircle(uint8_t x0, uint8_t y0, uint8_t radius,uint16_t color)
* @brief		Draw a colored circle of set radius at coordinates
* @note
*****************************************************************************/
void LCD_drawCircle(uint8_t x0, uint8_t y0, uint8_t radius,uint16_t color)
{
	int16_t x = radius;
    int16_t y = 0;
    int16_t err = 1 - x;

    while (x >= y) {
        // Draw the eight symmetric points of the circle, making sure the circle won't get clipped
        LCD_drawPixelClipped(x0 + x, y0 + y, color);
        LCD_drawPixelClipped(x0 + y, y0 + x, color);
        LCD_drawPixelClipped(x0 - y, y0 + x, color);
        LCD_drawPixelClipped(x0 - x, y0 + y, color);

        LCD_drawPixelClipped(x0 - x, y0 - y, color);
        LCD_drawPixelClipped(x0 - y, y0 - x, color);
        LCD_drawPixelClipped(x0 + y, y0 - x, color);
        LCD_drawPixelClipped(x0 + x, y0 - y, color);

        // Move to the next point along the circle
        y++;

        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}


/**************************************************************************//**
* @fn			void LCD_drawLine(short x0,short y0,short x1,short y1,uint16_t c)
* @brief		Draw a line from and to a point with a color
* @note
*****************************************************************************/
void LCD_drawLine(short x0,short y0,short x1,short y1,uint16_t c)
{
    //function can handle horizontal, vertical, diagonal lines, either direction 
    //avoid drawing pixel outside LCD boundaries
	short dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    short dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);

    short sx = (x0 < x1) ? 1 : -1;
    short sy = (y0 < y1) ? 1 : -1;

    short err = dx - dy;

    while (1) {
        //Draw the pixel if it is within the screen
        if (x0 >= 0 && x0 < LCD_WIDTH &&
            y0 >= 0 && y0 < LCD_HEIGHT) {
            LCD_drawPixel((uint8_t)x0, (uint8_t)y0, c);
        }

        //Stop once the endpoint is reached
        if (x0 == x1 && y0 == y1) {
            break;
        }

        short e2 = 2 * err;

        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }

}



/**************************************************************************//**
* @fn			void LCD_drawBlock(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,uint16_t color)
* @brief		Draw a colored block at coordinates
* @note
*****************************************************************************/

void LCD_drawBlock(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color)
{
    //Check that the coords valid
    if (x0 > x1 || y0 > y1) {
        return;
    }

    //Check that the rectangle starts on the screen
    if (x0 >= LCD_WIDTH || y0 >= LCD_HEIGHT) {
        return;
    }

    //Keep the rectangle within the screen boundaries
    if (x1 >= LCD_WIDTH) {
        x1 = LCD_WIDTH - 1;
    }
    if (y1 >= LCD_HEIGHT) {
        y1 = LCD_HEIGHT - 1;
    }

    //Calculate the num of pixels in the rectangle
    uint16_t numPixels = (uint16_t)(x1 - x0 + 1) *
                         (uint16_t)(y1 - y0 + 1);

    //Set rectangular region to draw
    LCD_setAddr(x0, y0, x1, y1);

    //Begin sending color data
    clear(LCD_PORT, LCD_TFT_CS);
    set(LCD_PORT, LCD_DC);

    //Fill the entire region
    for (uint16_t i = 0; i < numPixels; i++) {
        SPI_ControllerTx_16bit_stream(color);
    }

    //End communication
    set(LCD_PORT, LCD_TFT_CS);
}



/**************************************************************************//**
* @fn			void LCD_setScreen(uint16_t color)
* @brief		Draw the entire screen to a color
* @note
*****************************************************************************/
void LCD_setScreen(uint16_t color) 
{
	//Select the entire screen
    LCD_setAddr(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    //Begin sending pixel data
    clear(LCD_PORT, LCD_TFT_CS);
    set(LCD_PORT, LCD_DC);

    //Fill each pixel with the selected color
    for (uint16_t i = 0; i < LCD_SIZE; i++) {
        SPI_ControllerTx_16bit_stream(color);
    }

    //End communication
    set(LCD_PORT, LCD_TFT_CS);
}

/**************************************************************************//**
* @fn			void LCD_drawString(uint8_t x, uint8_t y, char* str, uint16_t fg, uint16_t bg)
* @brief		Draw a string starting at the point with foreground and background colors
* @note
*****************************************************************************/
void LCD_drawString(uint8_t x, uint8_t y, char* str, uint16_t fg, uint16_t bg)
{
    //each character using 5 x 8 pixel font, 1 pixel spacing 
	if (str == 0 || y > LCD_HEIGHT - 8) {
        return;
    }

    uint16_t cursorX = x;

    while (*str != '\0') {
        // Stop if another character won't fit
        if (cursorX > LCD_WIDTH - 8) {
            break;
        }

        uint8_t character = (uint8_t)*str;

        // Replace unsupported characters with '?'
        if (character < 32 || character > 126) {
            character = '?';
        }

        // Draw the character
        LCD_drawChar((uint8_t)cursorX, y, character, fg, bg);

        // Add a blank column between characters
        LCD_drawBlock((uint8_t)(cursorX + 5), y,
                      (uint8_t)(cursorX + 5), y + 7, bg);

        cursorX += 6;
        str++;
    }
}