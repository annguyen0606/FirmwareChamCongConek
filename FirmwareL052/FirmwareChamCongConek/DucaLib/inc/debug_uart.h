#ifndef __DEBUG_UART_H
#define __DEBUG_UART_H

#include "main.h"
#include "stdio.h"

#ifdef DEBUG_UART
    #define LOG_LEVEL_1 1U //main() LOG
    #define LOG_LEVEL_2 2U //connect module LOG
    #define LOG_LEVEL_3 3U // in-module LOG
    
    
    #ifdef __GNUC__
        /* With GCC, small printf (option LD Linker->Libraries->Small printf
        set to 'Yes') calls __io_putchar() */
        #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
    #else
        #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
    #endif /* __GNUC__ */
    HAL_StatusTypeDef DEBUG_UART_Init (USART_TypeDef* uart);
    extern  volatile  uint8_t debug_level;
    #define LOG(LEVEL,ARGS...) (LEVEL > debug_level)? (int)0: printf(ARGS)
#else
    #define LOG(LEVEL,ARGS...)
    #define DEBUG_UART_Init(uart)
#endif /* DEBUG_UART */
typedef struct _debug_spi_result_
{
  uint32_t spi_send_fail_count;
  uint32_t spi_send_succ_count;
  uint32_t spi_recv_fail_count;
  uint32_t spi_recv_succ_count;
  uint32_t spi_poll_fail_count;
  uint32_t spi_poll_succ_count;
}spi_debug_count_t;

extern spi_debug_count_t spi_debug_result;

#endif /* __DEBUG_UART_H */