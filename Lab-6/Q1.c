/*
 * ================================================================
 * Lab 6 - Question 1: 數字選擇與求和系統
 * 功能：從4個隨機數中選擇數字並計算總和，使用LCD顯示和LED指示
 * 硬體：NUC100系列微控制器
 * 作者：damnm3@googlegroups.com 共同作者
 * ================================================================
 * 
 * 硬體連接：
 * - PA0,1,2,3,4,5 連接至3x3按鍵矩陣
 * - PC12,13,14,15 連接至LED指示器
 * - PB11 連接至蜂鳴器
 * - LCD顯示器連接
 * 
 * 功能說明：
 * 1. 系統產生4個10-99的隨機數
 * 2. 使用者可以選擇最多4個數字
 * 3. LED指示已選擇的數字數量
 * 4. LCD顯示當前總和和可選擇的數字
 * 5. 支援重置、清除和返回功能
 * 
 * 按鍵對應：
 * - 按鍵4: 向上移動游標
 * - 按鍵6: 向下移動游標
 * - 按鍵5: 選擇當前數字
 * - 按鍵7: 重置系統
 * - 按鍵8: 清除選擇
 * - 按鍵9: 清除所有
 */

// 包含必要的標頭檔
#include <string.h>             // 字串處理函數
#include "NUC100Series.h"       // NUC100系列微控制器定義
#include "MCU_init.h"           // 微控制器初始化函數
#include "SYS_init.h"           // 系統初始化函數
#include "LCD.h"                // LCD顯示器控制函數
#include "Scankey.h"            // 按鍵掃描函數
#include "clk.h"                // 時鐘控制函數

// ================================================================
// 按鍵定義
// ================================================================
#define KEY_UP 4                // 向上鍵
#define KEY_DOWN 6              // 向下鍵
#define KEY_S 5                 // 選擇鍵
#define KEY_R 7                 // 重置鍵
#define KEY_B 8                 // 返回鍵（退回上一步）
#define KEY_C 9                 // 清除鍵（清空總和與選擇）

// ================================================================
// 全域變數
// ================================================================
static uint32_t g_seed;         // 全域隨機數種子

/*
 * ================================================================
 * 自定義隨機數種子設定函數
 * 功能：設定隨機數種子
 * 參數：seed - 種子值
 * ================================================================
 */
void my_srand(uint32_t seed) { 
    g_seed = seed; 
}

/*
 * ================================================================
 * 自定義隨機數產生函數
 * 功能：產生隨機數
 * 回傳值：隨機數
 * ================================================================
 */
int my_rand(void)
{
    // 如果種子為0（未初始化），避免保持為0
    if (g_seed == 0) g_seed = 1;
   
    // 使用線性同餘生成器(LCG)公式產生隨機數
    g_seed = (1103515245 * g_seed + 12345) & 0x7FFFFFFF;
    return (int)g_seed;
}

/*
 * ================================================================
 * 簡易整數轉字串函數
 * 功能：將整數轉換為字串
 * 參數：val - 要轉換的整數, buf - 輸出緩衝區
 * ================================================================
 */
void simple_itoa(int val, char *buf)
{
    int i = 0;
    char temp[10];
    int j = 0;
   
    // 處理 val = 0 的特殊情況
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
   
    // 不斷取 10 的餘數，反向存到 temp 陣列
    while (val > 0) {
        temp[j++] = (val % 10) + '0';
        val /= 10;
    }
   
    // 再將數字倒回正序存到 buf
    while (j > 0) {
        buf[i++] = temp[--j];
    }
   
    buf[i] = '\0';  // 字串結尾
}

// ================================================================
// 系統狀態變數
// ================================================================
int numbers[4];                 // 4個隨機數
int sum = 0;                    // 當前總和
int cursor_pos = 0;             // 游標位置（0~3）
int view_offset = 0;            // 視窗偏移量（用來顯示第幾個當成第一行）
int selected_numbers[4];        // 已選擇的數字（用來支援返回/清除）
int selected_count = 0;         // 已選擇數量（0~4）

/*
 * 初始化LED腳位（PC12~PC15）
 */
void init_leds(void)
{
    GPIO_SetMode(PC, BIT12 | BIT13 | BIT14 | BIT15, GPIO_PMD_OUTPUT);
    PC12 = 1; 
    PC13 = 1; 
    PC14 = 1; 
    PC15 = 1; // LED 預設熄滅（假設為 active-low）
}

/*
 * 依照已選擇數量更新LED顯示
 */
void update_leds(void)
{
    // 選擇越多顆數字，就點亮越多顆LED（active-low：0 代表亮）
    PC12 = (selected_count > 0) ? 0 : 1;
    PC13 = (selected_count > 1) ? 0 : 1;
    PC14 = (selected_count > 2) ? 0 : 1;
    PC15 = (selected_count > 3) ? 0 : 1;
}

/*
 * 初始化蜂鳴器（PB11）
 */
void init_buzzer(void)
{
    GPIO_SetMode(PB, BIT11, GPIO_PMD_OUTPUT);
    PB11 = 1; // 預設關閉蜂鳴器
}

/*
 * 蜂鳴器響 number 次
 */
void Buzz(int number)
{
    int i;
    for (i = 0; i < number; i++) {
        PB11 = 0; // PB11 = 0 時開啟蜂鳴器
        CLK_SysTickDelay(100000);
        PB11 = 1; // PB11 = 1 時關閉蜂鳴器
        CLK_SysTickDelay(100000);
    }
}

/*
 * 產生 4 個介於 10~99 的隨機整數
 */
