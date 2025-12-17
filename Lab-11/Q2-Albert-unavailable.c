#include <stdio.h>
#include "NUC100Series.h"
#include "SYS_init.h"
#include "MCU_init.h"
#include "Scankey.h"
#include "PWM.h"
#include "ADC.h"

/* ============================================================
 * Musical note frequency definitions (Hz)
 * ============================================================ */
#define REST 0

#define C4  262
#define D4  294
#define E4  330
#define F4  349
#define G4  392
#define A4  440
#define B4  494

#define C5  523
#define D5  587
#define E5  659

/* ============================================================
 * "Twinkle Twinkle Little Star"
 * ============================================================ */
const uint16_t pitch_star[] = {
    C4,C4,G4,G4,A4,A4,G4,
    F4,F4,E4,E4,D4,D4,C4,
    G4,G4,F4,F4,E4,E4,D4,
    G4,G4,F4,F4,E4,E4,D4,
    C4,C4,G4,G4,A4,A4,G4,
    F4,F4,E4,E4,D4,D4,C4
};

const uint8_t beat_star[] = {
    1,1,1,1,1,1,2,
    1,1,1,1,1,1,2,
    1,1,1,1,1,1,2,
    1,1,1,1,1,1,2,
    1,1,1,1,1,1,2,
    1,1,1,1,1,1,2
};

/* ============================================================
 * "Für Elise" (main melody)
 * ============================================================ */
const uint16_t pitch_elise[] = {
    E5,D5,E5,D5,E5,B4,D5,C5,A4,
    REST,
    C4,E4,A4,B4,
    REST,
    E4,G4,B4,C5,
    REST,
    E4,E5,D5,E5,D5,E5,B4,D5,C5,A4
};

const uint8_t beat_elise[] = {
    1,1,1,1,1,1,1,1,2,
    1,
    1,1,1,2,
    1,
    1,1,1,2,
    1,
    1,1,1,1,1,1,1,1,2
};

/* ============================================================
 * Function prototypes
 * ============================================================ */
void PWM_Init(void);
void ADC_Init(void);
uint16_t GetVolume(void);
void PlaySong(const uint16_t*, const uint8_t*, uint32_t);

/* ============================================================
 * Initialize PWM0 Channel 0 for audio output
 * ============================================================ */
void PWM_Init(void)
{
    /* Configure PA12 as PWM0 output */
		SYS->GPA_MFP &= ~(0xF << 24);
		SYS->GPA_MFP |=  (0x2 << 24);

    PWM_ConfigOutputChannel(PWM0, PWM_CH0, 1000, 50);
    PWM_EnableOutput(PWM0, (1 << PWM_CH0));
    PWM_Start(PWM0, (1 << PWM_CH0));
}

/* ============================================================
 * Initialize ADC for volume control (ADC channel 7)
 * ============================================================ */
void ADC_Init(void)
{
    CLK_EnableModuleClock(ADC_MODULE);
    CLK_SetModuleClock(ADC_MODULE, 0, CLK_CLKDIV_ADC(8));

    ADC_Open(ADC,
             ADC_ADCR_DIFFEN_SINGLE_END,
             ADC_ADCR_ADMD_SINGLE,
             BIT7);
}

/* ============================================================
 * Read potentiometer value and convert to PWM duty (0~255)
 * ============================================================ */
uint16_t GetVolume(void)
{
    ADC_START_CONV(ADC);
    while (!ADC_GET_INT_FLAG(ADC, ADC_ADF_INT));
    ADC_CLR_INT_FLAG(ADC, ADC_ADF_INT);

    return ADC_GET_CONVERSION_DATA(ADC, 7) >> 4;
}

/* ============================================================
 * Play a song using PWM
 * ============================================================ */
void PlaySong(const uint16_t *pitch,
              const uint8_t *beat,
              uint32_t length)
{
    uint32_t i;
    uint16_t volume;

    for (i = 0; i < length; i++) {

        volume = GetVolume();

        if (pitch[i] == REST) {
            PWM_Stop(PWM0, (1 << PWM_CH0));
        } else {
            PWM_ConfigOutputChannel(PWM0,
                                    PWM_CH0,
                                    pitch[i],
                                    volume);
            PWM_EnableOutput(PWM0, (1 << PWM_CH0));
            PWM_Start(PWM0, (1 << PWM_CH0));
        }

        CLK_SysTickDelay(200000 * beat[i]);
    }

    PWM_Stop(PWM0, (1 << PWM_CH0));
}

/* ============================================================
 * Main function
 * ============================================================ */
int main(void)
{
    uint8_t keyin;
    uint8_t last_key = 0;

    SYS_Init();
    OpenKeyPad();
    PWM_Init();
    ADC_Init();

    while (1) {
        keyin = ScanKey();

        if (keyin != 0 && keyin != last_key) {

            if (keyin == 5) {
                PlaySong(pitch_star,
                         beat_star,
                         sizeof(pitch_star) / sizeof(uint16_t));
            }
            else if (keyin == 6) {
                PlaySong(pitch_elise,
                         beat_elise,
                         sizeof(pitch_elise) / sizeof(uint16_t));
            }
        }

        last_key = keyin;
    }
}
