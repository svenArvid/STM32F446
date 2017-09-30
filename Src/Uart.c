/**
******************************************************************************
* @file    /Src/Uart.c
* @author  Joakim Carlsson
* @version V1.0
* @date    14-Jan-2017
* @brief   USART3 Tx is setup and configured with DMA
* To print message do the following:
* Use sprintf to format text and put it in USART3_TxBuff (respect the size of the buffer USART3_BUFF_SIZE)
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
UART_HandleTypeDef USART6Handle;

/* Buffers */
uint8_t USART3_TxBuff[USART3_BUFF_SIZE] = { 0 };
uint8_t USART3_RxBuff[USART3_BUFF_SIZE] = { 0 };

uint8_t USART6_TxBuff[USART6_BUFF_SIZE] = { 0 };
uint8_t USART6_RxBuff[USART6_BUFF_SIZE] = { 0 };

uint16_t USART3_TxBuffIndex = 0;

static void Uart_InitHW(void);

void Uart_Init(void)
{
  /*##-1- Configure the U(S)ART peripheral ######################################*/
  /* Put the USART peripheral in the Asynchronous mode (UART Mode) */
  
  /* UART configured as follows:
  - Word Length = 8 Bits (7 data bit + 1 parity bit) :
  BE CAREFUL : Program 7 data bits + 1 parity bit in PC HyperTerminal
  - Hardware flow control disabled (RTS and CTS signals) */
  USART3Handle.Instance = USART3;
  USART3Handle.Init.BaudRate = 115200;
  USART3Handle.Init.WordLength = UART_WORDLENGTH_8B;
  USART3Handle.Init.StopBits = UART_STOPBITS_1;
  USART3Handle.Init.Parity = UART_PARITY_NONE; //UART_PARITY_ODD;
  USART3Handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  USART3Handle.Init.Mode = UART_MODE_TX;
  USART3Handle.Init.OverSampling = UART_OVERSAMPLING_16;

  USART6Handle.Instance = USART6;
  USART6Handle.Init.BaudRate = 9600;
  USART6Handle.Init.WordLength = UART_WORDLENGTH_8B;
  USART6Handle.Init.StopBits = UART_STOPBITS_1;
  USART6Handle.Init.Parity = UART_PARITY_NONE;
  USART6Handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  USART6Handle.Init.Mode = UART_MODE_RX;
  USART6Handle.Init.OverSampling = UART_OVERSAMPLING_16;
   
  Uart_InitHW();
  
  if (HAL_UART_Init(&USART3Handle) != HAL_OK) { // Initialization Error 
    Error_Handler(); 
  }
  
  if (HAL_UART_Init(&USART6Handle) != HAL_OK) { // Initialization Error
    Error_Handler();
  }
  
  //(void)HAL_HalfDuplex_Init(&USART3Handle);


  HAL_UART_Receive_DMA(&USART6Handle, USART6_RxBuff, USART6_BUFF_SIZE);    // Start receiver
  //__HAL_DMA_ENABLE(USART6Handle.hdmarx);

  //HAL_UART_Receive(&USART3Handle, USART3_RxBuff, USART3_BUFF_SIZE,100);
}

