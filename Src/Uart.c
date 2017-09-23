/**
******************************************************************************
* @file    /Src/Uart.c
* @author  Joakim Carlsson
* @version V1.0
* @date    14-Jan-2017
* @brief   USART3 Tx is setup and configured with DMA
* To print message do the following:
* Use sprintf to format text and put it in USART3_TxBuff (respect the size of the buffer USART3_TX_BUFF_SIZE)
* Then call HAL_UART_Transmit_DMA with uarthandle, TxBuff and length of message
* Example:
* StrLength = sprintf(USART3_TxBuff, "\n\r UART Sprintf Example: Buffer is printed to the UART using DMA. Print %ld\n\r", number);
* HAL_UART_Transmit_DMA(&USART3Handle, USART3_TxBuff, StrLength);
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "ErrorHandler.h"
#include "ProjectDefs.h"
#include "Uart.h"


/* UART handler declaration */
UART_HandleTypeDef USART3Handle;

/* Buffers */
uint8_t USART3_TxBuff[USART3_TX_BUFF_SIZE] = {0};

void Uart_Init(void)
{
  /*##-1- Configure the U(S)ART peripheral ######################################*/
  /* Put the USART peripheral in the Asynchronous mode (UART Mode) */
  
  /* UART configured as follows:
  - Word Length = 8 Bits (7 data bit + 1 parity bit) :
  BE CAREFUL : Program 7 data bits + 1 parity bit in PC HyperTerminal
  - Stop Bit    = One Stop bit
  - Parity      = ODD parity
  - BaudRate    = 115200 baud
  - Hardware flow control disabled (RTS and CTS signals) */
  USART3Handle.Instance = USART3;
  USART3Handle.Init.BaudRate = 230400;
  USART3Handle.Init.WordLength = UART_WORDLENGTH_8B;
  USART3Handle.Init.StopBits = UART_STOPBITS_1;
  USART3Handle.Init.Parity = UART_PARITY_ODD;
  USART3Handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  USART3Handle.Init.Mode = UART_MODE_TX_RX;
  USART3Handle.Init.OverSampling = UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&USART3Handle) != HAL_OK) { /* Initialization Error */ Error_Handler(); }

}
/**
* @brief UART MSP Initialization. Function is called from HAL_UART_Init and overrides the weak HAL implementation.
*        This function configures the hardware resources needed for the UART:
*           - Peripheral's clock enable
*           - Peripheral's GPIO Configuration
* @param huart: UART handle pointer
* @retval None
*/
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  GPIO_InitTypeDef  GPIO_InitStruct;
  static DMA_HandleTypeDef  hdma_usart;

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO TX/RX clock */
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /* Enable USARTx clock */
  __HAL_RCC_USART3_CLK_ENABLE();

  /* Enable DMA1 clock */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /*##-2- Configure peripheral GPIO ##########################################*/
  /* USART3 TX/RX GPIO pin configuration. TX on Pin 8 and RX on Pin 9. */
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);


  /*##-3- Configure the DMA streams ##########################################*/
  /* Set the parameters to be configured for USART3 TX: */
  hdma_usart.Instance = DMA1_Stream3;
  hdma_usart.Init.Channel = DMA_CHANNEL_4;          // USART3_TX
  hdma_usart.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_usart.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_usart.Init.MemInc = DMA_MINC_ENABLE;
  hdma_usart.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  //hdma_usart.Init.MemDataAlignment = DMA_MDATAALIGN_WORD; // In Direct mode source and destination data widths are equal and defined by PSIZE
  hdma_usart.Init.Mode = DMA_NORMAL; //DMA_CIRCULAR;  
  hdma_usart.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  hdma_usart.Init.FIFOMode = DMA_FIFOMODE_DISABLE;            /* i.e. Direct mode */
  hdma_usart.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL; // Dont think this matters since Direct Mode is used
  hdma_usart.Init.MemBurst = DMA_MBURST_SINGLE;               // Burst transfers are not possible in Direct mode
  hdma_usart.Init.PeriphBurst = DMA_PBURST_SINGLE;

  HAL_DMA_Init(&hdma_usart);

  /* Associate the initialized DMA handle to the Usart handle */
  __HAL_LINKDMA(huart, hdmatx, hdma_usart);
}
