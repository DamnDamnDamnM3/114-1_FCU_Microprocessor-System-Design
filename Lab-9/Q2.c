#include <stdio.h>
#include <stdlib.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Seven_Segment.h" 

// ---------------- ???? ----------------
#define MAX_SNAKE_LEN 100 
#define GRID_W      64    
#define GRID_H      32    

#define ADC_CENTER  2048
#define ADC_THRES   700 

typedef enum {
    DIR_STOP = 0,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

// ---------------- ?????? ----------------
extern void OpenSevenSegment(void);
extern void ShowSevenSegment(uint8_t no, uint8_t number);
extern void CloseSevenSegment(void);

// ---------------- ???? ----------------
volatile uint8_t u8ADF;
volatile uint16_t X_ADC, Y_ADC; 
volatile uint8_t  B_Button;

int8_t snake_x[MAX_SNAKE_LEN];
int8_t snake_y[MAX_SNAKE_LEN];
uint16_t current_len = 16; 

int8_t fruit_x = -1;
int8_t fruit_y = -1;

int score = 0;
int game_over = 0;

Direction current_dir = DIR_RIGHT; 
Direction next_dir = DIR_RIGHT;    

// ---------------- [????] ?????? ----------------
void Game_Delay_and_Scan(uint32_t delay_cnt)
{
    // delay_cnt ?? 200000 (us) ??? 0.2 ?
    // ?????????????,??????????
    
    // ?????? (4??) ??? 4000us (4ms) => ?? 250Hz (???)
    // ????? = ????? / 4000
    uint32_t loop_count = delay_cnt / 4000; 
    
    int i;
    int digit0, digit1, digit2, digit3;
    
    // ???????,??????????
    digit0 = score % 10;
    digit1 = (score / 10) % 10;
    digit2 = (score / 100) % 10;
    digit3 = (score / 1000) % 10;

    for (i = 0; i < loop_count; i++) {
        
        // --- Digit 0 (??) ---
        ShowSevenSegment(0, digit0); 
        CLK_SysTickDelay(1000);      // ? 1ms
        CloseSevenSegment();         // ?

        // --- Digit 1 (??) ---
        if (score >= 10) ShowSevenSegment(1, digit1);
        CLK_SysTickDelay(1000);      // [??] ????????,????
        CloseSevenSegment();

        // --- Digit 2 (??) ---
        if (score >= 100) ShowSevenSegment(2, digit2);
        CLK_SysTickDelay(1000);      
        CloseSevenSegment();
        
        // --- Digit 3 (??) ---
        if (score >= 1000) ShowSevenSegment(3, digit3);
        CLK_SysTickDelay(1000);      
        CloseSevenSegment();
    }
}

// ---------------- ADC ???? ----------------
void ADC_IRQHandler(void)
{
    uint32_t u32Flag;
    u32Flag = ADC_GET_INT_FLAG(ADC, ADC_ADF_INT);
    if(u32Flag & ADC_ADF_INT) {
        X_ADC = ADC_GET_CONVERSION_DATA(ADC, 0);
        Y_ADC = ADC_GET_CONVERSION_DATA(ADC, 1);
    }
    ADC_CLR_INT_FLAG(ADC, u32Flag);
}

void Init_ADC(void)
{
    ADC_Open(ADC, ADC_INPUT_MODE, ADC_OPERATION_MODE, ADC_CHANNEL_MASK );
    ADC_POWER_ON(ADC);
    ADC_EnableInt(ADC, ADC_ADF_INT);
    NVIC_EnableIRQ(ADC_IRQn);
    ADC_START_CONV(ADC);    
}

// ---------------- ??????? ----------------

void draw_Snake_Block(int8_t gx, int8_t gy, uint16_t color)
{
    int16_t px = gx * 2;
    int16_t py = gy * 2;
    uint16_t bg = (color == 0) ? 1 : 0; 
    
    draw_Pixel(px,     py,     color, bg);
    draw_Pixel(px + 1, py,     color, bg);
    draw_Pixel(px,     py + 1, color, bg);
    draw_Pixel(px + 1, py + 1, color, bg);
}

void spawn_Fruit(void)
{
    int valid = 0;
    int i;
    int8_t rx, ry;

    srand(X_ADC + Y_ADC + score);

    while (!valid) {
        valid = 1;
        rx = rand() % GRID_W;
        ry = rand() % GRID_H;

        for (i = 0; i < current_len; i++) {
            if (snake_x[i] == rx && snake_y[i] == ry) {
                valid = 0; 
                break;
            }
        }
    }
    fruit_x = rx;
    fruit_y = ry;
    draw_Snake_Block(fruit_x, fruit_y, 1);
}

void init_Game(void)
{
    int i;
    int8_t start_x = (GRID_W / 2) - (16 / 2); 
    int8_t start_y = (GRID_H / 2);                   
    
    current_dir = DIR_RIGHT;
    next_dir = DIR_RIGHT; 
    current_len = 16; 
    score = 0;
    game_over = 0;

    clear_LCD();
    
    for(i = 0; i < current_len; i++) {
        snake_x[i] = start_x + i; 
        snake_y[i] = start_y;
        draw_Snake_Block(snake_x[i], snake_y[i], 1); 
    }

    spawn_Fruit();
}

void update_Joystick_Logic(void)
{
    int32_t dx = (int32_t)X_ADC - ADC_CENTER;
    int32_t dy = (int32_t)Y_ADC - ADC_CENTER;
    uint32_t dist_sq = (dx*dx) + (dy*dy);
    uint32_t thres_sq = ADC_THRES * ADC_THRES;
    Direction req_dir = DIR_STOP;

    if (dist_sq < thres_sq) return; 

    if (abs(dx) > abs(dy)) {
        if (dx > 0) req_dir = DIR_RIGHT;
        else req_dir = DIR_LEFT;
    } else {
        if (dy > 0) req_dir = DIR_DOWN; 
        else req_dir = DIR_UP;
    }

    if (current_dir == DIR_RIGHT && req_dir == DIR_LEFT) return;
    if (current_dir == DIR_LEFT && req_dir == DIR_RIGHT) return;
    if (current_dir == DIR_UP && req_dir == DIR_DOWN) return;
    if (current_dir == DIR_DOWN && req_dir == DIR_UP) return;

    next_dir = req_dir;
}

// ---------------- ??? ----------------
int32_t main (void)
{
    int i; 
    int8_t head_x, head_y, new_x, new_y;
    int valid_move;

    SYS_Init();
    SYS_UnlockReg();
    CLK->APBCLK |= (1 << 15) | (1 << 28); 
    SYS->GPD_MFP |= (SYS_GPD_MFP_PD8_SPI3_SS0 | SYS_GPD_MFP_PD9_SPI3_CLK | SYS_GPD_MFP_PD11_SPI3_MOSI0);
    SYS->GPA_MFP |= (SYS_GPA_MFP_PA0_ADC0 | SYS_GPA_MFP_PA1_ADC1);
    SYS_LockReg();

    GPIO_SetMode(PD, BIT12 | BIT14, GPIO_MODE_OUTPUT); 
    PD14 = 1; 
    PD12 = 1; CLK_SysTickDelay(10000);
    PD12 = 0; CLK_SysTickDelay(10000);
    PD12 = 1; CLK_SysTickDelay(100000); 

    Init_ADC(); 
    OpenSevenSegment(); 
    init_LCD();
    
    // [????] ? Reset ?? PC0 ??????? (Quasi-bidirectional)
    // ??????????,??????????????
    GPIO_SetMode(PC, BIT0, GPIO_MODE_QUASI); 

    init_Game();

    while(1) {
        // ??? (Reset)
        if (PC0 == 0) { 
            init_Game();
            // ?????,???????????,??????
            Game_Delay_and_Scan(500000); 
        }

        if (game_over) {
            Game_Delay_and_Scan(200000);
            continue; 
        }

        update_Joystick_Logic();

        if (next_dir != DIR_STOP) {
            
            head_x = snake_x[current_len - 1];
            head_y = snake_y[current_len - 1];
            new_x = head_x;
            new_y = head_y;
            valid_move = 1;

            current_dir = next_dir;

            switch(current_dir) {
                case DIR_UP:    new_y = head_y - 1; break; 
                case DIR_DOWN:  new_y = head_y + 1; break;
                case DIR_LEFT:  new_x = head_x - 1; break;
                case DIR_RIGHT: new_x = head_x + 1; break;
                default: break;
            }

            if (new_x < 0 || new_x >= GRID_W || new_y < 0 || new_y >= GRID_H) {
                valid_move = 0;
                game_over = 1;
            }

            if (valid_move) {
                for (i = 0; i < current_len - 1; i++) { 
                    if (snake_x[i] == new_x && snake_y[i] == new_y) {
                        valid_move = 0;
                        game_over = 1; 
                        break;
                    }
                }
            }

            if (valid_move) {
                if (new_x == fruit_x && new_y == fruit_y) {
                    score += 10;
                    if (current_len < MAX_SNAKE_LEN) current_len++; 
                    
                    snake_x[current_len - 1] = new_x;
                    snake_y[current_len - 1] = new_y;
                    
                    draw_Snake_Block(new_x, new_y, 1);
                    spawn_Fruit();
                } else {
                    draw_Snake_Block(snake_x[0], snake_y[0], 0);
                    for (i = 0; i < current_len - 1; i++) {
                        snake_x[i] = snake_x[i+1];
                        snake_y[i] = snake_y[i+1];
                    }
                    snake_x[current_len - 1] = new_x;
                    snake_y[current_len - 1] = new_y;
                    draw_Snake_Block(new_x, new_y, 1);
                }
                
                for (i = 0; i < current_len; i++) {
                    draw_Snake_Block(snake_x[i], snake_y[i], 1);
                }
                draw_Snake_Block(fruit_x, fruit_y, 1);
            }
        }
        
        // ??????
        Game_Delay_and_Scan(200000); 
    }
}
