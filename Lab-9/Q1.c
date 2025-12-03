#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"

// ---------------- Snake Parameters / 蛇的基本參數 ----------------
#define SNAKE_LEN   16
#define GRID_W      64  // Grid width (128/2) / 格子寬度
#define GRID_H      32  // Grid height (64/2)  / 格子高度

// Joystick ADC threshold / 搖桿 ADC 參數
#define ADC_CENTER  2048
#define ADC_THRES   700 // Dead-zone threshold (about 1/3) / 休止區域閾值(約1/3)

// Direction enum / 方向列舉
typedef enum {
    DIR_STOP = 0,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

// ---------------- ADC / Input Variables / ADC 與輸入參數 ----------------
volatile uint8_t u8ADF;
volatile uint16_t X_ADC, Y_ADC; 
volatile uint8_t  B_Button;

// Snake body coordinates / 蛇身座標 (index 0 = tail, index 15 = head)
int8_t snake_x[SNAKE_LEN];
int8_t snake_y[SNAKE_LEN];

Direction current_dir = DIR_RIGHT; // Current direction / 當前方向
Direction next_dir = DIR_RIGHT;    // Next direction requested by joystick / 搖桿要求的方向

// ---------------- ADC Interrupt Handler / ADC中斷 ----------------
void ADC_IRQHandler(void)
{
    uint32_t u32Flag;
    u32Flag = ADC_GET_INT_FLAG(ADC, ADC_ADF_INT);

    if(u32Flag & ADC_ADF_INT) {
        X_ADC = ADC_GET_CONVERSION_DATA(ADC, 0); // Read X-axis ADC / 讀取 X 軸
        Y_ADC = ADC_GET_CONVERSION_DATA(ADC, 1); // Read Y-axis ADC / 讀取 Y 軸
    }
    ADC_CLR_INT_FLAG(ADC, u32Flag);
}

void Init_ADC(void)
{
    ADC_Open(ADC, ADC_INPUT_MODE, ADC_OPERATION_MODE, ADC_CHANNEL_MASK); // Enable ADC channels / 開啟 ADC
    ADC_POWER_ON(ADC);
    ADC_EnableInt(ADC, ADC_ADF_INT);
    NVIC_EnableIRQ(ADC_IRQn);
    ADC_START_CONV(ADC);    
}

// ---------------- Drawing Snake on LCD / 在 LCD 畫出蛇 ----------------
void draw_Snake_Block(int8_t gx, int8_t gy, uint16_t color)
{
    int16_t px; 
    int16_t py;
    uint16_t bg;

    px = gx * 2; // Convert grid X to LCD pixel / 格子座標轉換成像素
    py = gy * 2; // Convert grid Y to LCD pixel / 格子座標轉換成像素
    
    // color=1 (white block), bg=0; color=0 (black block), bg=1
    // color=1 白色，color=0 黑色
    bg = (color == 0) ? 1 : 0; 
    
    draw_Pixel(px,     py,     color, bg);
    draw_Pixel(px + 1, py,     color, bg);
    draw_Pixel(px,     py + 1, color, bg);
    draw_Pixel(px + 1, py + 1, color, bg);
}

// Initialize snake position / 初始化蛇的位置
void init_Snake(void)
{
    int i;
    int8_t start_x; 
    int8_t start_y;
    
    start_x = (GRID_W / 2) - (SNAKE_LEN / 2); // Center snake horizontally / 水平置中
    start_y = (GRID_H / 2);                   // Center vertically / 垂直置中
    
    current_dir = DIR_RIGHT;
    next_dir = DIR_RIGHT;

    clear_LCD(); // Clear screen / 清除畫面

    // Draw snake initial state / 畫出初始蛇
    for(i = 0; i < SNAKE_LEN; i++) {
        snake_x[i] = start_x + i; 
        snake_y[i] = start_y;
        draw_Snake_Block(snake_x[i], snake_y[i], 1);
    }
}

// ---------------- Joystick Logic / 搖桿邏輯 ----------------
void update_Joystick_Logic(void)
{
    int32_t dx;
    int32_t dy;
    uint32_t dist_sq;
    uint32_t thres_sq;
    Direction req_dir = DIR_STOP;

    dx = (int32_t)X_ADC - ADC_CENTER;
    dy = (int32_t)Y_ADC - ADC_CENTER;
    
    dist_sq = (dx*dx) + (dy*dy);        // Square distance from center / 搖桿偏移距離平方
    thres_sq = ADC_THRES * ADC_THRES;  // Dead-zone threshold squared / 閾值平方

    // If inside dead-zone → STOP / 若在休止區域 → 停止
    if (dist_sq < thres_sq) {
        next_dir = DIR_STOP;
        return;
    }

    if (abs(dx) > abs(dy)) {
        // Horizontal movement / 水平方向
        if (dx > 0) req_dir = DIR_RIGHT;
        else req_dir = DIR_LEFT;
    } else {
        // Vertical movement / 垂直方向
        if (dy > 0) req_dir = DIR_DOWN; 
        else req_dir = DIR_UP;
    }

    // Prevent reversing direction / 禁止直接反向
    if (current_dir == DIR_RIGHT && req_dir == DIR_LEFT) return;
    if (current_dir == DIR_LEFT && req_dir == DIR_RIGHT) return;
    if (current_dir == DIR_UP && req_dir == DIR_DOWN) return;
    if (current_dir == DIR_DOWN && req_dir == DIR_UP) return;

    next_dir = req_dir;
}

// ---------------- Main Game Loop / 主遊戲迴圈 ----------------
int32_t main (void)
{
    int i; 
    int8_t head_x, head_y, new_x, new_y;
    int valid_move;
    
    int game_over = 0; // Game over flag / 遊戲結束旗標

    // 1. System init / 系統初始化
    SYS_Init();
    SYS_UnlockReg();

    // Enable SPI3 and ADC clocks / 打開 SPI3 與 ADC 時脈
    CLK->APBCLK |= (1 << 15) | (1 << 28); 

    // Pin MFP settings / 設定多功能腳位
    SYS->GPD_MFP |= (SYS_GPD_MFP_PD8_SPI3_SS0 | SYS_GPD_MFP_PD9_SPI3_CLK | SYS_GPD_MFP_PD11_SPI3_MOSI0);
    SYS->GPA_MFP |= (SYS_GPA_MFP_PA0_ADC0 | SYS_GPA_MFP_PA1_ADC1);

    SYS_LockReg();

    // LCD reset sequence / LCD 初始化流程
    GPIO_SetMode(PD, BIT12 | BIT14, GPIO_MODE_OUTPUT); 
    PD14 = 1; 
    PD12 = 1; CLK_SysTickDelay(10000);
    PD12 = 0; CLK_SysTickDelay(10000);
    PD12 = 1; CLK_SysTickDelay(100000); 

    // 3. Initialize ADC and LCD / 初始化 ADC 與 LCD
    Init_ADC(); 
    init_LCD();
    clear_LCD();
    
    GPIO_SetMode(PC, BIT0, GPIO_MODE_INPUT); // PC0 reset button / PC0 作為重設按鈕

    // 4. Initialize snake / 初始化蛇
    init_Snake();

    while(1) {
        // If button pressed → reset game / PC0 按下 → 重設遊戲
        if (PC0 == 0) { 
            init_Snake();
            game_over = 0; 
            CLK_SysTickDelay(500000);
        }

        // If game over → freeze movement / 遊戲結束 → 不再移動
        if (game_over == 1) {
            CLK_SysTickDelay(200000);
            continue;
        }

        update_Joystick_Logic();

        // Only move when direction ≠ STOP / 搖桿若不是 STOP 才移動
        if (next_dir != DIR_STOP) {
            
            head_x = snake_x[SNAKE_LEN - 1];
            head_y = snake_y[SNAKE_LEN - 1];
            new_x = head_x;
            new_y = head_y;
            valid_move = 1;

            current_dir = next_dir;

            // Compute new head position / 計算新蛇頭位置
            switch(current_dir) {
                case DIR_UP:    new_y = head_y - 1; break;
                case DIR_DOWN:  new_y = head_y + 1; break;
                case DIR_LEFT:  new_x = head_x - 1; break;
                case DIR_RIGHT: new_x = head_x + 1; break;
                default: break;
            }

            // Boundary check / 邊界檢查
            if (new_x < 0 || new_x >= GRID_W || new_y < 0 || new_y >= GRID_H) {
                valid_move = 0;
            }

            // Collision check with snake body / 檢查是否撞到自己
            if (valid_move) {
                for (i = 0; i < SNAKE_LEN - 1; i++) { 
                    if (snake_x[i] == new_x && snake_y[i] == new_y) {
                        valid_move = 0;
                        game_over = 1;
                        break;
                    }
                }
            }

            if (valid_move) {
                // Erase tail / 抹掉舊尾巴
                draw_Snake_Block(snake_x[0], snake_y[0], 0);

                // Shift body forward / 蛇身前移
                for (i = 0; i < SNAKE_LEN - 1; i++) {
                    snake_x[i] = snake_x[i+1];
                    snake_y[i] = snake_y[i+1];
                }

                // Add new head / 新蛇頭
                snake_x[SNAKE_LEN - 1] = new_x;
                snake_y[SNAKE_LEN - 1] = new_y;

                // Draw new head / 畫新蛇頭
                draw_Snake_Block(new_x, new_y, 1);

                // Redraw full snake (avoid flickering problems) / 重畫整條蛇避免殘影
                for (i = 0; i < SNAKE_LEN; i++) {
                    draw_Snake_Block(snake_x[i], snake_y[i], 1);
                }
            }
        }
        
        CLK_SysTickDelay(200000); // movement speed / 移動速度
    }
}
