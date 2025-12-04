#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Scankey.h"
// 2D繪圖函數庫，包含矩形繪製等功能
#include "Draw2D.h"

// ==========================================
//              常數定義
// ==========================================
// LCD顯示器尺寸
#define LCD_W 128        // LCD寬度（像素）
#define LCD_H 64         // LCD高度（像素）

// 硬體腳位定義
#define BUZZER_PIN 11    // 蜂鳴器腳位（PB11）
#define ADC_VR_CHANNEL 7 // ADC可變電阻通道（PA7）

// ==========================================
//              遊戲物件尺寸定義
// ==========================================
#define BALL_SIZE 8      // 球體尺寸（8x8像素）
#define PADDLE_W 16      // 擋板寬度（16像素）
#define PADDLE_H 8       // 擋板高度（8像素）
#define OBSTACLE_W 16    // 障礙物寬度（16像素）
#define OBSTACLE_H 8     // 障礙物高度（8像素）

// ==========================================
//              遊戲狀態列舉
// ==========================================
/**
 * @brief 遊戲狀態列舉
 * @note STATE_INIT: 初始狀態，等待按鍵開始
 * @note STATE_PLAYING: 遊戲進行中
 * @note STATE_GAMEOVER: 遊戲結束，球體掉落底部
 */
typedef enum {
    STATE_INIT,      // 初始狀態
    STATE_PLAYING,   // 遊戲進行中
    STATE_GAMEOVER  // 遊戲結束
} GameState;

// ==========================================
//              資料結構定義
// ==========================================
/**
 * @brief 矩形結構體（用於擋板和障礙物）
 * @param x 左上角X座標
 * @param y 左上角Y座標
 * @param w 寬度
 * @param h 高度
 */
typedef struct {
    int x, y;  // 左上角座標
    int w, h;  // 寬度和高度
} Rect;

/**
 * @brief 球體物件結構體
 * @param x  當前X座標
 * @param y  當前Y座標
 * @param dx X方向速度（像素/次，可為正負）
 * @param dy Y方向速度（像素/次，可為正負）
 * @param w  寬度
 * @param h  高度
 */
typedef struct {
    int x, y;   // 當前座標
    int dx, dy; // X和Y方向的速度
    int w, h;   // 寬度和高度
} BallObj;

// ==========================================
//              全域變數
// ==========================================
volatile GameState g_state = STATE_INIT;  // 當前遊戲狀態（volatile確保編譯器不優化）
Rect g_paddle;                            // 擋板物件
Rect g_obstacle;                          // 障礙物物件
BallObj g_ball;                           // 球體物件

// ==========================================
//              函數宣告
// ==========================================
void Init_Hardware(void);      // 硬體初始化
void Init_Game_Data(void);     // 遊戲資料初始化
void Update_Paddle_Pos(void); // 更新擋板位置（根據ADC）
void Beep(void);               // 蜂鳴器響聲
void Draw_Game(void);          // 繪製遊戲畫面

// ==========================================
//              硬體初始化
// ==========================================
/**
 * @brief 初始化所有硬體設備
 * @note 包含系統初始化、LCD、按鍵、ADC和蜂鳴器
 */
