/* ===============================================================
 * Lab 8.2 - PingPong  (C89 Compatible Version)
 *
 * Notes:
 * 1. Fixes C89 issue (#268): all local variable declarations
 *    appear at the beginning of a block.
 * 2. Improves paddle smoothness and prevents ADC “jumping”
 *    by applying speed limiting logic (MAX_PADDLE_SPEED).
 * =============================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Draw2D.h" 

// ==================== LCD Settings ====================
#define LCD_WIDTH       128
#define LCD_HEIGHT      64

// ==================== Object Sizes ====================
#define BALL_SIZE       8
#define PADDLE_WIDTH    16
#define PADDLE_HEIGHT   8
#define OBSTACLE_WIDTH  16
#define OBSTACLE_HEIGHT 8

// ==================== Game Settings ====================
#define BALL_SPEED      3
#define VR_CHANNEL      7
#define GAME_DELAY_TIME 100000      // 0.1s per frame (SysTickDelay units)

#define PADDLE_Y        (LCD_HEIGHT - PADDLE_HEIGHT - 1)
#define OBSTACLE_X      56
#define OBSTACLE_Y      16

// Maximum paddle change per frame (anti-jitter)
#define MAX_PADDLE_SPEED 8 

// ==================== Game State ====================
// 0 = Idle (waiting to start)
// 1 = Playing
// 2 = Game Over
volatile uint8_t game_state = 0;

int16_t ball_x, ball_y;
int16_t ball_dx, ball_dy;
int16_t paddle_x;

// Previous frame positions
int16_t old_ball_x, old_ball_y;
int16_t old_paddle_x;

// ==================== External Keypad ====================
extern void OpenKeyPad(void);
extern uint8_t ScanKey(void);

// ==================== Function Prototypes ====================
void Init_ADC(void);
void Init_Buzzer(void);
uint32_t Read_VR(void);
void Update_Game_Logic(void);
void Draw_Game_Scene(void);
void Beep_Short(void);
void Check_Start_Key(void);
void Fill_Rect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color);

// ==================== Main Program ====================
int main(void)
{
    uint32_t vr_val;
    int16_t target_x;
    int16_t diff;

    SYS_Init();
    
    OpenKeyPad();
    Init_ADC();     
    Init_Buzzer();  
    init_LCD();
    clear_LCD(); 

    // Initialize paddle and ball positions
    paddle_x = (LCD_WIDTH - PADDLE_WIDTH) / 2;
    old_paddle_x = paddle_x;
    
    ball_x = (LCD_WIDTH - BALL_SIZE) / 2;
    ball_y = (LCD_HEIGHT - BALL_SIZE) / 2;
    old_ball_x = ball_x;
    old_ball_y = ball_y;

    // Draw the fixed obstacle at the start
    Fill_Rect(OBSTACLE_X, OBSTACLE_Y,
              OBSTACLE_X + OBSTACLE_WIDTH,
              OBSTACLE_Y + OBSTACLE_HEIGHT, 1);
    
    while (1) {

        // ========= 1. Read ADC (Potentiometer) =========
        vr_val = Read_VR();

        // Convert VR (0~4095) to paddle X position
        target_x = (vr_val * (LCD_WIDTH - PADDLE_WIDTH)) / 4095;
        
        old_paddle_x = paddle_x;

        // ========= 2. Smooth paddle movement =========
        diff = target_x - paddle_x;
        
        // Limit maximum paddle movement per frame
        if (abs(diff) > MAX_PADDLE_SPEED) {
            if (diff > 0)
                paddle_x += MAX_PADDLE_SPEED;
            else
                paddle_x -= MAX_PADDLE_SPEED;
        } 
        else {
            paddle_x = target_x;
        }

        // ========= 3. Game State Machine =========
        switch (game_state) {

            case 0: // Idle
            case 2: // Game Over screen
                Check_Start_Key();
                break;
                
            case 1: // Playing
                old_ball_x = ball_x;
                old_ball_y = ball_y;
                Update_Game_Logic();
                break;
        }

        // ========= 4. Render Scene =========
        Draw_Game_Scene();
        
        // ========= 5. Frame Delay =========
        CLK_SysTickDelay(GAME_DELAY_TIME); 
    }
}

// ==================== Draw a Filled Rectangle ====================
void Fill_Rect(int16_t x1, int16_t y1,
               int16_t x2, int16_t y2,
               uint8_t color)
{
    int16_t i, j;
    for (i = x1; i < x2; i++) {
        for (j = y1; j < y2; j++) {
            draw_Pixel(i, j, color, color);
        }
    }
}

// ==================== Drawing the Entire Frame ====================
void Draw_Game_Scene(void)
{
    // Clear previous paddle and ball
    Fill_Rect(old_paddle_x, PADDLE_Y,
              old_paddle_x + PADDLE_WIDTH,
              PADDLE_Y + PADDLE_HEIGHT, 0);

    Fill_Rect(old_ball_x, old_ball_y,
              old_ball_x + BALL_SIZE,
              old_ball_y + BALL_SIZE, 0);

    // Draw obstacle (hide it when game over)
    if (game_state == 2) {
        Fill_Rect(OBSTACLE_X, OBSTACLE_Y,
                  OBSTACLE_X + OBSTACLE_WIDTH,
                  OBSTACLE_Y + OBSTACLE_HEIGHT, 0);
    } 
    else {
        Fill_Rect(OBSTACLE_X, OBSTACLE_Y,
                  OBSTACLE_X + OBSTACLE_WIDTH,
                  OBSTACLE_Y + OBSTACLE_HEIGHT, 1);
    }

    // Draw paddle (not shown during Game Over)
    if (game_state != 2) {
        Fill_Rect(paddle_x, PADDLE_Y,
                  paddle_x + PADDLE_WIDTH,
                  PADDLE_Y + PADDLE_HEIGHT, 1);
    }

    // Draw ball (only during gameplay)
    if (game_state == 1) {
        Fill_Rect(ball_x, ball_y,
                  ball_x + BALL_SIZE,
                  ball_y + BALL_SIZE, 1);
    }
    
    // Draw Game Over text
    if (game_state == 2) {
        printS(30, 24, "GAME OVER");
    }
}

// ==================== Start Game on Key Press ====================
void Check_Start_Key(void)
{
    uint8_t key = ScanKey();

    // Accept keypad 1–9 as valid start keys
    if (key >= 1 && key <= 9) {

        if (game_state == 2) {
            clear_LCD(); 
        }

        game_state = 1; 
        
        // Reset ball to center
        ball_x = (LCD_WIDTH - BALL_SIZE) / 2;
        ball_y = (LCD_HEIGHT - BALL_SIZE) / 2;
        old_ball_x = ball_x;
        old_ball_y = ball_y;
        
        // Randomize initial direction
        srand(Read_VR() + ball_x);
        ball_dx = (rand() % 2) ? BALL_SPEED : -BALL_SPEED;
        ball_dy = -BALL_SPEED; 
        
        // Small delay to avoid bouncing
        CLK_SysTickDelay(500000); 
    }
}

// ==================== Game Logic Update ====================
void Update_Game_Logic(void)
{
    ball_x += ball_dx;
    ball_y += ball_dy;

    // -------- Wall collision (Left/Right) --------
    if (ball_x <= 0) { 
        ball_x = 0;
        ball_dx = abs(ball_dx);
    }
    else if (ball_x_
