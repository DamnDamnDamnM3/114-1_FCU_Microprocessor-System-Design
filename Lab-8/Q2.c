#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Scankey.h"

// --- LCD / Hardware constants ---
#define LCD_W 128
#define LCD_H 64
#define BUZZER_PIN 11 

// VR1 is connected to ADC7 (PA7)
#define ADC_VR_CHANNEL 7 

// --- Object sizes ---
#define BALL_SIZE 8
#define PADDLE_W 16
#define PADDLE_H 8
#define OBSTACLE_W 16
#define OBSTACLE_H 8

// --- Game State Enum ---
typedef enum {
    STATE_INIT,
    STATE_PLAYING,
    STATE_GAMEOVER
} GameState;

// --- Structures ---
typedef struct {
    int x, y;
    int w, h;
} Rect;

typedef struct {
    int x, y;
    int dx, dy;
    int w, h;
} BallObj;

// --- Global game objects ---
volatile GameState g_state = STATE_INIT;
Rect g_paddle;   
Rect g_obstacle; 
BallObj g_ball;

// --- Simple bitmaps for drawing ---
unsigned char bmp_ball[8] = {
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};

unsigned char bmp_block[16] = {
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};

// --- Function prototypes ---
void Init_Hardware(void);
void Init_Game_Data(void);
void Update_Paddle_Pos(void);
void Beep(void);
void Draw_Game(void);

// ==========================================================
//                       Hardware Init
// ==========================================================
void Init_Hardware(void)
{
    SYS_Init();
    init_LCD();
    clear_LCD();
    OpenKeyPad();

    // ------------------------------------------------------
    //               Configure ADC for VR input
    // ------------------------------------------------------

    // Unlock protected registers
    SYS->REGWRPROT = 0x59;
    SYS->REGWRPROT = 0x16;
    SYS->REGWRPROT = 0x88;

    // Set PA7 to ADC7 (multi-function pin)
    SYS->GPA_MFP |= (1UL << ADC_VR_CHANNEL); 

    // Enable ADC clock
    CLK->APBCLK |= (1UL << 28);

    // Select ADC clock = external crystal (HXT)
    CLK->CLKSEL1 &= ~(0x3UL << 2);

    // ADC clock divider = 1
    CLK->CLKDIV &= ~(0xFFUL << 16);

    // Lock protected registers
    SYS->REGWRPROT = 0x00;

    // Configure PA7 as input mode
    PA->PMD &= ~(0x3UL << (ADC_VR_CHANNEL * 2));

    // Disable digital input path on PA7
    PA->OFFD |= (1UL << ADC_VR_CHANNEL);

    // Enable ADC
    ADC->ADCR |= (1UL << 0);

    // Enable channel 7
    ADC->ADCHER |= (1UL << ADC_VR_CHANNEL);  

    // ------------------------------------------------------
    //               Configure Buzzer (PB11)
    // ------------------------------------------------------
    PB->PMD &= ~(0x3UL << (BUZZER_PIN * 2));
    PB->PMD |=  (0x1UL << (BUZZER_PIN * 2));
    PB->DOUT |= (1UL << BUZZER_PIN); 
}

// ==========================================================
//                   Initialize Game Objects
// ==========================================================
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

    g_ball.dx = (rand()%2 == 0) ? speed : -speed;
    g_ball.dy = (rand()%2 == 0) ? speed : -speed;
}

// ==========================================================
//               Read VR and Update Paddle X-Position
// ==========================================================
void Update_Paddle_Pos(void)
{
    uint32_t adc_val = 0;
    int i;

    // Average 8 samples to smooth the paddle movement
    for(i = 0; i < 8; i++) 
    {
        ADC->ADCR |= (1UL << 11);   // Start conversion
        while(ADC->ADCR & (1UL << 11));  // Wait for completion
        
        adc_val += (ADC->ADDR[ADC_VR_CHANNEL] & 0xFFF);
    }
    
    adc_val /= 8;

    // Convert ADC(0~4095) to paddle X (0~112)
    g_paddle.x = (adc_val * (LCD_W - PADDLE_W)) / 4096;
}

// ==========================================================
//                      Buzzer Beep
// ==========================================================
void Beep(void)
{
    PB->DOUT &= ~(1UL << BUZZER_PIN); 
    CLK_SysTickDelay(50000);
    PB->DOUT |=  (1UL << BUZZER_PIN);  
}

// ==========================================================
//                 Rectangle–Ball Collision Check
// ==========================================================
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

// ==========================================================
//                        Draw Scene
// ==========================================================
void Draw_Game(void)
{
    clear_LCD();
    draw_Bmp8x8(g_ball.x, g_ball.y, 1, 0, bmp_ball);
    draw_Bmp16x8(g_paddle.x, g_paddle.y, 1, 0, bmp_block);
    draw_Bmp16x8(g_obstacle.x, g_obstacle.y, 1, 0, bmp_block);
}

// ==========================================================
//                        Main Loop
// ==========================================================
int main(void)
{
    Init_Hardware();
    srand(123);

    while(1)
    {
        // -------------------- INIT STATE --------------------
        if (g_state == STATE_INIT)
        {
            Init_Game_Data();
            Draw_Game();
            
            // Wait for keypad press before starting the game
            while(ScanKey() == 0) {
                Update_Paddle_Pos();
                Draw_Game();
                CLK_SysTickDelay(50000);
            }
            g_state = STATE_PLAYING;
        }

        // -------------------- PLAY STATE --------------------
        else if (g_state == STATE_PLAYING)
        {
            Update_Paddle_Pos();

            // Move ball
            g_ball.x += g_ball.dx;
            g_ball.y += g_ball.dy;

            // Bounce off left/right walls
            if (g_ball.x <= 0) {
                g_ball.x = 0;
                g_ball.dx = -g_ball.dx;
            }
            else if (g_ball.x >= LCD_W - BALL_SIZE) {
                g_ball.x = LCD_W - BALL_SIZE;
                g_ball.dx = -g_ball.dx;
            }

            // Bounce off top
            if (g_ball.y <= 0) {
                g_ball.y = 0;
                g_ball.dy = -g_ball.dy;
            }

            // If ball goes below screen → Game Over
            if (g_ball.y >= LCD_H - BALL_SIZE) {
                g_state = STATE_GAMEOVER;
                CLK_SysTickDelay(200000);
            }

            // Paddle collision
            if (Check_Collision(&g_paddle, &g_ball)) {
                g_ball.dy = -g_ball.dy;
                g_ball.y = g_paddle.y - BALL_SIZE;
                Beep();
            }

            // Obstacle collision
            if (Check_Collision(&g_obstacle, &g_ball)) {
                g_ball.dy = -g_ball.dy;
            }

            Draw_Game();
            CLK_SysTickDelay(100000);
        }

        // -------------------- GAME OVER --------------------
        else if (g_state == STATE_GAMEOVER)
        {
            clear_LCD();
            printS(30, 24, "GAME OVER");
            
            // Wait for key to restart
            while(ScanKey() == 0);
            CLK_SysTickDelay(500000);
            
            g_state = STATE_INIT;
        }
    }
}
