
#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h> //gives access to rand() and srand()
#include "lib/ST7735.h"
#include "lib/LCD_GFX.h"

// Game dimensions
#define FIELD_TOP 20
#define PADDLE_HEIGHT 24
#define PADDLE_STEP 2 //adjust speed of player
#define MAX_PADDLE_Y (LCD_HEIGHT - PADDLE_HEIGHT)
#define BALL_SIZE 6
#define POINTS_TO_WIN 1
#define ROUNDS_TO_WIN 1

// Joystick dead zone
#define JOY_LOW 420
#define JOY_HIGH 600

// LED pins and buzzer pins
#define PLAYER_LED PD2
#define CPU_LED PD4
#define BUZZER_PIN PD5
#define MOTOR_PIN PD3

// Wireless pins 
#define WIRELESS_UP PC1
#define WIRELESS_DOWN PC2


// Initialize ADC for joystick
void ADC_init(void)
{
    DDRC &= ~(1 << DDC0);
    ADMUX = (1 << REFS0);

    ADCSRA = (1 << ADEN) |
             (1 << ADPS2) |
             (1 << ADPS1) |
             (1 << ADPS0);
}

// Read joystick position
uint16_t ADC_read(void)
{
    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC)) {
    }

    return ADC;
}

// DDRC makes both pins inputs, PORTC leaves pull up resistors disabled
void wirelessInit(void)
{
    DDRC &= ~((1 << WIRELESS_UP) | (1 << WIRELESS_DOWN));
    PORTC &= ~((1 << WIRELESS_UP) | (1 << WIRELESS_DOWN));
}


//configs PD2 and PD4 as outputs, initially off
void LED_init(void)
{
    DDRD |= (1 << PLAYER_LED) | (1 << CPU_LED);
    PORTD &= ~((1 << PLAYER_LED) | (1 << CPU_LED));
}

//turn LED on for time that can be changed
void flashLED(uint8_t pin)
{
    for (uint8_t i = 0; i < 2; i++) {
        PORTD |= (1 << pin);
        _delay_ms(100);
        PORTD &= ~(1 << pin);
        _delay_ms(100);
    }
}

// functionality for E level 1
void winLEDAnimation(void)
{
    for (uint8_t i = 0; i < 3; i++) {
        PORTD |= (1 << PLAYER_LED);
        PORTD &= ~(1 << CPU_LED);
        _delay_ms(150);

        PORTD &= ~(1 << PLAYER_LED);
        PORTD |= (1 << CPU_LED);
        _delay_ms(150);
    }

    PORTD &= ~((1 << PLAYER_LED) | (1 << CPU_LED));
}

void buzzerInit(void)
{
    DDRD |= (1 << BUZZER_PIN);
    PORTD &= ~(1 << BUZZER_PIN);
}

void playSound(uint16_t duration_ms)
{
    PORTD |= (1 << BUZZER_PIN);

    while (duration_ms--) {
        _delay_ms(1);
    }

    PORTD &= ~(1 << BUZZER_PIN);
}

// Fulfills part E level 3, adds random LED flashing, sounds, and flashes screen red
//Has to rewrite everything after done flashing though

void randomDistraction(uint8_t playerY, uint8_t computerY,
                       int16_t ballX, int16_t ballY,
                       uint8_t playerScore, uint8_t computerScore)
{
    if (rand() % 120 == 0) {

        // Flash both LEDs and sound buzzer
        for (uint8_t i = 0; i < 2; i++) {
            PORTD |= (1 << PLAYER_LED) | (1 << CPU_LED);
            playSound(70);
            PORTD &= ~((1 << PLAYER_LED) | (1 << CPU_LED));
            _delay_ms(80);
        }

        // Flash entire screen red
        LCD_setScreen(RED);
        _delay_ms(70);

        // Restore black background
        LCD_setScreen(BLACK);

        // Restore scoreboard
        LCD_drawString(15, 3, "PLAYER 0       CPU 0", WHITE, BLACK);
        LCD_drawChar(57, 3, '0' + playerScore, WHITE, BLACK);
        LCD_drawChar(129, 3, '0' + computerScore, WHITE, BLACK);
        LCD_drawLine(0, 17, 159, 17, CYAN);

        // Restore dashed center line
        for (uint8_t y = 24; y < 120; y += 14) {
            LCD_drawBlock(79, y, 80, y + 5, BLUE);
        }

        // Restore player paddle
        LCD_drawBlock(5, playerY, 10,
                      playerY + PADDLE_HEIGHT - 1, GREEN);

        // Restore computer paddle
        LCD_drawBlock(149, computerY, 154,
                      computerY + PADDLE_HEIGHT - 1, YELLOW);

        // Restore ball
        LCD_drawBlock(ballX, ballY,
                      ballX + BALL_SIZE - 1,
                      ballY + BALL_SIZE - 1, WHITE);
    }
}




