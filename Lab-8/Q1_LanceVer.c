#include "LCD.h"
#include "MCU_init.h"
#include "NUC100Series.h"
#include "SYS_init.h"
#include "Scankey.h"
#include <stdio.h>
#include <stdlib.h>

// ==========================================
//              常數定義
// ==========================================
// 終點位置：LCD寬度128 - 字元寬度8 = 120
// 使用8x16字元顯示（printC函數），考慮邊界留白
#define RIGHT_BOUND 120

// ==========================================
//              資料結構定義
// ==========================================
/**
 * @brief 移動物件結構體
 * @param num     數字值（1-9）
 * @param x       當前X座標位置
 * @param speed   移動速度（像素/次）
 * @param reached 是否已到達終點（1=已到達，0=未到達）
 */
typedef struct {
  int num;     // 數字值（1-9）
  int x;       // 當前X座標位置（0-120）
  int speed;   // 移動速度（像素/次，值為2, 4, 6, 8）
  int reached; // 是否已到達終點（1=已到達，0=未到達）
} MOVING;

// ==========================================
//              全域變數
// ==========================================
MOVING obj[4];               // 4個移動物件的陣列
volatile int start_flag = 0; // 外部中斷啟動旗標（volatile確保編譯器不優化）

// ==========================================
//              延遲函數
// ==========================================
/**
 * @brief 毫秒級延遲函數
 * @param ms 延遲的毫秒數
 * @note 使用系統時鐘延遲，每次迴圈延遲1ms
 */
void Delay_ms(int ms) {
  int i;
  // 迴圈ms次，每次延遲1ms（1000微秒）
  for (i = 0; i < ms; i++)
    CLK_SysTickDelay(1000); // 1000微秒 = 1毫秒
}

// ==========================================
//              外部中斷處理
// ==========================================
/**
 * @brief 外部中斷1中斷服務程式（EINT1 IRQ Handler）
 * @note 當PB15腳位發生下降緣時觸發此中斷
 * @note 此函數由硬體自動呼叫，執行時間應盡量短
 */
void EINT1_IRQHandler(void) {
  // 設定啟動旗標，通知主程式開始競賽
  start_flag = 1;

  // 清除PB15的中斷來源旗標（ISRC: Interrupt Source Register）
  PB->ISRC = (1 << 15);

  // 清除NVIC中的待處理中斷，確保中斷正確處理
  NVIC_ClearPendingIRQ(EINT1_IRQn);
}

/**
 * @brief 初始化外部中斷1（PB15腳位）
 * @note 設定PB15為輸入模式，啟用下降緣觸發中斷
 * @note 啟用硬體防彈跳功能，避免按鈕震動造成多次觸發
 */
void init_EINT1(void) {
  // 設定PB15為輸入模式
  GPIO_SetMode(PB, BIT15, GPIO_MODE_INPUT);

  // 啟用PB15的外部中斷，設定為下降緣觸發（按鈕按下時觸發）
  GPIO_EnableInt(PB, 15, GPIO_INT_FALLING);

  // 在NVIC中啟用EINT1中斷
  NVIC_EnableIRQ(EINT1_IRQn);

  // 啟用PB15的防彈跳功能（DBEN: Debounce Enable）
  PB->DBEN |= (1 << 15);

  // 設定防彈跳時鐘源為LIRC（低頻內部振盪器），防彈跳時間為64個時鐘週期
  // 這可以過濾掉按鈕按下時的機械震動
  GPIO_SET_DEBOUNCE_TIME(GPIO_DBCLKSRC_LIRC, GPIO_DBCLKSEL_64);
}

// ==========================================
//              LED控制函數
// ==========================================
/**
 * @brief 初始化LED腳位（PC12~PC15）
 * @note LED為共陽極連接，輸出1（高電位）時LED熄滅，輸出0（低電位）時LED點亮
 */
void init_LED(void) {
  // 設定PC12~PC15為輸出模式
  GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
  GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);
  GPIO_SetMode(PC, BIT14, GPIO_MODE_OUTPUT);
  GPIO_SetMode(PC, BIT15, GPIO_MODE_OUTPUT);

  // 初始化所有LED為熄滅狀態（高電位）
  PC12 = PC13 = PC14 = PC15 = 1;
}

