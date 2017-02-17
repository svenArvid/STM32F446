/**
******************************************************************************
* @file    /Src/TicToc.c
* @author  Joakim Carlsson
* @version V1.0
* @date    20-Jan-2017
* @brief   Use Timer 2 to measure elapsed time.  Timer 2 is a 32-bit counter which is setup in InputCapture.c
*
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "ErrorHandler.h"
#include "ProjectDefs.h"
#include "TicToc.h"
#include "Uart.h"

#ifdef TIC_TOC  // Complete file in the #define

volatile uint32_t TicToc_Tim13IrqStart;
volatile uint32_t TicToc_Tim13IrqEnd;


// Use this function to print out time measurement data for testing/debugging
void TicToc_20ms(void)
{
/*
  uint32_t CurrTick;
  static uint32_t LastTick = 0;
  int32_t StrLength;

  CurrTick = Timer2Handle.Instance->CNT;
  StrLength = sprintf(USART3_TxBuff, "CurrTick:\t%lu, LastTick:\t%lu, Elapsed Time:\t%lu\n", CurrTick, LastTick, CurrTick-LastTick );

//  StrLength = sprintf(USART3_TxBuff, "Tim13IrqStart:\t%lu, Tim13IrqEnd:\t%lu, Elapsed Time:\t%lu\n", 
//              TicToc_Tim13IrqStart, TicToc_Tim13IrqEnd, TicToc_Tim13IrqEnd - TicToc_Tim13IrqStart );

  // Print to Terminal 
  HAL_UART_DMAStop(&USART3Handle);
  HAL_UART_Transmit_DMA(&USART3Handle, USART3_TxBuff, StrLength);

  LastTick = CurrTick;
*/
}




#endif