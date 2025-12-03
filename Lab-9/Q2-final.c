#include <stdio.h>
#include <stdlib.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Seven_Segment.h" 

// ---------------- 定義常數 ----------------
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

// ---------------- 外部函式宣告 ----------------
// 確保你有 Seven_Segment.c 在 Library 中
extern void OpenSevenSegment(void);
extern void ShowSevenSegment(uint8_t no, uint8_t number);
extern void CloseSevenSegment(void);

// ---------------- 全域變數 ----------------
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

// ---------------- [新功能] Timer 掃描相關變數 ----------------
volatile int8_t g_DisplayBuf[4] = {-1, -1, -1, -1}; 
volatile uint8_t g_ScanIndex = 0; 

// ---------------- Timer0 中斷服務程式 (ISR) ----------------
// 這個函式由硬體自動呼叫，用來解決七段顯示器閃爍問題
void TMR0_IRQHandler(void)
{
    // 1. 清除 Timer0 中斷旗標
    TIMER0->TISR = 1; 

    // 2. 關閉所有顯示 (消影)
    CloseSevenSegment();

    // 3. 顯示目前的位數
    if (g_DisplayBuf[g_ScanIndex] != -1) {
        ShowSevenSegment(g_ScanIndex, g_DisplayBuf[g_ScanIndex]);
    }

    // 4. 準備下一次掃描的位數
    g_ScanIndex++;
    if (g_ScanIndex >= 4) g_ScanIndex = 0;
}

// ---------------- 初始化 Timer0 ----------------
void Init_Timer0_For_Scan(void)
{
    // 開啟 Timer0 時鐘
    CLK->APBCLK |= (1 << 2); // TMR0_EN

    // 選擇 Timer0 時鐘源為 HXT (12MHz)
    CLK->CLKSEL1 &= ~(0x7 << 8); 

    // 設定 Timer0: Prescaler=11, CMP=2500 -> 2.5ms 中斷一次 (400Hz)
    TIMER0->TCSR = 0; 
    TIMER0->TCSR |= (11 << 0); 
    TIMER0->TCMPR = 2500;      
    TIMER0->TCSR |= (1 << 29); // IE (Interrupt Enable)
    TIMER0->TCSR |= (1 << 27); // Periodic mode
    
    NVIC_EnableIRQ(TMR0_IRQn);

    // 啟動 Timer0
    TIMER0->TCSR |= (1 << 30); // CEN
}

// ---------------- 更新顯示緩衝區 ----------------
void Update_Score_Display(int val)
{
    // 個位數
    g_DisplayBuf[0] = val % 10;

    // 十位數
    if (val >= 10) g_DisplayBuf[1] = (val / 10) % 10;
    else g_DisplayBuf[1] = -1;

    // 百位數
    if (val >= 100) g_DisplayBuf[2] = (val / 100) % 10;
    else g_DisplayBuf[2] = -1;

    // 千位數
    if (val >= 1000) g_DisplayBuf[3] = (val / 1000) % 10;
    else g_DisplayBuf[3] = -1;
}

// ---------------- ADC 相關函式 ----------------
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

// ---------------- 繪圖與遊戲邏輯 ----------------

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
    Update_Score_Display(score); // 初始分數顯示
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

// ---------------- 主程式 ----------------
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
    
    // [重要] PC0 設為 Quasi，防止浮接造成一直 Reset
    GPIO_SetMode(PC, BIT0, GPIO_MODE_QUASI); 

    // [重要] 初始化 Timer0 來負責七段顯示器掃描
    Init_Timer0_For_Scan();

    init_Game();

    while(1) {
        // 重置鍵
        if (PC0 == 0) { 
            init_Game();
            CLK_SysTickDelay(500000); 
        }

        // 遊戲結束，雖然卡住，但 Timer 中斷仍會在背景更新顯示器
        if (game_over) {
            CLK_SysTickDelay(200000);
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
                    // 吃到水果
                    score += 10;
                    Update_Score_Display(score); // 更新顯示 Buffer

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
                
                // 斷尾修復
                for (i = 0; i < current_len; i++) {
                    draw_Snake_Block(snake_x[i], snake_y[i], 1);
                }
                draw_Snake_Block(fruit_x, fruit_y, 1);
            }
        }
        
        CLK_SysTickDelay(200000); 
    }
}
