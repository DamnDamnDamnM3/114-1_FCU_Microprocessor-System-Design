//
// PWM_Music : PWM0 Ch0 (PA12) output music "Fur Elise"
//
// EVB : Nu-LB-NUC140
// MCU : NUC140VE3CN  (LQPF-100)
// Buzzer: used as speaker

// PA12/PWM0 connected to PB11
#include <stdio.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "Note_Freq.h"
#include "ScanKey.h"

#define  P125ms 125000
#define  P250ms 250000
#define  P500ms 500000
#define  P1S   1000000

// 給愛麗絲 (Fur Elise) 音樂資料（72個音符）
// 音符定義：E6=高音Mi, D6u=高音Re降, B5=Si, C6=高音Do, A5=La, 0=休止符
uint16_t music[72] = { 
	E6 ,D6u,E6 ,D6u,E6 ,B5 ,D6 ,C6 ,A5 ,A5 , 0 , 0 ,
	C5 ,E5 ,A5 ,B5 ,B5 , 0 ,C5 ,A5 ,B5 ,C6 ,C6 , 0 ,
	E6 ,D6u,E6 ,D6u,E6 ,B5 ,D6 ,C6 ,A5 ,A5 , 0 , 0 ,
	C5 ,E5 ,A5 ,B5 ,B5 , 0 ,E5 ,C6 ,B5 ,A5 ,A5 , 0 ,
	B5 ,C6 ,D6 ,E6 ,E6 , 0 ,G5 ,F6 ,E6 ,D6 ,D6 , 0 ,
	F5 ,E6 ,D6 ,C6 ,C6 , 0 ,E5 ,D6 ,C6 ,B5 ,B5 , 0 
};

uint16_t test_music[58] = { // nokia 3310
    NULL, E5,    D5,     F4u, F4u,   G4u,G4u,
     C5u,   B4,    D4,D4,    E4,E4,

     B4,   A4,    C4u,C4u,   E4,E4,
     A4,   A4, A4
};

uint16_t test2_music[58] = {
    NULL,   D2u,D2u,D2u, G5,G5, F4,F4,A6,  G5,G5,G5,G5};


	uint16_t star_music[144] = { //小星星
		
		
    0, C4,0, C4,0, G4,0, G4,0, A4,0, A4,0, G4, 0, 0,
    F4,0, F4,0, E4,0, E4,0, D4,0, D4,0, C4,0, 0 ,0,
    G4,0, G4,0, F4,0, F4,0, E4,0, E4,0, D4,0, 0 ,0,
    G4,0, G4,0, F4,0, F4,0, E4,0, E4,0, D4,0, 0 ,0,
    C4,0, C4,0, G4,0, G4,0, A4,0, A4,0, G4,0, 0, 0,
    F4,0, F4,0, E4,0, E4,0, D4,0, D4,0, C4,0};
	
  uint32_t pitch[72] = {
	P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms,
	P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms,
	P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms,
	P500ms, P500ms, P500ms, P500ms,	P500ms, P500ms, P500ms, P500ms,	P500ms, P500ms, P500ms, P500ms,
	P500ms, P500ms, P500ms, P500ms,	P500ms, P500ms, P500ms, P500ms,	P500ms, P500ms, P500ms, P500ms,
	P500ms, P500ms, P500ms, P500ms,	P500ms, P500ms, P500ms, P500ms,	P500ms, P500ms, P500ms, P500ms
	};

	uint32_t star_pitch[144] = {
	P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms,
	P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms,
	P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms,
	P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms,
	P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms,
	P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms};
uint32_t test_pitch[72] = {
	P125ms, P125ms, P125ms, P125ms,	P125ms, P125ms, P125ms, P125ms, P125ms, P125ms, P125ms, P125ms,	P125ms, P125ms, P125ms, P125ms,
	P125ms, P125ms, P125ms, P125ms,	P125ms, P125ms, P125ms, P125ms, P125ms, P125ms, P125ms, P125ms,	P125ms, P125ms, P125ms, P125ms,
	P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms};

