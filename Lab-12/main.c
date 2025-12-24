#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Scankey.h"
#include "Seven_Segment.h"

// --- Global Variables ---
volatile uint8_t secret[4] = {0,0,0,0};      
volatile uint8_t guess_buf[4];               
volatile uint8_t opponent_guess[4];          

volatile uint8_t g_u8Key_Flag = 0;           
volatile int g_i8Key_Index = 0;              
volatile int g_i8RX_Index = 0;               
volatile uint8_t g_u8RX_Complete = 0;        

volatile uint32_t cnt_5ms = 0;               

// --- History Buffers ---
char g_History[4][17] = {
    "                ", 
    "                ", 
    "                ", 
    "                "  
};

// --- Function Prototypes ---
extern void print_Line(int8_t line, char text[]); 
extern void init_LCD(void);
extern void clear_LCD(void);

void Init_Timer0(void);
void Init_Timer1(void);
void Init_UART1(void);
void Generate_Secret(void);
void Calculate_1A2B(volatile uint8_t *secret_arr, volatile uint8_t *guess_arr, int *A, int *B);
void Add_Result_To_History(volatile uint8_t *guess, int A, int B);

int main(void)
{
    int A, B;
    uint32_t i; 

    // 1. Initialization
    SYS_Init();
    Init_Timer0(); 
    Init_Timer1(); 
    Init_UART1();  
    
    init_LCD();
    clear_LCD();
    OpenKeyPad();
    
    // 2. Game Setup
    Generate_Secret(); 

    while(1)
    {
        // --- Task A: Process Keypad Input ---
        if (g_u8Key_Flag != 0) 
        {
            if (g_i8Key_Index < 4) {
                guess_buf[g_i8Key_Index] = g_u8Key_Flag;
                g_i8Key_Index++;
            }
            
            // If 4 digits entered, Send to Opponent
            if (g_i8Key_Index == 4) {
                for(i=0; i<4; i++) {
                     while((UART1->FSR & UART_FSR_TX_EMPTY_Msk) == 0); 
                     UART_WRITE(UART1, guess_buf[i]);
                }
                
                g_i8Key_Index = 0; 
            }
            
            g_u8Key_Flag = 0; 
        }

        // --- Task B: Process Received Guess ---
        if (g_u8RX_Complete == 1) 
        {
            Calculate_1A2B(secret, opponent_guess, &A, &B);
            Add_Result_To_History(opponent_guess, A, B);
            g_u8RX_Complete = 0; 
        }
    }
}

// --- Interrupt Service Routines ---

// UPDATED: Timer0 now runs faster (50Hz) for responsive keys
void TMR0_IRQHandler(void)
{
    static uint8_t u8LastKey = 0; // Remembers previous state
    uint8_t u8CurrentKey;

    TIMER_ClearIntFlag(TIMER0); 
    
    u8CurrentKey = ScanKey();
    
    // logic: Only register if Key is pressed NOW, but was NOT pressed before.
    // This prevents "1111" when holding the button.
    if (u8CurrentKey != 0 && u8LastKey == 0) {
        g_u8Key_Flag = u8CurrentKey; 
    }
    
    u8LastKey = u8CurrentKey; // Update history
}

void TMR1_IRQHandler(void)
{
    TIMER_ClearIntFlag(TIMER1);
    cnt_5ms++;
    
    if (cnt_5ms % 4 == 0) {
        CloseSevenSegment();
        ShowSevenSegment(3, secret[0]); 
    }
    else if (cnt_5ms % 4 == 1) {
        CloseSevenSegment();
        ShowSevenSegment(2, secret[1]); 
    }
    else if (cnt_5ms % 4 == 2) {
        CloseSevenSegment();
        ShowSevenSegment(1, secret[2]); 
    }
    else if (cnt_5ms % 4 == 3) {
        CloseSevenSegment();
        ShowSevenSegment(0, secret[3]); 
    }
}

void UART1_IRQHandler(void)
{
    uint32_t u32IntSts = UART1->ISR;
    uint8_t c; 
    
    if(u32IntSts & UART_ISR_RDA_INT_Msk) 
    {
        c = UART_READ(UART1);
        
        if (g_i8RX_Index < 4) {
            opponent_guess[g_i8RX_Index] = c;
            g_i8RX_Index++;
        }
        
        if (g_i8RX_Index == 4) {
            g_i8RX_Index = 0;
            g_u8RX_Complete = 1; 
        }
    }
}

// --- Initialization Functions ---

// UPDATED: Changed from 5Hz to 50Hz
void Init_Timer0(void) { 
    // 50Hz = Check keypad every 20ms (Much faster!)
    TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 50); 
    TIMER_EnableInt(TIMER0);
    NVIC_EnableIRQ(TMR0_IRQn);
    TIMER_Start(TIMER0);
}

void Init_Timer1(void) { 
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 200); 
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);
    TIMER_Start(TIMER1);
}

void Init_UART1(void) { 
    UART_Open(UART1, 115200);
    UART_EnableInt(UART1, UART_IER_RDA_IEN_Msk); 
    NVIC_EnableIRQ(UART1_IRQn);
}

// --- Game Logic ---

void Generate_Secret(void) {
    int i, j;
    int is_duplicate;
    
    for (i = 0; i < 4; i++) {
        do {
            is_duplicate = 0;
            secret[i] = rand() % 10; 
            
            for (j = 0; j < i; j++) {
                if (secret[i] == secret[j]) {
                    is_duplicate = 1;
                    break;
                }
            }
        } while (is_duplicate);
    }
}

void Calculate_1A2B(volatile uint8_t *secret_arr, volatile uint8_t *guess_arr, int *A, int *B) {
    int i, j;
    *A = 0;
    *B = 0;
    
    for (i = 0; i < 4; i++) {
        if (guess_arr[i] == secret_arr[i]) {
            (*A)++;
        } else {
            for (j = 0; j < 4; j++) {
                if (guess_arr[i] == secret_arr[j]) {
                    (*B)++;
                    break; 
                }
            }
        }
    }
}

void Add_Result_To_History(volatile uint8_t *guess, int A, int B) {
    strcpy(g_History[0], g_History[1]);
    strcpy(g_History[1], g_History[2]);
    strcpy(g_History[2], g_History[3]);

    sprintf(g_History[3], "%d%d%d%d %dA%dB", 
            guess[0], guess[1], guess[2], guess[3], A, B);

    clear_LCD();
    print_Line(0, g_History[0]);
    print_Line(1, g_History[1]);
    print_Line(2, g_History[2]);
    print_Line(3, g_History[3]);
}
