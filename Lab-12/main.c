/*
 * ================================================================
 * Lab 12: 1A2B猜數字遊戲（UART通訊版）
 * 功能：實作一個雙人對戰的1A2B猜數字遊戲，使用UART進行通訊
 * 硬體：Nu-LB-NUC140開發板，NUC140VE3CN微控制器（需要兩塊開發板）
 * 作者：damnm3@googlegroups.com 共同作者
 * ================================================================
 * 
 * 硬體連接：
 * - UART1_TX (PB5) 連接至對手的 UART1_RX
 * - UART1_RX (PB4) 連接至對手的 UART1_TX
 * - LCD顯示器：顯示猜測歷史記錄
 * - 七段顯示器：顯示自己的密碼（僅供參考）
 * - 按鍵矩陣：輸入猜測數字
 * 
 * 遊戲規則：
 * - A (位置正確): 數字正確且位置正確
 * - B (數字正確): 數字正確但位置錯誤
 * - 範例：密碼為 1234，猜測為 1356 → 1A1B
 */

#include <stdio.h>              // 標準輸入輸出函數
#include <stdlib.h>             // 標準函數庫（rand）
#include <string.h>             // 字串處理函數
#include "NUC100Series.h"       // NUC100系列微控制器定義
#include "MCU_init.h"          // 微控制器初始化函數
#include "SYS_init.h"          // 系統初始化函數
#include "LCD.h"               // LCD顯示函數庫
#include "Scankey.h"           // 按鍵掃描函數
#include "Seven_Segment.h"     // 七段顯示器函數庫

// ================================================================
// 全域變數
// ================================================================
volatile uint8_t secret[4] = {0,0,0,0};      // 自己的密碼（4位不重複數字）
volatile uint8_t guess_buf[4];               // 輸入緩衝區（儲存自己輸入的猜測）
volatile uint8_t opponent_guess[4];          // 對手的猜測（從UART接收）

volatile uint8_t g_u8Key_Flag = 0;           // 按鍵標記（Timer0中斷設定）
volatile int g_i8Key_Index = 0;              // 輸入索引（已輸入的位數，0-4）
volatile int g_i8RX_Index = 0;                // 接收索引（已接收的位數，0-4）
volatile uint8_t g_u8RX_Complete = 0;         // 接收完成標記（4位數字接收完成）

volatile uint32_t cnt_5ms = 0;               // 5毫秒計數器（用於七段顯示器多工）

// ================================================================
// 歷史記錄緩衝區（LCD顯示用）
// ================================================================
char g_History[4][17] = {
    "                ",  // 第1行（最舊的記錄）
    "                ",  // 第2行
    "                ",  // 第3行
    "                "   // 第4行（最新的記錄）
};

// ================================================================
// 函數宣告
// ================================================================
extern void print_Line(int8_t line, char text[]);  // LCD列印函數
extern void init_LCD(void);                        // LCD初始化函數
extern void clear_LCD(void);                       // LCD清除函數

void Init_Timer0(void);                            // Timer0初始化（按鍵掃描）
void Init_Timer1(void);                            // Timer1初始化（七段顯示器多工）
void Init_UART1(void);                             // UART1初始化（通訊）
void Generate_Secret(void);                        // 產生密碼
void Calculate_1A2B(volatile uint8_t *secret_arr, volatile uint8_t *guess_arr, int *A, int *B);  // 計算1A2B結果
void Add_Result_To_History(volatile uint8_t *guess, int A, int B);  // 將結果加入歷史記錄

/*
 * ================================================================
 * 主程式
 * 功能：系統初始化和主迴圈，處理按鍵輸入和UART通訊
 * ================================================================
 */
int main(void)
{
    int A, B;                   // 1A2B計算結果
    uint32_t i;                 // 迴圈計數器

    // ================================================================
    // 1. 系統初始化
    // ================================================================
    SYS_Init();                 // 系統初始化
    Init_Timer0();              // Timer0初始化（按鍵掃描，50Hz）
    Init_Timer1();              // Timer1初始化（七段顯示器多工，200Hz）
    Init_UART1();               // UART1初始化（115200 baud）
    
    init_LCD();                 // LCD初始化
    clear_LCD();                 // 清除LCD畫面
    OpenKeyPad();                // 開啟按鍵掃描功能
    
    // ================================================================
    // 2. 遊戲設定
    // ================================================================
    Generate_Secret();           // 產生4位不重複數字的密碼

    // ================================================================
    // 主程式迴圈
    // ================================================================
    while(1)
    {
        // --- 任務A：處理按鍵輸入 ---
        if (g_u8Key_Flag != 0) 
        {
            // 如果尚未輸入4位數字，將按鍵值存入緩衝區
            if (g_i8Key_Index < 4) {
                guess_buf[g_i8Key_Index] = g_u8Key_Flag;
                g_i8Key_Index++;
            }
            
            // 如果已輸入4位數字，發送給對手
            if (g_i8Key_Index == 4) {
                // 透過UART發送4位數字
                for(i=0; i<4; i++) {
                    // 等待UART傳送緩衝區為空
                    while((UART1->FSR & UART_FSR_TX_EMPTY_Msk) == 0); 
                    // 發送一個位元組
                    UART_WRITE(UART1, guess_buf[i]);
                }
                
                g_i8Key_Index = 0;  // 重置輸入索引
            }
            
            g_u8Key_Flag = 0;    // 清除按鍵標記
        }

        // --- 任務B：處理接收到的猜測 ---
        if (g_u8RX_Complete == 1) 
        {
            // 計算1A2B結果
            Calculate_1A2B(secret, opponent_guess, &A, &B);
            // 將結果加入歷史記錄並更新LCD顯示
            Add_Result_To_History(opponent_guess, A, B);
            g_u8RX_Complete = 0;  // 清除接收完成標記
        }
    }
}

