#include "debug_uart.h"
#include "stdio.h"

#ifdef DEBUG_UART
UART_HandleTypeDef debug_uart;
volatile uint8_t debug_level = 2;
spi_debug_count_t spi_debug_result;

HAL_StatusTypeDef DEBUG_UART_Init (USART_TypeDef* uart)
{
    debug_uart.Instance = uart;
    debug_uart.Init.BaudRate = 115200;
    debug_uart.Init.WordLength = UART_WORDLENGTH_8B;
    debug_uart.Init.StopBits = UART_STOPBITS_1;
    debug_uart.Init.Parity = UART_PARITY_NONE;
    debug_uart.Init.Mode = UART_MODE_TX_RX;
    debug_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    debug_uart.Init.OverSampling = UART_OVERSAMPLING_16;
    debug_uart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    debug_uart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_StatusTypeDef status = HAL_UART_Init (&debug_uart);
    LOG (LOG_LEVEL_2, "%s return %d\n", __func__, status);
    return status;
}

PUTCHAR_PROTOTYPE
{
    /* Place your implementation of fputc here */
    /* e.g. write a character to the USART1 and Loop until the end of transmission */
    HAL_UART_Transmit (&debug_uart, (uint8_t*) &ch, 1, 0xFFFF);
    
    return ch;
}
#endif /* DEBUG_UART */