#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Scankey.h"
// [??] ?????,?? include ???
#include "Draw2D.h"

// --- ?????? ---
#define LCD_W 128
#define LCD_H 64
#define BUZZER_PIN 11 
#define ADC_VR_CHANNEL 7 

// --- ?????? ---
#define BALL_SIZE 8
#define PADDLE_W 16
#define PADDLE_H 8
#define OBSTACLE_W 16
#define OBSTACLE_H 8

// --- ???? ---
typedef enum {
    STATE_INIT,
    STATE_PLAYING,
    STATE_GAMEOVER
} GameState;

// --- ?????? ---
typedef struct {
    int x, y;
    int w, h;
} Rect;

typedef struct {
    int x, y;
    int dx, dy;
    int w, h;
} BallObj;

// --- ???? ---
volatile GameState g_state = STATE_INIT;
Rect g_paddle;   
Rect g_obstacle; 
BallObj g_ball;  

// --- ???? ---
void Init_Hardware(void);
void Init_Game_Data(void);
void Update_Paddle_Pos(void);
void Beep(void);
void Draw_Game(void);

// ==========================================
//                 ?????
// ==========================================
void Init_Hardware(void)
{
    SYS_Init();
    init_LCD();
    clear_LCD();
    OpenKeyPad();

    // --- ADC ??? (VR1 @ PA7) ---
    SYS->REGWRPROT = 0x59;
    SYS->REGWRPROT = 0x16;
    SYS->REGWRPROT = 0x88;

    SYS->GPA_MFP |= (1UL << ADC_VR_CHANNEL); 

    CLK->APBCLK |= (1UL << 28);     
    CLK->CLKSEL1 &= ~(0x3UL << 2);  
    CLK->CLKDIV &= ~(0xFFUL << 16); 

    SYS->REGWRPROT = 0x00;

    PA->PMD &= ~(0x3UL << (ADC_VR_CHANNEL * 2));
    PA->OFFD |= (1UL << ADC_VR_CHANNEL);

    ADC->ADCR |= (1UL << 0);    
    ADC->ADCHER |= (1UL << ADC_VR_CHANNEL);  
    
    // --- ?????? ---
    PB->PMD &= ~(0x3UL << (BUZZER_PIN * 2)); 
    PB->PMD |= (0x1UL << (BUZZER_PIN * 2));
    PB->DOUT |= (1UL << BUZZER_PIN); 
}

// ==========================================
//               ??????
// ==========================================

void Init_Game_Data(void)
{
    int speed = 4;

    g_obstacle.x = 56; 
    g_obstacle.y = 8; 
    g_obstacle.w = OBSTACLE_W;
    g_obstacle.h = OBSTACLE_H;

    g_paddle.x = 56; 
    g_paddle.y = LCD_H - PADDLE_H; 
    g_paddle.w = PADDLE_W;
    g_paddle.h = PADDLE_H;

    g_ball.x = (LCD_W - BALL_SIZE) / 2;
    g_ball.y = (LCD_H - BALL_SIZE) / 2;
    g_ball.w = BALL_SIZE;
    g_ball.h = BALL_SIZE;
    
    g_ball.dx = (rand() % 2 == 0) ? speed : -speed;
    g_ball.dy = (rand() % 2 == 0) ? speed : -speed;
}

void Update_Paddle_Pos(void)
{
    uint32_t adc_val = 0;
    int i;
    
    // ?????
    for(i = 0; i < 8; i++) 
    {
        ADC->ADCR |= (1UL << 11); 
        while(ADC->ADCR & (1UL << 11));
        adc_val += (ADC->ADDR[ADC_VR_CHANNEL] & 0xFFF);
    }
    adc_val = adc_val / 8;
    
    // ??: 0~4095 -> 0 ~ (128 - 16)
    g_paddle.x = (adc_val * (LCD_W - PADDLE_W)) / 4096;
}

void Beep(void)
{
    PB->DOUT &= ~(1UL << BUZZER_PIN); 
    CLK_SysTickDelay(50000);
    PB->DOUT |= (1UL << BUZZER_PIN);  
}

int Check_Collision(Rect *rect, BallObj *ball)
{
    if (ball->x < rect->x + rect->w &&
        ball->x + ball->w > rect->x &&
        ball->y < rect->y + rect->h &&
        ball->y + ball->h > rect->y)
    {
        return 1;
    }
    return 0;
}

void Draw_Game(void)
{
    clear_LCD();
    
    // 1. ?? (8x8 ????)
    // ??: x0, y0, x1, y1, fgColor, bgColor
    fill_Rectangle(g_ball.x, g_ball.y, 
                   g_ball.x + BALL_SIZE - 1, g_ball.y + BALL_SIZE - 1, 
                   1, 0);
    
    // 2. ??? (16x8 ????)
    fill_Rectangle(g_paddle.x, g_paddle.y, 
                   g_paddle.x + PADDLE_W - 1, g_paddle.y + PADDLE_H - 1, 
                   1, 0);
    
    // 3. ???? (16x8 ????)
    fill_Rectangle(g_obstacle.x, g_obstacle.y, 
                   g_obstacle.x + OBSTACLE_W - 1, g_obstacle.y + OBSTACLE_H - 1, 
                   1, 0);
}

// ==========================================
//                  ???
// ==========================================
int main(void)
{
    Init_Hardware();
    srand(123); 

    while(1)
    {
        if (g_state == STATE_INIT)
        {
            Init_Game_Data();
            Draw_Game();
            
            while(ScanKey() == 0) {
                Update_Paddle_Pos(); 
                Draw_Game(); 
                CLK_SysTickDelay(50000);
            }
            g_state = STATE_PLAYING;
        }
        else if (g_state == STATE_PLAYING)
        {
            Update_Paddle_Pos();

            g_ball.x += g_ball.dx;
            g_ball.y += g_ball.dy;

            // ????
            if (g_ball.x <= 0) {
                g_ball.x = 0;
                g_ball.dx = -g_ball.dx;
            } 
            else if (g_ball.x >= LCD_W - BALL_SIZE) {
                g_ball.x = LCD_W - BALL_SIZE;
                g_ball.dx = -g_ball.dx;
            }

            if (g_ball.y <= 0) {
                g_ball.y = 0;
                g_ball.dy = -g_ball.dy;
            }

            if (g_ball.y >= LCD_H - BALL_SIZE) {
                g_state = STATE_GAMEOVER;
                CLK_SysTickDelay(200000); 
            }

            // ????
            if (Check_Collision(&g_paddle, &g_ball)) {
                g_ball.dy = -g_ball.dy;
                g_ball.y = g_paddle.y - BALL_SIZE; 
                Beep();
            }

            if (Check_Collision(&g_obstacle, &g_ball)) {
                g_ball.dy = -g_ball.dy;
            }

            Draw_Game();
            CLK_SysTickDelay(100000); 
        }
        else if (g_state == STATE_GAMEOVER)
        {
            clear_LCD();
            printS(30, 24, "GAME OVER");
            
            while(ScanKey() == 0);
            CLK_SysTickDelay(500000); 
            g_state = STATE_INIT;
        }
    }
}