void Init_Hardware(void)
{
    // 基本系統初始化
    SYS_Init();      // 系統時鐘和基本設定
    init_LCD();      // LCD顯示器初始化
    clear_LCD();     // 清除LCD畫面
    OpenKeyPad();    // 按鍵矩陣初始化

    // ========== ADC初始化（可變電阻VR1連接至PA7） ==========
    // 解鎖暫存器寫入保護（需要特定序列）
    SYS->REGWRPROT = 0x59;
    SYS->REGWRPROT = 0x16;
    SYS->REGWRPROT = 0x88;

    // 設定PA7為ADC功能（多功能腳位設定）
    SYS->GPA_MFP |= (1UL << ADC_VR_CHANNEL);

    // 啟用ADC時鐘（APBCLK bit 28）
    CLK->APBCLK |= (1UL << 28);
    
    // 選擇ADC時鐘源（清除bit 2-3，使用預設時鐘）
    CLK->CLKSEL1 &= ~(0x3UL << 2);
    
    // 清除ADC時鐘分頻器（使用預設分頻）
    CLK->CLKDIV &= ~(0xFFUL << 16);

    // 重新鎖定暫存器寫入保護
    SYS->REGWRPROT = 0x00;

    // 設定PA7為類比輸入模式（清除PMD設定）
    PA->PMD &= ~(0x3UL << (ADC_VR_CHANNEL * 2));
    
    // 關閉數位輸入緩衝器（類比輸入需要）
    PA->OFFD |= (1UL << ADC_VR_CHANNEL);

    // 啟用ADC（ADCR bit 0）
    ADC->ADCR |= (1UL << 0);
    
    // 啟用ADC通道7（ADCHER: ADC Channel Enable Register）
    ADC->ADCHER |= (1UL << ADC_VR_CHANNEL);

    // ========== 蜂鳴器初始化（PB11） ==========
    // 清除PB11的模式設定
    PB->PMD &= ~(0x3UL << (BUZZER_PIN * 2));
    
    // 設定PB11為輸出模式（PMD = 01）
    PB->PMD |= (0x1UL << (BUZZER_PIN * 2));
    
    // 初始化蜂鳴器為高電位（不響）
    // 注意：蜂鳴器為Active-Low，低電位時響
    PB->DOUT |= (1UL << BUZZER_PIN);
}

// ==========================================
//              遊戲邏輯函數
// ==========================================

/**
 * @brief 初始化遊戲資料
 * @note 設定球體、擋板和障礙物的初始位置和速度
 */
void Init_Game_Data(void)
{
    int speed = 4;  // 球體初始速度（像素/次）

    // ========== 障礙物初始位置 ==========
    // 障礙物位於螢幕上方中央
    g_obstacle.x = 56;  // X座標：(128-16)/2 = 56（水平置中）
    g_obstacle.y = 8;   // Y座標：距離頂部8像素
    g_obstacle.w = OBSTACLE_W;
    g_obstacle.h = OBSTACLE_H;

    // ========== 擋板初始位置 ==========
    // 擋板位於螢幕底部中央
    g_paddle.x = 56;                    // X座標：(128-16)/2 = 56（水平置中）
    g_paddle.y = LCD_H - PADDLE_H;     // Y座標：64-8 = 56（緊貼底部）
    g_paddle.w = PADDLE_W;
    g_paddle.h = PADDLE_H;

    // ========== 球體初始位置和速度 ==========
    // 球體位於螢幕中央
    g_ball.x = (LCD_W - BALL_SIZE) / 2;  // X座標：(128-8)/2 = 60（水平置中）
    g_ball.y = (LCD_H - BALL_SIZE) / 2;   // Y座標：(64-8)/2 = 28（垂直置中）
    g_ball.w = BALL_SIZE;
    g_ball.h = BALL_SIZE;
    
    // 隨機設定球體的初始移動方向
    // dx和dy可以是+speed或-speed，形成四個方向之一
    g_ball.dx = (rand() % 2 == 0) ? speed : -speed;  // X方向：隨機左右
    g_ball.dy = (rand() % 2 == 0) ? speed : -speed;  // Y方向：隨機上下
}

/**
 * @brief 更新擋板位置（根據ADC可變電阻值）
 * @note 讀取ADC值並映射到擋板的X座標範圍
 * @note 使用8次取樣平均，提高穩定性
 */