/**
* @brief UART MSP Initialization. Function is called from HAL_UART_Init and overrides the weak HAL implementation.
*        This function configures the hardware resources needed for the UART:
*           - Peripheral's clock enable
*           - Peripheral's GPIO Configuration
* @param huart: UART handle pointer
* @retval None
*/
static void Uart_InitHW(void)
//void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  GPIO_InitTypeDef  GPIO_InitStruct;

  static DMA_HandleTypeDef  hdmatx_usart3;
  //static DMA_HandleTypeDef  hdmarx_usart3;
  //static DMA_HandleTypeDef  hdmatx_usart6;
  static DMA_HandleTypeDef  hdmarx_usart6;

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO TX/RX clock */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /* Enable USARTx clock */
  __HAL_RCC_USART3_CLK_ENABLE();
  __HAL_RCC_USART6_CLK_ENABLE();

  /* Enable DMA1 clock for USART3 and DMA2 clock for USART6 */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  
  //##-2- Configure peripheral GPIO ##########################################
  // USART3 TX/RX GPIO pin configuration. TX on Pin 8 and RX on Pin 9.
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  GPIO_InitStruct.Pin = GPIO_PIN_8 /* |  GPIO_PIN_9 */;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  // USART6 TX GPIO pin configuration. TX on Pin 14 and RX on Pin 9.
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
  GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_14;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  
  //##-3a- Configure the DMA stream for USART3 TX ##########################################
  hdmatx_usart3.Instance = DMA1_Stream3;
  hdmatx_usart3.Init.Channel = DMA_CHANNEL_4;          // USART3_TX is on stream 3, channel 4
  hdmatx_usart3.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdmatx_usart3.Init.PeriphInc = DMA_PINC_DISABLE;
  hdmatx_usart3.Init.MemInc = DMA_MINC_ENABLE;
  hdmatx_usart3.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  //hdmatx_usart3.Init.MemDataAlignment = DMA_MDATAALIGN_WORD; // In Direct mode source and destination data widths are equal and defined by PSIZE
  hdmatx_usart3.Init.Mode = DMA_NORMAL; //DMA_CIRCULAR;  
  hdmatx_usart3.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  hdmatx_usart3.Init.FIFOMode = DMA_FIFOMODE_DISABLE;            // i.e. Direct mode 
  hdmatx_usart3.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL; // Dont think this matters since Direct Mode is used
  hdmatx_usart3.Init.MemBurst = DMA_MBURST_SINGLE;               // Burst transfers are not possible in Direct mode
  hdmatx_usart3.Init.PeriphBurst = DMA_PBURST_SINGLE;

  HAL_DMA_Init(&hdmatx_usart3);

  // Associate the initialized DMA handle to the Usart handle 
  __HAL_LINKDMA(&USART3Handle, hdmatx, hdmatx_usart3);

  /*
  //##-3b- Configure the DMA stream for USART3 RX ##########################################
  hdmarx_usart3.Instance = DMA1_Stream1;
  hdmarx_usart3.Init.Channel = DMA_CHANNEL_4;          // USART3_RX is on stream 1, channel 4
  hdmarx_usart3.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdmarx_usart3.Init.PeriphInc = DMA_PINC_DISABLE;
  hdmarx_usart3.Init.MemInc = DMA_MINC_ENABLE;
  hdmarx_usart3.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  //hdmarx_usart3.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;    // In Direct mode source and destination data widths are equal and defined by PSIZE
  hdmarx_usart3.Init.Mode = DMA_NORMAL; //DMA_CIRCULAR;  
  hdmarx_usart3.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  hdmarx_usart3.Init.FIFOMode = DMA_FIFOMODE_DISABLE;             // i.e. Direct mode 
  hdmarx_usart3.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL; // Dont think this matters since Direct Mode is used
  hdmarx_usart3.Init.MemBurst = DMA_MBURST_SINGLE;                // Burst transfers are not possible in Direct mode
  hdmarx_usart3.Init.PeriphBurst = DMA_PBURST_SINGLE;
  
  HAL_DMA_Init(&hdmarx_usart3);

  // Associate the initialized DMA handle to the Usart handle 
  __HAL_LINKDMA(&USART3Handle, hdmarx, hdmarx_usart3);
  */
  /*
  //##-3c- Configure the DMA stream for USART6 TX ##########################################
  hdmatx_usart6.Instance = DMA2_Stream7;
  hdmatx_usart6.Init.Channel = DMA_CHANNEL_5;
  hdmatx_usart6.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdmatx_usart6.Init.PeriphInc = DMA_PINC_DISABLE;
  hdmatx_usart6.Init.MemInc = DMA_MINC_ENABLE;
  hdmatx_usart6.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  //hdmatx_usart6.Init.MemDataAlignment = DMA_MDATAALIGN_WORD; // In Direct mode source and destination data widths are equal and defined by PSIZE
  hdmatx_usart6.Init.Mode = DMA_NORMAL;  
  hdmatx_usart6.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  hdmatx_usart6.Init.FIFOMode = DMA_FIFOMODE_DISABLE;            // i.e. Direct mode 
  hdmatx_usart6.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL; // Dont think this matters since Direct Mode is used
  hdmatx_usart6.Init.MemBurst = DMA_MBURST_SINGLE;               // Burst transfers are not possible in Direct mode
  hdmatx_usart6.Init.PeriphBurst = DMA_PBURST_SINGLE;

  HAL_DMA_Init(&hdmatx_usart6);

  // Associate the initialized DMA handle to the Usart handle 
  __HAL_LINKDMA(&USART6Handle, hdmatx, hdmatx_usart6);
  */
  //##-3d- Configure the DMA stream for USART6 RX ##########################################
  hdmarx_usart6.Instance = DMA2_Stream1;
  hdmarx_usart6.Init.Channel = DMA_CHANNEL_5;
  hdmarx_usart6.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdmarx_usart6.Init.PeriphInc = DMA_PINC_DISABLE;
  hdmarx_usart6.Init.MemInc = DMA_MINC_ENABLE;
  hdmarx_usart6.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  //hdmarx_usart6.Init.MemDataAlignment = DMA_MDATAALIGN_WORD; // In Direct mode source and destination data widths are equal and defined by PSIZE
  hdmarx_usart6.Init.Mode = DMA_NORMAL;
  hdmarx_usart6.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  hdmarx_usart6.Init.FIFOMode = DMA_FIFOMODE_DISABLE;            // i.e. Direct mode 
  hdmarx_usart6.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL; // Dont think this matters since Direct Mode is used
  hdmarx_usart6.Init.MemBurst = DMA_MBURST_SINGLE;               // Burst transfers are not possible in Direct mode
  hdmarx_usart6.Init.PeriphBurst = DMA_PBURST_SINGLE;

  HAL_DMA_Init(&hdmarx_usart6);

  // Associate the initialized DMA handle to the Usart handle 
  __HAL_LINKDMA(&USART6Handle, hdmarx, hdmarx_usart6);
}