uint32_t test2_pitch[72] = { //test
	P500ms, P500ms, P500ms, P500ms,	P500ms, P500ms, P500ms, P500ms,	P500ms, P500ms, P500ms, P500ms,
	P125ms, P125ms, P125ms, P125ms,	P125ms, P125ms, P125ms, P125ms, P125ms, P125ms, P125ms, P125ms,	P125ms, P125ms, P125ms, P125ms,
	P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms,	P250ms, P250ms, P250ms, P250ms};



	


// 全域變數（未使用，保留供擴展）
volatile uint32_t digit[4] = {0,0,0,0};  // 七段顯示器數字緩衝區
int cnt_5ms=0;                            // 5毫秒計數器

// ADC相關變數
volatile uint8_t u8ADF=0;                 // ADC轉換完成標記
int scaled_ADF=0;                        // 縮放後的ADC值（音量值）
uint32_t u32ADCvalue=0;                  // ADC原始讀取值
int song_index=0;                        // 音樂播放索引

/*
 * ================================================================
 * ADC中斷處理函數
 * 功能：處理ADC轉換完成中斷
 * ================================================================
 */
void ADC_IRQHandler(void)
{
    uint32_t u32Flag;

    // 取得ADC轉換完成中斷標記
    u32Flag = ADC_GET_INT_FLAG(ADC, ADC_ADF_INT);

    // 如果ADC轉換完成，設定標記
    if(u32Flag & ADC_ADF_INT)
        u8ADF = 1;

    // 清除ADC中斷標記
    ADC_CLR_INT_FLAG(ADC, u32Flag);
}

/*
 * ================================================================
 * ADC初始化函數
 * 功能：初始化ADC模組，用於讀取可變電阻值（音量控制）
 * ================================================================
 */
void Init_ADC(void)
{
    ADC_Open(ADC, ADC_INPUT_MODE, ADC_OPERATION_MODE, ADC_CHANNEL_MASK);  // 開啟ADC
    ADC_POWER_ON(ADC);                    // 開啟ADC電源
    ADC_EnableInt(ADC, ADC_ADF_INT);     // 啟用ADC中斷
    NVIC_EnableIRQ(ADC_IRQn);             // 啟用NVIC中斷
}

/*
 * ================================================================
 * 播放給愛麗絲音樂
 * 功能：播放給愛麗絲音樂，並即時調整音量
 * 說明：在播放迴圈中即時讀取ADC值以實現動態音量控制
 * ================================================================
 */
void play_ALICE(void) {
	for (song_index=0; song_index<72; song_index++) {
		// 啟動ADC轉換（讀取可變電阻值）
		ADC_START_CONV(ADC);
		// 注意：不使用while等待，直接讀取上次轉換結果（非阻塞式）
		
		// 讀取ADC通道7的轉換結果（0-4095）
		u32ADCvalue = ADC_GET_CONVERSION_DATA(ADC, 7);
		// 將ADC值縮放到音量範圍（0-100）
		// 公式：scaled_ADF = ADC_value / 409.6
		// 結果範圍：0-100（0為最大音量，100為靜音）
		scaled_ADF = u32ADCvalue/409.6;
		
		// 設定PWM頻率和占空比
		// 頻率：music[song_index]（音符頻率）
		// 占空比：100-scaled_ADF（音量控制，100為完全靜音）
		PWM_ConfigOutputChannel(PWM1, PWM_CH0, music[song_index], 100-scaled_ADF);
		
		// 如果音符不為0（非休止符），啟用PWM輸出
		if (music[song_index]!=0) {
			PWM_EnableOutput(PWM1, PWM_CH_0_MASK);
		}
		else {
			// 休止符時關閉PWM輸出
			PWM_DisableOutput(PWM1, PWM_CH_0_MASK);
		}
		
		// 暫時關閉ADC中斷（避免衝突）
		ADC_DisableInt(ADC, ADC_ADF_INT);
		// 延遲對應的節拍時間
		CLK_SysTickDelay(pitch[song_index]);
	}
	song_index=0;  // 重置播放索引
}

/*
 * ================================================================
 * 播放小星星音樂
 * 功能：播放小星星音樂，並即時調整音量
 * ================================================================
 */
