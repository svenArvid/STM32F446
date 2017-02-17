/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UART_H
#define __UART_H

#include "stm32f4xx_hal.h"
#include "stdio.h"

#define USART3_TX_BUFF_SIZE 1024

extern UART_HandleTypeDef USART3Handle;
extern uint8_t USART3_TxBuff[USART3_TX_BUFF_SIZE];


extern void Uart_Init(void);

#endif // __UART_H