void Update_Paddle_Pos(void)
{
    uint32_t adc_val = 0;
    int i;
    
    // ========== ADC取樣（8次取樣平均） ==========
    // 多次取樣取平均可以減少雜訊影響，提高穩定性
    for(i = 0; i < 8; i++)
    {
        // 啟動ADC轉換（ADCR bit 11: ADST）
        ADC->ADCR |= (1UL << 11);
        
        // 等待轉換完成（ADST位元自動清除）
        while(ADC->ADCR & (1UL << 11));
        
        // 讀取ADC轉換結果（12位元，範圍0-4095）
        // ADDR[channel]的低12位元（bit 0-11）為轉換結果
        adc_val += (ADC->ADDR[ADC_VR_CHANNEL] & 0xFFF);
    }
    
    // 計算平均值
    adc_val = adc_val / 8;
    
    // ========== ADC值映射到擋板X座標 ==========
    // ADC值範圍：0~4095
    // 擋板X座標範圍：0 ~ (128-16) = 0~112
    // 使用線性映射：paddle_x = (adc_val * 112) / 4096
    g_paddle.x = (adc_val * (LCD_W - PADDLE_W)) / 4096;
}

/**
 * @brief 蜂鳴器響聲
 * @note 蜂鳴器為Active-Low，低電位時響
 * @note 響聲持續時間約50ms
 */
void Beep(void)
{
    // 設定PB11為低電位（蜂鳴器響）
    PB->DOUT &= ~(1UL << BUZZER_PIN);
    
    // 延遲50ms（50000微秒）
    CLK_SysTickDelay(50000);
    
    // 設定PB11為高電位（蜂鳴器停止）
    PB->DOUT |= (1UL << BUZZER_PIN);
}

/**
 * @brief 碰撞檢測函數（AABB：軸對齊邊界框）
 * @param rect 矩形物件（擋板或障礙物）
 * @param ball 球體物件
 * @return 1表示碰撞，0表示未碰撞
 * @note 使用AABB碰撞檢測，將球體視為矩形進行檢測
 */
int Check_Collision(Rect *rect, BallObj *ball)
{
    // AABB碰撞檢測：兩個矩形重疊的條件
    // 矩形A和矩形B重疊，當且僅當：
    // - A的左邊 < B的右邊 且
    // - A的右邊 > B的左邊 且
    // - A的上邊 < B的下邊 且
    // - A的下邊 > B的上邊
    if (ball->x < rect->x + rect->w &&        // 球體左邊 < 矩形右邊
        ball->x + ball->w > rect->x &&         // 球體右邊 > 矩形左邊
        ball->y < rect->y + rect->h &&        // 球體上邊 < 矩形下邊
        ball->y + ball->h > rect->y)           // 球體下邊 > 矩形上邊
    {
        return 1;  // 發生碰撞
    }
    return 0;  // 未碰撞
}

/**
 * @brief 繪製遊戲畫面
 * @note 清除整個LCD，然後繪製球體、擋板和障礙物
 * @note 使用fill_Rectangle函數繪製實心矩形
 */
void Draw_Game(void)
{
    // 清除整個LCD畫面
    clear_LCD();
    
    // ========== 1. 繪製球體（8x8像素白色矩形） ==========
    // fill_Rectangle參數：x0, y0, x1, y1, fgColor, bgColor
    // fgColor=1（白色），bgColor=0（黑色）
    fill_Rectangle(g_ball.x, g_ball.y,
                   g_ball.x + BALL_SIZE - 1, g_ball.y + BALL_SIZE - 1,
                   1, 0);
    
    // ========== 2. 繪製擋板（16x8像素白色矩形） ==========
    fill_Rectangle(g_paddle.x, g_paddle.y,
                   g_paddle.x + PADDLE_W - 1, g_paddle.y + PADDLE_H - 1,
                   1, 0);
    
    // ========== 3. 繪製障礙物（16x8像素白色矩形） ==========
    fill_Rectangle(g_obstacle.x, g_obstacle.y,
                   g_obstacle.x + OBSTACLE_W - 1, g_obstacle.y + OBSTACLE_H - 1,
                   1, 0);
}

// ==========================================
//              主程式
// ==========================================
/**
 * @brief 主程式：打磚塊遊戲
 * @note 使用狀態機控制遊戲流程
 * @note 狀態轉換：INIT -> PLAYING -> GAMEOVER -> INIT
 */