void motorInit(void)
{
    DDRD |= (1 << MOTOR_PIN);
    PORTD &= ~(1 << MOTOR_PIN);

    OCR2B = 0;
    TCCR2A = (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << CS21);
}

// duty cycle increase from 0 to 255, then from 255 to 0
// accelerate 1 sec, decelerate 1 sec
void motorAnimation(void)
{
    // Enable PWM output
    OCR2B = 0;
    TCCR2A |= (1 << COM2B1);

    // Gradually accelerate motor
    for (uint16_t duty = 0; duty <= 255; duty += 5) {
        OCR2B = (uint8_t)duty;
        _delay_ms(20);
    }

    // Gradually decelerate motor
    for (int16_t duty = 255; duty >= 0; duty -= 5) {
        OCR2B = (uint8_t)duty;
        _delay_ms(20);
    }

    // Turn off motor
    TCCR2A &= ~(1 << COM2B1);
    OCR2B = 0;
    PORTD &= ~(1 << MOTOR_PIN);
}





void resetBall(int16_t *x, int16_t *y, int16_t *vx, int16_t *vy)
{
    // Reset ball to the center
    *x = (LCD_WIDTH - BALL_SIZE) / 2;
    *y = (LCD_HEIGHT - BALL_SIZE) / 2;

    // Randomize direction and speed
    *vx = (rand() % 2 ? 1 : -1) * (2 + rand() % 2);
    *vy = (rand() % 2 ? 1 : -1) * (1 + rand() % 2);
}


