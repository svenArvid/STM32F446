/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UART_H
#define __UART_H

#include "ProjectDefs.h"
#include "stdio.h"

#define USART3_BUFF_SIZE  2048
#define USART6_BUFF_SIZE  1024

#define UART_PRINTF(...)  USART3_TxBuffIndex += sprintf(USART3_TxBuff + USART3_TxBuffIndex, __VA_ARGS__)

UART_HandleTypeDef USART3Handle;
UART_HandleTypeDef USART6Handle;

uint8_t USART3_TxBuff[USART3_BUFF_SIZE];
uint8_t USART6_TxBuff[USART6_BUFF_SIZE];

uint16_t USART3_TxBuffIndex;

void Uart_Init(void);
void Uart_1ms(void);

void Uart_TransmitTerminalBuffer(void);

void Uart_PrintToTerminal(void);

#endif // __UART_H
