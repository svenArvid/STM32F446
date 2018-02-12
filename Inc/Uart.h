/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UART_H
#define __UART_H

#include "ProjectDefs.h"
#include "stdio.h"

#define USART3_BUFF_SIZE  2048
#define USART6_BUFF_SIZE  1024

#define UART_PRINTF(...)  { USART3_TxBuffIndex += sprintf(USART3_TxBuff + USART3_TxBuffIndex, __VA_ARGS__); }

typedef struct {
  uint8_t  *Buffer;
  uint16_t Size;
} Buffer_t;

typedef struct {
  USART_TypeDef *Usart;
  DMA_Stream_TypeDef *DMAStream_Rx;
  DMA_Stream_TypeDef *DMAStream_Tx;
  Buffer_t Rx;
  Buffer_t Tx;
} UartPort;

UART_HandleTypeDef USART3Handle;

uint8_t USART3_TxBuff[USART3_BUFF_SIZE];
uint8_t USART6_TxBuff[USART6_BUFF_SIZE];

uint16_t USART3_TxBuffIndex;

UartPort ModbusPort;

void Uart_Init(void);
void Uart_20ms(void);

uint16_t Uart_MessageReceived(UartPort *Port);
void Uart_StopReceiver(UartPort *Port);
void Uart_StartReceiver(UartPort *Port);
bool Uart_TransmissionComplete(UartPort *Port);
void Uart_StopTransmitter(UartPort *Port);
void Uart_StartTransmitter(UartPort *Port, uint16_t BytesToSend);

void Uart_TransmitTerminalBuffer(void);
void Uart_PrintToTerminal(void);

#endif // __UART_H