void play_STAR(void) {
	for (song_index=0; song_index<144; song_index++) {
		// 啟動ADC轉換（讀取可變電阻值）
		ADC_START_CONV(ADC);
		// 讀取ADC通道7的轉換結果
		u32ADCvalue = ADC_GET_CONVERSION_DATA(ADC, 7);
		// 將ADC值縮放到音量範圍（0-100）
		scaled_ADF = u32ADCvalue/409.6;
		
		// 設定PWM頻率和占空比（動態音量控制）
		PWM_ConfigOutputChannel(PWM1, PWM_CH0, star_music[song_index], 100-scaled_ADF);
		
		// 如果音符不為0，啟用PWM輸出
		if (star_music[song_index]!=0) {
			PWM_EnableOutput(PWM1, PWM_CH_0_MASK);
		}
		else {
			// 休止符時關閉PWM輸出
			PWM_DisableOutput(PWM1, PWM_CH_0_MASK);
		}
		// 延遲對應的節拍時間
		CLK_SysTickDelay(star_pitch[song_index]);
	}
	song_index=0;  // 重置播放索引
}

/*
 * ================================================================
 * 播放Nokia 3310音樂
 * 功能：播放Nokia 3310經典鈴聲
 * 說明：使用固定音量（80）而非動態調整
 * ================================================================
 */
void play_TEST(void) {
	for (song_index=0; song_index<48; song_index++) {
		// 啟動ADC轉換（雖然此函數使用固定音量，但仍讀取ADC以保持一致性）
		ADC_START_CONV(ADC);
		// 讀取ADC通道7的轉換結果
		u32ADCvalue = ADC_GET_CONVERSION_DATA(ADC, 7);
		// 將ADC值縮放到音量範圍（此處未使用）
		scaled_ADF = u32ADCvalue/409.6;
		
		// 設定PWM頻率和固定占空比（80為固定音量）
		PWM_ConfigOutputChannel(PWM1, PWM_CH0, test_music[song_index], 80);
		
		// 如果音符不為0，啟用PWM輸出
		if (test_music[song_index]!=0) {
			PWM_EnableOutput(PWM1, PWM_CH_0_MASK);
		}
		else {
			// 休止符時關閉PWM輸出
			PWM_DisableOutput(PWM1, PWM_CH_0_MASK);
		}
		// 延遲對應的節拍時間
		CLK_SysTickDelay(test_pitch[song_index]);
	}
	song_index=0;  // 重置播放索引
}

/*
 * ================================================================
 * 主程式
 * 功能：系統初始化和主迴圈，處理按鍵選擇音樂
 * ================================================================
 */
int32_t main (void)
{
	int k;                      // 按鍵掃描結果
	
	// ================================================================
	// 系統初始化
	// ================================================================
	SYS_Init();                 // 系統初始化
	Init_ADC();                  // ADC初始化（音量控制）
	
	// ================================================================
	// PWM初始化
	// ================================================================
	u8ADF = 0;                  // 清除ADC標記
	PWM_EnableOutput(PWM1, PWM_CH_0_MASK);  // 啟用PWM1通道0輸出
	PWM_Start(PWM1, PWM_CH_0_MASK);          // 啟動PWM1
	
	// ================================================================
	// 主程式迴圈
	// ================================================================
	while(1) {
		k = ScanKey();          // 掃描按鍵

		/* 
		 * 設計說明：為什麼在播放函數中讀取ADC值？
		 * 
		 * 如果只在主程式讀取可變電阻的值，當執行到播放函數時，
		 * 程式會卡在播放迴圈中，無法即時讀取ADC值。
		 * 因此轉動可變電阻時不會有反應。
		 * 
		 * 解決方案：將ADC讀取放在播放函數的迴圈中，
		 * 這樣可以在播放過程中即時讀取ADC值，實現動態音量調整。
		 * 雖然程式碼看起來較長，但能正常運作。
		 */

		// 根據按鍵選擇播放的音樂
		switch (k) {
			case 5:
				play_ALICE();   // 按鍵5：播放給愛麗絲
				break;
			case 6:
				play_STAR();    // 按鍵6：播放小星星
				break;
			case 4:
				play_TEST();    // 按鍵4：播放Nokia 3310
				break;
			default:
				break;          // 其他按鍵：無動作
		}
	}
}