void generate_numbers(void)
{
    int i;
    for (i = 0; i < 4; i++) {
        numbers[i] = my_rand() % 90 + 10;
    }
}

/*
 * 更新 LCD 顯示內容：
 * 第 0 行顯示 SUM = 總和
 * 第 1~3 行顯示目前視窗中的 3 個數字（前面可能有游標 '>'）
 */
void update_display(void)
{
    char line_buffer[16 + 1];
    char num_buffer[10];
    int i, j;

    // 先準備 SUM 那一行
    for(i = 0; i < 16; i++) 
        line_buffer[i] = ' ';
   
    line_buffer[0] = 'S'; 
    line_buffer[1] = 'U'; 
    line_buffer[2] = 'M';
    line_buffer[3] = ' '; 
    line_buffer[4] = '='; 
    line_buffer[5] = ' ';
   
    // 將 sum 轉為字串放到第 6 格之後
    simple_itoa(sum, num_buffer);
   
    i = 0;
    while (num_buffer[i] != '\0' && (6 + i) < 16) {
        line_buffer[6 + i] = num_buffer[i];
        i++;
    }
    line_buffer[16] = '\0';
    print_Line(0, line_buffer);   // 在第 0 行顯示

    // 顯示 3 行數字（視窗最多顯示 3 個）
    for (j = 0; j < 3; j++)
    {
        int num_index = j + view_offset;          // 實際要顯示的 numbers[] index
        int has_cursor = (num_index == cursor_pos);

        for(i = 0; i < 16; i++) 
            line_buffer[i] = ' ';

        // 行首顯示游標或空白
        line_buffer[0] = (has_cursor ? '>' : ' ');
        line_buffer[1] = ' '; // 游標後面空一格

        // 將數字轉成字串放到後面
        simple_itoa(numbers[num_index], num_buffer);

        i = 0;
        while (num_buffer[i] != '\0' && (2 + i) < 16) {
            line_buffer[2 + i] = num_buffer[i];
            i++;
        }
        line_buffer[16] = '\0';
        print_Line(j + 1, line_buffer);   // 顯示在第 1~3 行
    }
}

int main(void)
{
    uint8_t keyin;
    uint8_t last_keyin = 0;    // 上一次讀到的按鍵值
    uint32_t count = 0;        // 簡單用來當亂數種子的計數器

    SYS_Init();
    init_LCD();
    clear_LCD();
    OpenKeyPad();
    init_leds();
    init_buzzer();

    generate_numbers();   // 先產生一組隨機數
    update_display();     // 初始化畫面

    while (1)
    {
        count++;
        keyin = ScanKey();   // 持續掃描按鍵
       
        // 當前沒有按鍵，但上一圈有偵測到按鍵，代表「按鍵剛放開」
        if (keyin == 0 && last_keyin != 0)
        {
            // 依照剛放開的是哪一顆按鍵（last_keyin）來執行功能
            switch (last_keyin)
            {
            case KEY_UP: // 向上鍵
                if (cursor_pos > 0)
                {
                    cursor_pos--;
                    // 如果游標往上移回前三個範圍，就將視窗偏移調回 0
                    view_offset = (cursor_pos == 3) ? 1 : 0;
                }
                break;

            case KEY_DOWN: // 向下鍵
                if (cursor_pos < 3)
                {
                    cursor_pos++;
                    // 當游標移到第 4 個項目時，將視窗往下捲動一格
                    view_offset = (cursor_pos == 3) ? 1 : 0;
                }
                break;

            case KEY_S: // 選擇鍵
                // 限制最多只能選 4 次
                if (selected_count < 4)
                {
                    int selected_val = numbers[cursor_pos];
                    sum += selected_val;  // 加到總和中

                    // 記錄這次選到的數字，讓「返回」可以把它扣回去
                    selected_numbers[selected_count] = selected_val;
                    selected_count++;

                    update_leds();  // 更新 LED 顯示已選數量
                    Buzz(1);        // 按一次選擇時響一聲（在放開按鍵時執行）
                }
                break;

            case KEY_R: // 重置鍵
                my_srand(count);    // 使用目前 count 當作亂數種子
                generate_numbers(); // 重新產生新的一組隨機數

                sum = 0;            // 重置總和
                selected_count = 0; // 清除已選數量
                cursor_pos = 0;     // 游標回到第一個
                view_offset = 0;    // 視窗回到顯示前 3 個
                update_leds();      // 所有 LED 熄滅
                break;

            case KEY_B: // 返回鍵（取消最後一次選擇）
                // 如果有選過數字才需要返回
                if (selected_count > 0)
                {
                    selected_count--;                 // 回復已選數量
                    sum -= selected_numbers[selected_count]; // 總和扣掉最後一次選到的數
                    update_leds();                    // 更新 LED 狀態
                }
                break;

            case KEY_C: // 清除鍵（清除所有選擇與總和）
                sum = 0;            // 總和歸零
                selected_count = 0; // 已選數量清零
                // 游標與隨機數內容保持不變
                update_leds();      // LED 全部熄滅
                break;
               
            default:
                // 其他按鍵（例如 1,2,3）這裡不處理
                break;
            }
           
            // 處理完這次按鍵事件後，更新整個 LCD 畫面
            update_display();
        }
       
        // 記錄這一圈掃描到的按鍵值，下一圈拿來判斷「放開」事件
        last_keyin = keyin;
       
        // 小小延遲，減少CPU一直瘋狂輪詢
        CLK_SysTickDelay(10000); // 約 10ms 延遲
    }

    // 這個程式在 while(1) 迴圈中永遠不會結束
    // return 0; // 在一般嵌入式系統中通常不會被執行到
}
