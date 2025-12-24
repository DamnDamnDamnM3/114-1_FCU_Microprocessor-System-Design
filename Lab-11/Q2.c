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

uint16_t music[72] = { // 給愛麗絲
	E6 ,D6u,E6 ,D6u,E6 ,B5 ,D6 ,C6 ,A5 ,A5 , 0 , 0 ,
	C5 ,E5 ,A5 ,B5 ,B5 , 0 ,C5 ,A5 ,B5 ,C6 ,C6 , 0 ,
	E6 ,D6u,E6 ,D6u,E6 ,B5 ,D6 ,C6 ,A5 ,A5 , 0 , 0 ,
	C5 ,E5 ,A5 ,B5 ,B5 , 0 ,E5 ,C6 ,B5 ,A5 ,A5 , 0 ,
	B5 ,C6 ,D6 ,E6 ,E6 , 0 ,G5 ,F6 ,E6 ,D6 ,D6 , 0 ,
	F5 ,E6 ,D6 ,C6 ,C6 , 0 ,E5 ,D6 ,C6 ,B5 ,B5 , 0 };

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



	


volatile uint32_t digit[4] = {0,0,0,0};
int cnt_5ms=0;


volatile uint8_t u8ADF=0;
int scaled_ADF=0;
uint32_t u32ADCvalue=0;
int song_index=0;

void ADC_IRQHandler(void) // 從 adc的程式抓過來的
{
    uint32_t u32Flag;

    // Get ADC conversion finish interrupt flag
    u32Flag = ADC_GET_INT_FLAG(ADC, ADC_ADF_INT);

    if(u32Flag & ADC_ADF_INT)
        u8ADF = 1;

    ADC_CLR_INT_FLAG(ADC, u32Flag);
}

void Init_ADC(void)
{
    ADC_Open(ADC, ADC_INPUT_MODE, ADC_OPERATION_MODE, ADC_CHANNEL_MASK);
    ADC_POWER_ON(ADC);
    ADC_EnableInt(ADC, ADC_ADF_INT);
    NVIC_EnableIRQ(ADC_IRQn);
}

void play_ALICE(void) { //愛麗絲
	for (song_index=0; song_index<72; song_index++) {
		
		
			ADC_START_CONV(ADC); // ADC
      //while (u8ADF == 0);
      		u32ADCvalue = ADC_GET_CONVERSION_DATA(ADC, 7);
			scaled_ADF = u32ADCvalue/409.6;
		
			PWM_ConfigOutputChannel(PWM1, PWM_CH0, music[song_index], 100-scaled_ADF); // 100 為完全靜音，90剛剛好，90以下的聲音會太大
		
		
		
			if (music[song_index]!=0) PWM_EnableOutput(PWM1, PWM_CH_0_MASK);
			else             PWM_DisableOutput(PWM1, PWM_CH_0_MASK);
			
			ADC_DisableInt(ADC, ADC_ADF_INT);
		  CLK_SysTickDelay(pitch[song_index]);
    }
	song_index=0;
}

void play_STAR(void) { //小星星
	for (song_index=0; song_index<144; song_index++) {
		
		
			ADC_START_CONV(ADC);
      //while (u8ADF == 0);
      u32ADCvalue = ADC_GET_CONVERSION_DATA(ADC, 7);
			scaled_ADF = u32ADCvalue/409.6;
		
		
			PWM_ConfigOutputChannel(PWM1, PWM_CH0, star_music[song_index], 100-scaled_ADF); // 0=Buzzer ON
		
			if (star_music[song_index]!=0) PWM_EnableOutput(PWM1, PWM_CH_0_MASK);
			else             PWM_DisableOutput(PWM1, PWM_CH_0_MASK);
		  CLK_SysTickDelay(star_pitch[song_index]);
    }
	song_index=0;
}

void play_TEST(void) { //nokia 3310
	for (song_index=0; song_index<48; song_index++) {
		
		
			ADC_START_CONV(ADC);
      //while (u8ADF == 0);
      u32ADCvalue = ADC_GET_CONVERSION_DATA(ADC, 7);
			scaled_ADF = u32ADCvalue/409.6;
		
		
			PWM_ConfigOutputChannel(PWM1, PWM_CH0, test_music[song_index], 80); // 0=Buzzer ON
		
			if (test_music[song_index]!=0) PWM_EnableOutput(PWM1, PWM_CH_0_MASK);
			else             PWM_DisableOutput(PWM1, PWM_CH_0_MASK);
		  CLK_SysTickDelay(test_pitch[song_index]);
    }
	song_index=0;
}

int32_t main (void)
{
  	int k;
    SYS_Init();
    Init_ADC();  
	
    u8ADF = 0; 
    PWM_EnableOutput(PWM1, PWM_CH_0_MASK);
    PWM_Start(PWM1, PWM_CH_0_MASK);	
	
		while(1) {
			k = ScanKey();

			/* 
			不在主程式取可變電阻的值而是在副程式迴圈裡面才取是因為
			如果只在這裡拿可變電阻的值，那執行到副程式的時候卡在迴圈裡面播放，會取不到值，所以執行時轉可變電阻不會有反應

			改成放在副程式的迴圈裡面取值就能做到邊放邊改的效果，雖然看起來有點屎山代碼 一長串 但至少能正常運作

			ADC_START_CONV(ADC);
      		while (u8ADF == 0);
      		u32ADCvalue = ADC_GET_CONVERSION_DATA(ADC, 7);
			scaled_ADF = u32ADCvalue/40.96;
			
			printf("%d\n", u32ADCvalue);
			printf("scaled: %d\n", scaled_ADF);
			*/

			switch (k) {
				case 5:
					play_ALICE();
					break;
				case 6:
					play_STAR();
					break;
				case 4:
					play_TEST(); //沒時間做bad apple
					break;
				default:
					break;
			}
			
		}
		
}