int main(void)
{
    lcd_init();
    ADC_init();
    LED_init();
    buzzerInit();
    motorInit();
    wirelessInit();
    
            
    

   

    // Paddle positions
    uint8_t playerY = 62;
    uint8_t computerY = 62;
    uint8_t cpuCounter = 0;
    
    // Player scores
    uint8_t playerScore = 0;
    uint8_t computerScore = 0;
    
    // Round wins
    uint8_t playerRounds = 0;
    uint8_t computerRounds = 0;

    // Ball position and velocity
    int16_t ballX = 77;
    int16_t ballY = 61;
    int16_t ballVX = 2;
    int16_t ballVY = 1;
    
    // Initialize random ball movement
    srand(ADC_read());
    resetBall(&ballX, &ballY, &ballVX, &ballVY);
    
    // Select computer difficulty
    uint8_t cpuSpeed = 2;

    LCD_setScreen(BLACK);
    LCD_drawString(10, 30, "SELECT DIFFICULTY", WHITE, BLACK);
    LCD_drawString(10, 50, "UP=HARD DOWN=EASY", WHITE, BLACK);
    LCD_drawString(10, 70, "WAIT 3S=NORMAL", WHITE, BLACK);

    
    // difficulty selector 
    for (uint8_t i = 0; i < 60; i++) {
        uint16_t choice = ADC_read();

        if (choice > JOY_HIGH) {
            cpuSpeed = 1;
            break;
        } else if (choice < JOY_LOW) {
            cpuSpeed = 5;
            break;
        }

        _delay_ms(50);
    }

    if (cpuSpeed == 1) {
        LCD_drawString(10, 95, "HARD SELECTED", GREEN, BLACK);
    } else if (cpuSpeed == 5) {
        LCD_drawString(10, 95, "EASY SELECTED", GREEN, BLACK);
    } else {
        LCD_drawString(10, 95, "NORMAL SELECTED", GREEN, BLACK);
    }

    _delay_ms(800);

    // Clear screen
    LCD_setScreen(BLACK);

    // Scoreboard
    LCD_drawString(15, 3, "PLAYER 0       CPU 0", WHITE, BLACK);
    LCD_drawLine(0, 17, 159, 17, CYAN);

    // Dashed center line
    for (uint8_t y = 24; y < 120; y += 14) {
        LCD_drawBlock(79, y, 80, y + 5, BLUE);
    }

    // Draw paddles
    LCD_drawBlock(5, playerY, 10,
                  playerY + PADDLE_HEIGHT - 1, GREEN);

    LCD_drawBlock(149, computerY, 154,
                  computerY + PADDLE_HEIGHT - 1, YELLOW);

    // Draw initial ball
    LCD_drawBlock(ballX, ballY,
                  ballX + BALL_SIZE - 1,
                  ballY + BALL_SIZE - 1, WHITE);

    while (1)
    {
        // Read joystick
        uint16_t joy = ADC_read();
        uint8_t oldY = playerY;
        uint8_t oldComputerY = computerY;

        // Read wireless paddle commands
        uint8_t wirelessUp = (PINC & (1 << WIRELESS_UP)) != 0;
        uint8_t wirelessDown = (PINC & (1 << WIRELESS_DOWN)) != 0;

        // Move player paddle using wireless controls
        if (wirelessUp && !wirelessDown && playerY > FIELD_TOP) {
            playerY -= PADDLE_STEP;
        } else if (wirelessDown && !wirelessUp && playerY < MAX_PADDLE_Y) {
            playerY += PADDLE_STEP;
        }
            // Use joystick when no wireless button is pressed
        else if (!wirelessUp && !wirelessDown) {

            // Move player paddle down
            if (joy < JOY_LOW && playerY < MAX_PADDLE_Y) {
                playerY += PADDLE_STEP;
            }
                // Move player paddle up
            else if (joy > JOY_HIGH && playerY > FIELD_TOP) {
                playerY -= PADDLE_STEP;
            }
        }
        
        
        // Computer follows the ball, can change tracking with cpuCounter
        cpuCounter++;

        if (cpuCounter >= cpuSpeed) {
            cpuCounter = 0;

            int16_t ballCenter = ballY + BALL_SIZE / 2;
            int16_t paddleCenter = computerY + PADDLE_HEIGHT / 2;
            
            //adjust computer speed by adjusting how much is increased or decreased 

            if (ballCenter < paddleCenter && computerY > FIELD_TOP) {
                computerY--; 
            }
            else if (ballCenter > paddleCenter && computerY < MAX_PADDLE_Y) {
                computerY++;
            }
        }


        // Erase old ball position
        LCD_drawBlock(ballX, ballY,
                      ballX + BALL_SIZE - 1,
                      ballY + BALL_SIZE - 1, BLACK);

        // Erase old paddle if it moved
        if (playerY != oldY) {
            LCD_drawBlock(5, oldY, 10,
                          oldY + PADDLE_HEIGHT - 1, BLACK);
        }
        
        // Erase old computer paddle position
        if (computerY != oldComputerY) {
            LCD_drawBlock(149, oldComputerY, 154,
                    oldComputerY + PADDLE_HEIGHT - 1, BLACK);
        }
        

        // Update ball position
        int16_t previousBallX = ballX;
        ballX += ballVX;
        ballY += ballVY;

        // Bounce off top wall
        if (ballY <= FIELD_TOP) {
            ballY = FIELD_TOP;
            ballVY = -ballVY;
            playSound(30); 
        }

        // Bounce off bottom wall
        if (ballY >= LCD_HEIGHT - BALL_SIZE) {
            ballY = LCD_HEIGHT - BALL_SIZE;
            ballVY = -ballVY;
            playSound(30); 
        }
        

        // Player paddle collision (x = 5 to 10)
        if (ballVX < 0 &&
                previousBallX > 11 && ballX <= 11 &&
                ballY + BALL_SIZE - 1 >= playerY &&
                ballY <= playerY + PADDLE_HEIGHT - 1) {

            ballX = 11;
            ballVX = -ballVX;
            
            playSound(50);
        }

        // Computer paddle collision (x = 149 to 154)
        if (ballVX > 0 &&
                previousBallX + BALL_SIZE - 1 < 149 &&
                ballX + BALL_SIZE - 1 >= 149 &&
                ballY + BALL_SIZE - 1 >= computerY &&
                ballY <= computerY + PADDLE_HEIGHT - 1) {

            ballX = 149 - BALL_SIZE;
            ballVX = -ballVX;
            
            playSound(50);
        }

        // Scoring when a paddle misses
        if (ballX <= 0 || ballX >= LCD_WIDTH - BALL_SIZE) {

            if (ballX <= 0) {
                computerScore++;
                flashLED(CPU_LED);
            } else {
                playerScore++;
                flashLED(PLAYER_LED);
            }
            
            playSound(100);

            // Update scoreboard
            LCD_drawChar(57, 3, '0' + playerScore, WHITE, BLACK);
            LCD_drawChar(129, 3, '0' + computerScore, WHITE, BLACK);

            // Check if a round has been won
            if (playerScore >= POINTS_TO_WIN ||
                    computerScore >= POINTS_TO_WIN) {

                LCD_setScreen(BLACK);

                if (playerScore >= POINTS_TO_WIN) {
                    playerRounds++;
                    LCD_drawString(20, 45, "PLAYER WINS ROUND",
                            YELLOW, BLACK);
                    
                    
                } else {
                    computerRounds++;
                    LCD_drawString(20, 45, "CPU WINS ROUND",
                            YELLOW, BLACK);
                    
                    flashLED(CPU_LED);
                }

                // Display round totals
                LCD_drawString(34, 65, "ROUNDS", WHITE, BLACK);
                LCD_drawChar(65, 80, '0' + playerRounds, WHITE, BLACK);
                LCD_drawChar(77, 80, '-', WHITE, BLACK);
                LCD_drawChar(89, 80, '0' + computerRounds, WHITE, BLACK);

                _delay_ms(1500);

                // Check if the match has been won
                if (playerRounds >= ROUNDS_TO_WIN ||
                        computerRounds >= ROUNDS_TO_WIN) {

                    LCD_setScreen(BLACK);

                    if (playerRounds >= ROUNDS_TO_WIN) {
                        LCD_drawString(20, 55, "PLAYER WINS MATCH",
                                GREEN, BLACK);
                        
                        motorAnimation();
                    } else {
                        LCD_drawString(20, 55, "CPU WINS MATCH",
                                YELLOW, BLACK);
                        
                        _delay_ms(2000);
                    }
                    
                    winLEDAnimation();

                    _delay_ms(2000);

                    playerRounds = 0;
                    computerRounds = 0;
                }

                // Reset scores and paddle positions
                playerScore = 0;
                computerScore = 0;
                playerY = 62;
                computerY = 62;
                cpuCounter = 0;

                // Restore scoreboard and field
                LCD_setScreen(BLACK);
                LCD_drawString(15, 3, "PLAYER 0       CPU 0",
                        WHITE, BLACK);
                LCD_drawLine(0, 17, 159, 17, CYAN);

                resetBall(&ballX, &ballY, &ballVX, &ballVY);
            } else {
                // Reset ball for next point
                resetBall(&ballX, &ballY, &ballVX, &ballVY);
                _delay_ms(400);
            }
        }



        

        // Restore dashed center line
        for (uint8_t y = 24; y < 120; y += 14) {
            LCD_drawBlock(79, y, 80, y + 5, BLUE);
        }

        // Redraw paddles
        LCD_drawBlock(5, playerY, 10,
                      playerY + PADDLE_HEIGHT - 1, GREEN);

        LCD_drawBlock(149, computerY, 154,
                      computerY + PADDLE_HEIGHT - 1, YELLOW);

        // Draw ball at its new position
        LCD_drawBlock(ballX, ballY,
                      ballX + BALL_SIZE - 1,
                      ballY + BALL_SIZE - 1, WHITE);
        
        // Random distractions during gameplay
        randomDistraction(playerY, computerY, ballX, ballY,
                playerScore, computerScore);


        // Delay between updates
        _delay_ms(30);
    }

    return 0;
}


