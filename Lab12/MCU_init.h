//Define Clock source
#define MCU_CLOCK_SOURCE      
#define MCU_CLOCK_SOURCE_HXT  // HXT, LXT, HIRC, LIRC
#define MCU_CLOCK_FREQUENCY   50000000  //Hz

//Define MCU Interfaces
#define MCU_INTERFACE_SPI3
#define SPI3_CLOCK_SOURCE_HCLK // HCLK, PLL
#define PIN_SPI3_SS0_PD8
#define PIN_SPI3_SCLK_PD9
#define PIN_SPI3_MISO0_PD10
#define PIN_SPI3_MOSI0_PD11

// ... Your existing SPI3 definitions ...

// --- Add these for Lab 12-1 (1A2B) ---

// 1. Enable UART1 for Communication
#define MCU_INTERFACE_UART1
#define UART_CLOCK_SOURCE_HXT   // Use External Crystal (12MHz) for stable baud rate
#define UART_CLOCK_DIVIDER      1
#define PIN_UART1_RX_PB4        // Default RX Pin on NUC140 Learning Board
#define PIN_UART1_TX_PB5        // Default TX Pin on NUC140 Learning Board

// 2. Enable Timer0 for Keypad Scanning (200ms)
#define MCU_INTERFACE_TMR0
#define TMR0_CLOCK_SOURCE_HXT   // Use HXT for accurate timing

// 3. Enable Timer1 for 7-Segment Multiplexing (5ms)
#define MCU_INTERFACE_TMR1
#define TMR1_CLOCK_SOURCE_HXT
