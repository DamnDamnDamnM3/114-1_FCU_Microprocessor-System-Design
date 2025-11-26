#include <stdio.h>
#include <stdlib.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Scankey.h"

#define RIGHT_BOUND 120 // 128 - 8 = 112

typedef struct {
    int num;
    int x;
    int speed;
    int reached;
} MOVING;

MOVING obj[4];
volatile int start_flag = 0;

// ---------------- Delay ----------------
void Delay_ms(int ms)
{
    int i;
    for(i = 0; i < ms; i++)
        CLK_SysTickDelay(1000);
}

// ----------- Interrupt Handler ----------
void EINT1_IRQHandler(void)
{
    start_flag = 1;

    PB->ISRC = (1 << 15);  
    NVIC_ClearPendingIRQ(EINT1_IRQn);
}

// ----------- Init EINT on PB15 ----------
void init_EINT1(void)
{
    GPIO_SetMode(PB, BIT15, GPIO_MODE_INPUT);
    GPIO_EnableInt(PB, 15, GPIO_INT_FALLING);
    NVIC_EnableIRQ(EINT1_IRQn);

    PB->DBEN |= (1 << 15);
    GPIO_SET_DEBOUNCE_TIME(GPIO_DBCLKSRC_LIRC, GPIO_DBCLKSEL_64);
}

// ----------- LED pins (PC12~PC15) ----------
void init_LED(void)
{
    GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT14, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT15, GPIO_MODE_OUTPUT);

    PC12 = PC13 = PC14 = PC15 = 1;
}

void LED_On(int idx)
{
    PC12 = (idx == 0 ? 0 : 1);
    PC13 = (idx == 1 ? 0 : 1);
    PC14 = (idx == 2 ? 0 : 1);
    PC15 = (idx == 3 ? 0 : 1);
}

void LED_OffAll(void)
{
    PC12 = PC13 = PC14 = PC15 = 1;
}

// ---------- LCD Draw ----------
void draw_all(void)
{
    int i;

    clear_LCD();
    for(i = 0; i < 4; i++)
        printC_5x7(obj[i].x, i * 16, obj[i].num + '0');
}

int all_done(void)
{
    int i;
    for(i = 0; i < 4; i++)
        if(!obj[i].reached) return 0;
    return 1;
}

// --------- Number Generate + Speed Mapping ---------
void generate_numbers(void)
{
    int used[10] = {0};
    int numbers[4];
    int sorted[4];
    int i, j, temp;

    // step1: four different numbers
    for(i = 0; i < 4; i++)
    {
        int r;
        do {
            r = rand() % 9 + 1;
        } while(used[r]);

        used[r] = 1;
        numbers[i] = r;
        obj[i].num = r;
        obj[i].x = 0;
        obj[i].reached = 0;
    }

    // step2: copy for sorting
    for(i = 0; i < 4; i++)
        sorted[i] = numbers[i];

    // step3: bubble sort (descending)
    for(i = 0; i < 3; i++)
    {
        for(j = i + 1; j < 4; j++)
        {
            if(sorted[j] > sorted[i])
            {
                temp = sorted[j];
                sorted[j] = sorted[i];
                sorted[i] = temp;
            }
        }
    }

    // step4: assign speed by size
    for(i = 0; i < 4; i++)
    {
        if(obj[i].num == sorted[0])
            obj[i].speed = 8;
        else if(obj[i].num == sorted[1])
            obj[i].speed = 6;
        else if(obj[i].num == sorted[2])
            obj[i].speed = 4;
        else
            obj[i].speed = 2;
    }
}

// ----------------------- MAIN LOOP -----------------------
int main(void)
{
    int i;
    int led_shown = 0;

    SYS_Init();
    init_LCD();
    clear_LCD();
    OpenKeyPad();
    init_LED();
    init_EINT1();

    LED_OffAll();
    srand(1234);

    while(1)
    {
        led_shown = 0;
        generate_numbers();
        draw_all();
        start_flag = 0;

        while(!start_flag);

        while(1)
        {
            for(i = 0; i < 4; i++)
            {
                if(!obj[i].reached)
                {
                    obj[i].x += obj[i].speed;

                    if(obj[i].x >= RIGHT_BOUND)
                    {
                        obj[i].x = RIGHT_BOUND;
                        obj[i].reached = 1;

                        // Only the FIRST arrival triggers LED
                        if(!led_shown)
                        {
                            LED_On(i);
                            led_shown = 1;
                        }
                    }
                }
            }

            draw_all();
            Delay_ms(200);

            if(all_done())
                break;
        }

        while(ScanKey() == 0);

        LED_OffAll();
    }
}