// ================================================================
// 中斷服務常式 (Interrupt Service Routines)
// ================================================================

/*
 * ================================================================
 * Timer0中斷處理函數（按鍵掃描）
 * 功能：每20ms掃描一次按鍵，檢測按鍵按下事件
 * 頻率：50Hz（比原來的5Hz快10倍，提供更快的響應）
 * ================================================================
 */
void TMR0_IRQHandler(void)
{
    static uint8_t u8LastKey = 0;  // 記錄上次按鍵狀態（靜態變數）
    uint8_t u8CurrentKey;          // 當前按鍵狀態

    TIMER_ClearIntFlag(TIMER0);     // 清除Timer0中斷標記
    
    u8CurrentKey = ScanKey();      // 掃描當前按鍵狀態
    
    // 邏輯：只有在按鍵從未按下變為按下時才記錄
    // 這樣可以防止按住按鍵時重複觸發（避免出現"1111"的情況）
    if (u8CurrentKey != 0 && u8LastKey == 0) {
        g_u8Key_Flag = u8CurrentKey;  // 設定按鍵標記
    }
    
    u8LastKey = u8CurrentKey;      // 更新歷史狀態
}

/*
 * ================================================================
 * Timer1中斷處理函數（七段顯示器多工掃描）
 * 功能：每5ms切換一位七段顯示器，顯示自己的密碼
 * 頻率：200Hz（每5ms觸發一次，4位數字輪流顯示）
 * ================================================================
 */
void TMR1_IRQHandler(void)
{
    TIMER_ClearIntFlag(TIMER1);     // 清除Timer1中斷標記
    cnt_5ms++;                      // 5毫秒計數器加1
    
    // 根據計數器的值決定顯示哪一位數字
    if (cnt_5ms % 4 == 0) {
        CloseSevenSegment();        // 關閉所有七段顯示器
        ShowSevenSegment(3, secret[0]);  // 顯示第4位（最左邊）
    }
    else if (cnt_5ms % 4 == 1) {
        CloseSevenSegment();
        ShowSevenSegment(2, secret[1]);  // 顯示第3位
    }
    else if (cnt_5ms % 4 == 2) {
        CloseSevenSegment();
        ShowSevenSegment(1, secret[2]);  // 顯示第2位
    }
    else if (cnt_5ms % 4 == 3) {
        CloseSevenSegment();
        ShowSevenSegment(0, secret[3]);  // 顯示第1位（最右邊）
    }
}

/*
 * ================================================================
 * UART1中斷處理函數（資料接收）
 * 功能：接收對手發送的猜測數字（4位數字）
 * 觸發：接收資料就緒（RDA - Receive Data Available）
 * ================================================================
 */
void UART1_IRQHandler(void)
{
    uint32_t u32IntSts = UART1->ISR;  // 讀取UART1中斷狀態暫存器
    uint8_t c;                        // 接收的資料位元組
    
    // 檢查是否為接收資料就緒中斷
    if(u32IntSts & UART_ISR_RDA_INT_Msk) 
    {
        c = UART_READ(UART1);         // 讀取接收到的資料
        
        // 如果尚未接收完4位數字，存入緩衝區
        if (g_i8RX_Index < 4) {
            opponent_guess[g_i8RX_Index] = c;
            g_i8RX_Index++;
        }
        
        // 如果已接收完4位數字，設定接收完成標記
        if (g_i8RX_Index == 4) {
            g_i8RX_Index = 0;         // 重置接收索引
            g_u8RX_Complete = 1;       // 設定接收完成標記
        }
    }
}

// ================================================================
// 初始化函數
// ================================================================