/**
 * @brief 點亮指定索引的LED
 * @param idx LED索引（0-3），對應PC12-15
 * @note 只有對應索引的LED點亮（低電位），其他LED熄滅（高電位）
 */
void LED_On(int idx) {
  // 根據索引點亮對應的LED（輸出0），其他LED熄滅（輸出1）
  PC12 = (idx == 0 ? 0 : 1); // idx=0時點亮PC12
  PC13 = (idx == 1 ? 0 : 1); // idx=1時點亮PC13
  PC14 = (idx == 2 ? 0 : 1); // idx=2時點亮PC14
  PC15 = (idx == 3 ? 0 : 1); // idx=3時點亮PC15
}

/**
 * @brief 關閉所有LED
 * @note 將所有LED腳位設為高電位（熄滅狀態）
 */
void LED_OffAll(void) {
  // 所有LED輸出高電位（熄滅）
  PC12 = PC13 = PC14 = PC15 = 1;
}

// ==========================================
//              LCD繪圖函數
// ==========================================
/**
 * @brief 繪製所有移動物件到LCD
 * @note 清除整個LCD畫面，然後在對應位置顯示4個數字
 * @note 每個數字垂直間距16像素，使用8x16字元顯示（printC函數）
 * @note 與Q1.c的差異：使用printC而非printC_5x7，字元尺寸為8x16像素
 */
void draw_all(void) {
  int i;

  // 清除整個LCD畫面
  clear_LCD();

  // 繪製4個移動物件
  for (i = 0; i < 4; i++) {
    // 在座標(obj[i].x, i*16)位置顯示數字
    // obj[i].num + '0' 將數字轉換為ASCII字元
    // i*16 確保每個數字垂直間距16像素（0, 16, 32, 48）
    // 使用printC函數（8x16像素字元），而非printC_5x7（5x7像素字元）
    printC(obj[i].x, i * 16, obj[i].num + '0');
  }
}

/**
 * @brief 檢查所有物件是否都已到達終點
 * @return 1表示所有物件都已到達，0表示還有物件未到達
 */
int all_done(void) {
  int i;
  // 檢查所有物件的reached旗標
  for (i = 0; i < 4; i++)
    if (!obj[i].reached)
      return 0; // 如果有任何物件未到達，返回0
  return 1;     // 所有物件都已到達
}

// ==========================================
//              數字產生與速度分配
// ==========================================
/**
 * @brief 產生4個不重複的隨機數字並分配移動速度
 * @note 步驟1：產生4個1-9的不重複隨機數字
 * @note 步驟2：複製數字陣列用於排序
 * @note 步驟3：將數字由大到小排序
 * @note 步驟4：根據數字大小分配速度（大數字速度快）
 *
 * 速度分配規則：
 * - 最大數字：速度 = 8 像素/次
 * - 第二大數字：速度 = 6 像素/次
 * - 第三大數字：速度 = 4 像素/次
 * - 最小數字：速度 = 2 像素/次
 */
void generate_numbers(void) {
  int used[10] = {0}; // 標記陣列，記錄已使用的數字（索引1-9）
  int numbers[4];     // 儲存產生的4個數字
  int sorted[4];      // 用於排序的數字陣列
  int i, j, temp;

  // ========== 步驟1：產生4個不重複的隨機數字 ==========
  for (i = 0; i < 4; i++) {
    int r;
    // 持續產生隨機數直到得到未使用的數字
    do {
      r = rand() % 9 + 1; // 產生1-9的隨機數
    } while (used[r]); // 如果數字已使用，重新產生

    // 標記數字為已使用
    used[r] = 1;
    numbers[i] = r;

    // 初始化物件資料
    obj[i].num = r;     // 設定數字值
    obj[i].x = 0;       // 重置X座標為起始位置（最左側）
    obj[i].reached = 0; // 重置到達旗標
  }

  // ========== 步驟2：複製數字陣列用於排序 ==========
  for (i = 0; i < 4; i++)
    sorted[i] = numbers[i];

  // ========== 步驟3：氣泡排序（由大到小） ==========
  // 外層迴圈：進行3輪比較（4個數字需要3輪）
  for (i = 0; i < 3; i++) {
    // 內層迴圈：比較剩餘未排序的數字
    for (j = i + 1; j < 4; j++) {
      // 如果後面的數字比前面的數字大，則交換
      if (sorted[j] > sorted[i]) {
        temp = sorted[j];
        sorted[j] = sorted[i];
        sorted[i] = temp;
      }
    }
  }
  // 排序完成後，sorted[0]為最大數字，sorted[3]為最小數字

  // ========== 步驟4：根據數字大小分配速度 ==========
  for (i = 0; i < 4; i++) {
    // 根據物件數字在排序後的位置分配速度
    if (obj[i].num == sorted[0]) // 最大數字
      obj[i].speed = 8;
    else if (obj[i].num == sorted[1]) // 第二大數字
      obj[i].speed = 6;
    else if (obj[i].num == sorted[2]) // 第三大數字
      obj[i].speed = 4;
    else // 最小數字
      obj[i].speed = 2;
  }
}

