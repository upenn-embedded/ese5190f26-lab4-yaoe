
#define F_CPU 16000000UL

#include <avr/io.h>
#include "lib/ST7735.h"
#include "lib/LCD_GFX.h"

int main(void)
{
    lcd_init();

    //Clear the display
    LCD_setScreen(BLACK);

    //Title bar
    LCD_drawBlock(0, 0, 159, 14, BLUE);
    LCD_drawString(4, 3, "LCD GRAPHICS", WHITE, BLUE);

    //Horizontal divider
    LCD_drawLine(0, 20, 159, 20, YELLOW);

    //Two rectangles representing Pong paddles
    LCD_drawBlock(20, 55, 26, 99, GREEN);
    LCD_drawBlock(133, 55, 139, 99, GREEN);

    //Circle in the middle
    LCD_drawCircle(80, 70, 24, RED);

    //Center pixel
    LCD_drawPixel(80, 70, WHITE);

    //Bottom text
    LCD_drawString(5, 113, "PART A TEST", CYAN, BLACK);

    while (1) {
    }

    return 0;
}
