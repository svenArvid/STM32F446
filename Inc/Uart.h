/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UART_H
#define __UART_H

#include <stdarg.h>
#include <string.h>
#include "ProjectDefs.h"
#include "stdio.h"

#define USART3_BUFF_SIZE  8192
#define USART3_BUFF_END_INDX  (USART3_BUFF_SIZE - 16)  

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define UART_PRINTF(...)  Uart_printf(__FILENAME__, __LINE__, __VA_ARGS__)
/*
#define UART_PRINTF(...)  \
do { \
  if ( TerminalPort.Tx.Indx < USART3_BUFF_END_INDX ) { \
    TerminalPort.Tx.Indx += snprintf(TerminalPort.Tx.Buffer + TerminalPort.Tx.Indx, (USART3_BUFF_END_INDX - TerminalPort.Tx.Indx), __VA_ARGS__); \
    if ( TerminalPort.Tx.Indx >= USART3_BUFF_END_INDX ) \
      TerminalPort.Tx.Indx += snprintf(TerminalPort.Tx.Buffer + TerminalPort.Tx.Indx, 16, "\r\nBUFFER_FULL\r\n"); \
  } \
} while (0)
*/
typedef struct {
  uint8_t  *Buffer;
  uint16_t Size;
  uint16_t Indx;
} Buffer_t;

typedef struct {
  USART_TypeDef *Usart;
  DMA_Stream_TypeDef *DMAStream_Rx;
  DMA_Stream_TypeDef *DMAStream_Tx;
  Buffer_t Rx;
  Buffer_t Tx;
} UartPort;


UartPort TerminalPort;
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
bool Uart_TerminalBufferEmpty(void);
void Uart_PrintRegisters(void);

void Uart_printf(const char *SourceFilename, int SourceLineno, const char *CFormatString, ...);

#endif // __UART_H