/*
 * ================================================================
 * Timer0初始化函數（按鍵掃描）
 * 功能：設定Timer0為50Hz週期性模式，用於按鍵掃描
 * 更新：從5Hz改為50Hz，提供更快的響應速度（每20ms掃描一次）
 * ================================================================
 */
void Init_Timer0(void) { 
    TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 50);  // 開啟Timer0，50Hz（每20ms觸發）
    TIMER_EnableInt(TIMER0);                      // 啟用Timer0中斷
    NVIC_EnableIRQ(TMR0_IRQn);                    // 啟用NVIC中斷
    TIMER_Start(TIMER0);                           // 啟動Timer0
}

/*
 * ================================================================
 * Timer1初始化函數（七段顯示器多工）
 * 功能：設定Timer1為200Hz週期性模式，用於七段顯示器多工掃描
 * ================================================================
 */
void Init_Timer1(void) { 
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 200);  // 開啟Timer1，200Hz（每5ms觸發）
    TIMER_EnableInt(TIMER1);                       // 啟用Timer1中斷
    NVIC_EnableIRQ(TMR1_IRQn);                     // 啟用NVIC中斷
    TIMER_Start(TIMER1);                            // 啟動Timer1
}

/*
 * ================================================================
 * UART1初始化函數（通訊）
 * 功能：初始化UART1，設定為115200 baud，啟用接收中斷
 * ================================================================
 */
void Init_UART1(void) { 
    UART_Open(UART1, 115200);                      // 開啟UART1，115200 baud
    UART_EnableInt(UART1, UART_IER_RDA_IEN_Msk);   // 啟用接收資料就緒中斷
    NVIC_EnableIRQ(UART1_IRQn);                     // 啟用NVIC中斷
}

// ================================================================
// 遊戲邏輯函數
// ================================================================

/*
 * ================================================================
 * 產生密碼函數
 * 功能：產生4位不重複數字的密碼
 * 演算法：使用rand()產生隨機數，並檢查重複性
 * ================================================================
 */
void Generate_Secret(void) {
    int i, j;
    int is_duplicate;              // 重複標記
    
    // 產生4位數字
    for (i = 0; i < 4; i++) {
        do {
            is_duplicate = 0;
            secret[i] = rand() % 10;  // 產生0-9的隨機數
            
            // 檢查是否與前面的數字重複
            for (j = 0; j < i; j++) {
                if (secret[i] == secret[j]) {
                    is_duplicate = 1;  // 發現重複
                    break;
                }
            }
        } while (is_duplicate);       // 如果重複，重新產生
    }
}

/*
 * ================================================================
 * 計算1A2B結果函數
 * 功能：計算猜測與密碼的1A2B結果
 * 參數：
 *   - secret_arr: 密碼陣列
 *   - guess_arr: 猜測陣列
 *   - A: 位置正確的數量（輸出參數）
 *   - B: 數字正確但位置錯誤的數量（輸出參數）
 * ================================================================
 */
void Calculate_1A2B(volatile uint8_t *secret_arr, volatile uint8_t *guess_arr, int *A, int *B) {
    int i, j;
    *A = 0;  // 初始化A計數器
    *B = 0;  // 初始化B計數器
    
    // 遍歷每個位置
    for (i = 0; i < 4; i++) {
        // 檢查位置是否正確
        if (guess_arr[i] == secret_arr[i]) {
            (*A)++;  // 位置正確，A加1
        } else {
            // 位置不正確，檢查數字是否存在於密碼中
            for (j = 0; j < 4; j++) {
                if (guess_arr[i] == secret_arr[j]) {
                    (*B)++;  // 數字正確但位置錯誤，B加1
                    break;   // 找到後跳出內層迴圈
                }
            }
        }
    }
}

/*
 * ================================================================
 * 將結果加入歷史記錄函數
 * 功能：將猜測結果加入歷史記錄並更新LCD顯示
 * 參數：
 *   - guess: 猜測陣列
 *   - A: 位置正確的數量
 *   - B: 數字正確但位置錯誤的數量
 * ================================================================
 */
void Add_Result_To_History(volatile uint8_t *guess, int A, int B) {
    // 將歷史記錄向上移動（最舊的記錄被覆蓋）
    strcpy(g_History[0], g_History[1]);
    strcpy(g_History[1], g_History[2]);
    strcpy(g_History[2], g_History[3]);

    // 格式化新的歷史記錄（格式：1234 1A2B）
    sprintf(g_History[3], "%d%d%d%d %dA%dB", 
            guess[0], guess[1], guess[2], guess[3], A, B);

    // 更新LCD顯示
    clear_LCD();                // 清除LCD畫面
    print_Line(0, g_History[0]); // 顯示第1行（最舊的記錄）
    print_Line(1, g_History[1]); // 顯示第2行
    print_Line(2, g_History[2]); // 顯示第3行
    print_Line(3, g_History[3]); // 顯示第4行（最新的記錄）
}