// ==========================================
//              主程式
// ==========================================
/**
 * @brief 主程式：數字競賽遊戲（Lance版本）
 * @note 遊戲流程：
 *       1. 產生4個不重複的隨機數字
 *       2. 根據數字大小分配移動速度
 *       3. 等待外部中斷按鈕啟動
 *       4. 數字開始移動，第一個到達終點的觸發LED
 *       5. 所有數字到達後等待按鍵繼續下一輪
 *
 * @note 與Q1.c的差異：
 *       - 使用printC函數（8x16像素字元）而非printC_5x7（5x7像素字元）
 *       - 終點位置為120（128-8）而非122（128-6）
 *       - 其他遊戲機制完全相同
 */
int main(void) {
  int i;
  int led_shown = 0; // LED顯示旗標，確保只有第一個到達者觸發LED

  // ========== 系統初始化 ==========
  SYS_Init();   // 系統時鐘和基本設定初始化
  init_LCD();   // LCD顯示器初始化
  clear_LCD();  // 清除LCD畫面
  OpenKeyPad(); // 按鍵矩陣初始化
  init_LED();   // LED腳位初始化
  init_EINT1(); // 外部中斷1（PB15）初始化

  // ========== 初始狀態設定 ==========
  LED_OffAll(); // 關閉所有LED
  srand(1234);  // 設定隨機數種子（固定種子可重現結果）

  // ========== 主遊戲迴圈 ==========
  while (1) {
    // 重置LED顯示旗標
    led_shown = 0;

    // 產生4個不重複的隨機數字並分配速度
    generate_numbers();

    // 繪製初始位置（所有數字在X=0位置）
    draw_all();

    // 重置啟動旗標，等待外部中斷
    start_flag = 0;

    // ========== 等待外部中斷按鈕啟動 ==========
    // 當PB15按鈕按下時，EINT1_IRQHandler會設定start_flag=1
    while (!start_flag)
      ;

    // ========== 競賽移動迴圈 ==========
    while (1) {
      // 更新每個物件的位置
      for (i = 0; i < 4; i++) {
        // 只處理尚未到達終點的物件
        if (!obj[i].reached) {
          // 根據速度更新X座標
          obj[i].x += obj[i].speed;

          // 檢查是否到達終點
          if (obj[i].x >= RIGHT_BOUND) {
            // 限制X座標不超過終點
            obj[i].x = RIGHT_BOUND;
            // 標記為已到達
            obj[i].reached = 1;

            // 只有第一個到達終點的物件會觸發LED
            // 這確保即使多個物件同時到達，也只有第一個會顯示LED
            if (!led_shown) {
              LED_On(i);     // 點亮對應索引的LED
              led_shown = 1; // 設定旗標，防止後續到達者觸發LED
            }
          }
        }
      }

      // 更新LCD顯示（繪製所有物件的新位置）
      draw_all();

      // 延遲200毫秒，控制移動速度
      Delay_ms(200);

      // 檢查是否所有物件都已到達終點
      if (all_done())
        break; // 跳出移動迴圈
    }

    // ========== 等待按鍵繼續下一輪 ==========
    // 所有物件到達後，等待使用者按下任意按鍵
    while (ScanKey() == 0)
      ;

    // 關閉所有LED，準備下一輪
    LED_OffAll();
  }
}