static void Uart_PollReceiver(UART_HandleTypeDef *huart)
{
  static index = 0;
  /*
  if (__HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE))
  {
    USART6_RxBuff[index++] = huart->Instance->DR;
  }
  else if (index > 0)
  {
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);

    // Print to Terminal
    for (int i = 0; i < index; i++)
    {
      LogStrLength += sprintf(USART3_TxBuff + LogStrLength, "%d", USART6_RxBuff[i]);
    }
    LogStrLength += sprintf(USART3_TxBuff + LogStrLength, "\n%d\n", huart->hdmarx->Instance->NDTR);
    LogStrLength += sprintf(USART3_TxBuff + LogStrLength, "%X\n", huart->hdmarx->Instance->CR);
    LogStrLength += sprintf(USART3_TxBuff + LogStrLength, "%X\n", huart->hdmarx->Instance->PAR);
    LogStrLength += sprintf(USART3_TxBuff + LogStrLength, "%X\n", huart->hdmarx->Instance->FCR);
    if (LogStrLength > 0)
    {
      if (__HAL_UART_GET_FLAG(&USART3Handle, UART_FLAG_TC))    // Check if Transmission is complete
      {
        USART3Handle.gState = HAL_UART_STATE_READY;
        USART3Handle.hdmatx->State = HAL_DMA_STATE_READY;
        __HAL_UNLOCK(USART3Handle.hdmatx);
        HAL_UART_Transmit_DMA(&USART3Handle, USART3_TxBuff, LogStrLength);
        LogStrLength = 0;
      }
    }
    index = 0;
  }
  */

  if(__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) 
  {
    uint32_t Temp = USART6->DR;    // To clear IDLE LINE FLAG
    int BytesReceived = huart->RxXferSize - huart->hdmarx->Instance->NDTR;

    __HAL_DMA_DISABLE(huart->hdmarx);                       // Must disable stream before writing to it's registers

    huart->hdmarx->Instance->NDTR = huart->RxXferSize;      // Reset to full size
    //DMA2->LIFCR = 0x3DU << 6;                               // Clear DMA Interrupt flags for Stream 1
    DMA2->LIFCR = DMA_FLAG_TCIF1_5 | DMA_FLAG_HTIF1_5 | DMA_FLAG_TEIF1_5 | DMA_FLAG_DMEIF1_5 | DMA_FLAG_FEIF1_5;

    __HAL_DMA_ENABLE(huart->hdmarx);
    // Check message
    Uart_PrintToTerminal();
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);

    // Print to Terminal
    for (int i = 0; i < BytesReceived; i++)
    {
      UART_PRINTF("%X", USART6_RxBuff[i]);
    }
    Uart_TransmitTerminalBuffer();
  }
}

void Uart_1ms(void)
{
  // Check Receiver
  Uart_PollReceiver(&USART6Handle);
}

void Uart_PrintToTerminal(void)
{  
  UART_PRINTF("\r\nUSART6:\r\n");
  UART_PRINTF("SR:  %X\r\n", USART6->SR);
  UART_PRINTF("DR:  %X\r\n", USART6->DR);
  UART_PRINTF("BRR: %X\r\n", USART6->BRR);
  UART_PRINTF("CR1: %X\r\n", USART6->CR1);
  UART_PRINTF("CR2: %X\r\n", USART6->CR2);
  UART_PRINTF("CR3: %X\r\n", USART6->CR3);
  
  UART_PRINTF("\r\nDMA2 Stream 1 Channel 5 (USART6 Rx):\r\n");
  UART_PRINTF("NDTR: %d\r\n", USART6Handle.hdmarx->Instance->NDTR);
  UART_PRINTF("CR:  %X\r\n", USART6Handle.hdmarx->Instance->CR);
  UART_PRINTF("PAR: %X\r\n", USART6Handle.hdmarx->Instance->PAR);
  UART_PRINTF("FCR: %X\r\n", USART6Handle.hdmarx->Instance->FCR);

  UART_PRINTF("\nDMA2 Common:\r\n");
  UART_PRINTF("LISR:  %X\r\n", DMA2->LISR);
  UART_PRINTF("LIFCR: %X\r\n", DMA2->LIFCR);
}

// Call this function to print out the data stored in the Terminal Buffer
void Uart_TransmitTerminalBuffer(void)
{
  if (__HAL_UART_GET_FLAG(&USART3Handle, UART_FLAG_TC))    // Check if Transmission is complete
  {
    USART3Handle.gState = HAL_UART_STATE_READY;
    USART3Handle.hdmatx->State = HAL_DMA_STATE_READY;
    __HAL_UNLOCK(USART3Handle.hdmatx);
    HAL_UART_Transmit_DMA(&USART3Handle, USART3_TxBuff, USART3_TxBuffIndex);
    USART3_TxBuffIndex = 0;
  }
}