int main(void)
{
    // ========== 系統初始化 ==========
    Init_Hardware();  // 初始化所有硬體設備
    srand(123);       // 設定隨機數種子（固定種子可重現結果）

    // ========== 主遊戲迴圈 ==========
    while(1)
    {
        // ========== 狀態1：初始狀態 ==========
        if (g_state == STATE_INIT)
        {
            // 初始化遊戲資料（球體、擋板、障礙物位置）
            Init_Game_Data();
            
            // 繪製初始畫面
            Draw_Game();
            
            // 等待按鍵開始遊戲
            // 在等待期間持續更新擋板位置（讓使用者可以預先調整）
            while(ScanKey() == 0) {
                Update_Paddle_Pos();  // 根據ADC更新擋板位置
                Draw_Game();          // 更新畫面顯示
                CLK_SysTickDelay(50000);  // 延遲50ms
            }
            
            // 按鍵按下，進入遊戲進行狀態
            g_state = STATE_PLAYING;
        }
        // ========== 狀態2：遊戲進行中 ==========
        else if (g_state == STATE_PLAYING)
        {
            // 更新擋板位置（根據ADC可變電阻）
            Update_Paddle_Pos();

            // ========== 更新球體位置 ==========
            g_ball.x += g_ball.dx;  // X方向移動
            g_ball.y += g_ball.dy;  // Y方向移動

            // ========== 邊界碰撞檢測與反彈 ==========
            // 左邊界碰撞
            if (g_ball.x <= 0) {
                g_ball.x = 0;           // 限制X座標不超出左邊界
                g_ball.dx = -g_ball.dx; // X方向速度反向（反彈）
            }
            // 右邊界碰撞
            else if (g_ball.x >= LCD_W - BALL_SIZE) {
                g_ball.x = LCD_W - BALL_SIZE;  // 限制X座標不超出右邊界
                g_ball.dx = -g_ball.dx;        // X方向速度反向（反彈）
            }

            // 上邊界碰撞
            if (g_ball.y <= 0) {
                g_ball.y = 0;           // 限制Y座標不超出上邊界
                g_ball.dy = -g_ball.dy; // Y方向速度反向（反彈）
            }

            // 下邊界碰撞（球體掉落底部，遊戲結束）
            if (g_ball.y >= LCD_H - BALL_SIZE) {
                g_state = STATE_GAMEOVER;  // 切換到遊戲結束狀態
                CLK_SysTickDelay(200000);  // 延遲200ms
            }

            // ========== 物件碰撞檢測 ==========
            // 球體與擋板碰撞
            if (Check_Collision(&g_paddle, &g_ball)) {
                g_ball.dy = -g_ball.dy;                    // Y方向速度反向（向上反彈）
                g_ball.y = g_paddle.y - BALL_SIZE;         // 調整球體位置，避免穿透擋板
                Beep();                                    // 發出蜂鳴器聲響
            }

            // 球體與障礙物碰撞
            if (Check_Collision(&g_obstacle, &g_ball)) {
                g_ball.dy = -g_ball.dy;  // Y方向速度反向（反彈）
            }

            // 繪製更新後的遊戲畫面
            Draw_Game();
            
            // 延遲100ms，控制遊戲速度
            CLK_SysTickDelay(100000);
        }
        // ========== 狀態3：遊戲結束 ==========
        else if (g_state == STATE_GAMEOVER)
        {
            // 清除畫面並顯示遊戲結束訊息
            clear_LCD();
            printS(30, 24, "GAME OVER");  // 在座標(30, 24)顯示"GAME OVER"
            
            // 等待按鍵重新開始
            while(ScanKey() == 0);
            
            // 延遲500ms，避免按鍵誤觸
            CLK_SysTickDelay(500000);
            
            // 切換回初始狀態，準備下一輪遊戲
            g_state = STATE_INIT;
        }
    }
}
