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

/* Buffers */
#define USART6_BUFF_SIZE  512

uint8_t USART3_TxBuff[USART3_BUFF_SIZE] = { 0 };
uint8_t USART3_RxBuff[USART3_BUFF_SIZE] = { 0 };

uint8_t USART6_TxBuff[USART6_BUFF_SIZE] = { 0 };
uint8_t USART6_RxBuff[USART6_BUFF_SIZE] = { 0 };

UartPort ModbusPort;
UartPort TerminalPort;

static void Uart_InitHW(void);

static void DMA_ClearAllFlags(DMA_Stream_TypeDef *DmaStream)
{
  volatile uint32_t *Register;

  if (DmaStream >= DMA2_Stream4)
  {
    Register = &(DMA2->HIFCR);
  }
  else if (DmaStream >= DMA2_Stream0)
  {
    Register = &(DMA2->LIFCR);
  }
  else if (DmaStream >= DMA1_Stream4)
  {
    Register = &(DMA1->HIFCR);
  }
  else {
    Register = &(DMA1->LIFCR);
  }

  switch ((uint32_t)DmaStream)
  {
  case (uint32_t)DMA1_Stream0:
  case (uint32_t)DMA2_Stream0:
  case (uint32_t)DMA1_Stream4:
  case (uint32_t)DMA2_Stream4:
    *Register = (DMA_FLAG_TCIF0_4 | DMA_FLAG_HTIF0_4 | DMA_FLAG_TEIF0_4 | DMA_FLAG_DMEIF0_4 | DMA_FLAG_FEIF0_4);
    break;
  case (uint32_t)DMA1_Stream1:
  case (uint32_t)DMA2_Stream1:
  case (uint32_t)DMA1_Stream5:
  case (uint32_t)DMA2_Stream5:
    *Register = (DMA_FLAG_TCIF1_5 | DMA_FLAG_HTIF1_5 | DMA_FLAG_TEIF1_5 | DMA_FLAG_DMEIF1_5 | DMA_FLAG_FEIF1_5);
    break;
  case (uint32_t)DMA1_Stream2:
  case (uint32_t)DMA2_Stream2:
  case (uint32_t)DMA1_Stream6:
  case (uint32_t)DMA2_Stream6:
    *Register = (DMA_FLAG_TCIF2_6 | DMA_FLAG_HTIF2_6 | DMA_FLAG_TEIF2_6 | DMA_FLAG_DMEIF2_6 | DMA_FLAG_FEIF2_6);
    break;
  case (uint32_t)DMA1_Stream3:
  case (uint32_t)DMA2_Stream3:
  case (uint32_t)DMA1_Stream7:
  case (uint32_t)DMA2_Stream7:
    *Register = (DMA_FLAG_TCIF3_7 | DMA_FLAG_HTIF3_7 | DMA_FLAG_TEIF3_7 | DMA_FLAG_DMEIF3_7 | DMA_FLAG_FEIF3_7);
    break;
  }
}

void Uart_Init(void)
{
  /*##-1- Configure the U(S)ART peripheral ######################################*/
  /* Put the USART peripheral in the Asynchronous mode (UART Mode) */
  /*
  USART3Handle.Instance = USART3;
  USART3Handle.Init.BaudRate = 115200;
  USART3Handle.Init.WordLength = UART_WORDLENGTH_8B;
  USART3Handle.Init.StopBits = UART_STOPBITS_1;
  USART3Handle.Init.Parity = UART_PARITY_NONE; //UART_PARITY_ODD;
  USART3Handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  USART3Handle.Init.Mode = UART_MODE_TX_RX;
  USART3Handle.Init.OverSampling = UART_OVERSAMPLING_16;
  */
  ModbusPort.Rx.Buffer = USART6_RxBuff;
  ModbusPort.Rx.Size = USART6_BUFF_SIZE;
  ModbusPort.Tx.Buffer = USART6_TxBuff;
  ModbusPort.Tx.Size = USART6_BUFF_SIZE;

  TerminalPort.Rx.Buffer = USART3_RxBuff;
  TerminalPort.Rx.Size = USART3_BUFF_SIZE;
  TerminalPort.Tx.Buffer = USART3_TxBuff;
  TerminalPort.Tx.Size = USART3_BUFF_SIZE;

  Uart_InitHW();
  
  //(void)HAL_HalfDuplex_Init(&USART3Handle);

}

/**
*        This function configures the hardware resources needed for the UART:
*           - Peripheral's clock enable
*           - Peripheral's GPIO Configuration
*           - USART configuration
*           - DMA configuration for Tx and Rx
*/
static void Uart_InitHW(void)
{
  GPIO_InitTypeDef  GPIO_InitStruct;

  static DMA_HandleTypeDef  hdmatx_usart3, hdmarx_usart3;

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
  GPIO_InitStruct.Pin = GPIO_PIN_8 |  GPIO_PIN_9;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  // USART6 TX GPIO pin configuration. TX on Pin 14 and RX on Pin 9.
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
  GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_14;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  //##-3- Configure ModbusPort: USART 6, Rx using DMA2 Stream 1 (channel 5) and Tx using DMA2 Stream 7 (channel 5) ##
  uint32_t BaudRate = 9600;

  ModbusPort.Usart = USART6;
  ModbusPort.DMAStream_Rx = DMA2_Stream1;
  ModbusPort.DMAStream_Tx = DMA2_Stream7;

  ModbusPort.Usart->BRR = UART_BRR_SAMPLING16(HAL_RCC_GetPCLK2Freq(), BaudRate);  // USART6 uses APB2 bus
  ModbusPort.Usart->CR2 = 0x0;
  ModbusPort.Usart->CR3 = USART_CR3_DMAR;
  ModbusPort.Usart->CR1 = USART_CR1_UE | USART_CR1_RE; /* | USART_CR1_TE */
  // Reg. GTPR  Keep at reset val

  // ------ Rx Stream ------
  ModbusPort.DMAStream_Rx->CR &= ~DMA_SxCR_EN;      // Make sure stream is disabled before writing to it's registers
  DMA_ClearAllFlags(ModbusPort.DMAStream_Rx);
  ModbusPort.DMAStream_Rx->PAR = (uint32_t)&(ModbusPort.Usart->DR);
  ModbusPort.DMAStream_Rx->M0AR = (uint32_t)ModbusPort.Rx.Buffer;
  ModbusPort.DMAStream_Rx->NDTR = ModbusPort.Rx.Size;
  // Reg. M1AR   Not used
  // Reg. FCR    Keep at reset val
  ModbusPort.DMAStream_Rx->CR = DMA_CHANNEL_5 | DMA_PRIORITY_VERY_HIGH | DMA_MINC_ENABLE | DMA_PERIPH_TO_MEMORY /*| DMA_IT_TC | DMA_IT_TE | DMA_IT_DME*/;

  ModbusPort.DMAStream_Rx->CR |= DMA_SxCR_EN;    // Enable Rx stream    

  // ------ Tx Stream ------
  ModbusPort.DMAStream_Tx->CR &= ~DMA_SxCR_EN;      // Make sure stream is disabled before writing to it's registers
  DMA_ClearAllFlags(ModbusPort.DMAStream_Tx);

  ModbusPort.DMAStream_Tx->PAR = (uint32_t)&(ModbusPort.Usart->DR);
  ModbusPort.DMAStream_Tx->M0AR = (uint32_t)ModbusPort.Tx.Buffer;
  ModbusPort.DMAStream_Tx->NDTR = 0;
  // Reg. M1AR   Not used
  // Reg. FCR    Keep at reset val
  ModbusPort.DMAStream_Tx->CR = DMA_CHANNEL_5 | DMA_PRIORITY_VERY_HIGH | DMA_MINC_ENABLE | DMA_MEMORY_TO_PERIPH;


  //##-4- Configure TerminalPort: USART 3, Rx using DMA1 Stream 1 (channel 4) and Tx using DMA1 Stream 3 (channel 4) ##
  BaudRate = 115200;

  TerminalPort.Usart = USART3;
  TerminalPort.DMAStream_Rx = DMA1_Stream1;
  TerminalPort.DMAStream_Tx = DMA1_Stream3;

  TerminalPort.Usart->BRR = UART_BRR_SAMPLING16(HAL_RCC_GetPCLK1Freq(), BaudRate);  // USART3 uses APB2 bus ???
  TerminalPort.Usart->CR2 = 0x0;
  TerminalPort.Usart->CR3 = USART_CR3_DMAR;
  TerminalPort.Usart->CR1 = USART_CR1_UE | USART_CR1_RE; /* | USART_CR1_TE */
  // Reg. GTPR  Keep at reset val

  // ------ Rx Stream ------
  TerminalPort.DMAStream_Rx->CR &= ~DMA_SxCR_EN;      // Make sure stream is disabled before writing to it's registers
  DMA_ClearAllFlags(TerminalPort.DMAStream_Rx);

  TerminalPort.DMAStream_Rx->PAR = (uint32_t)&(TerminalPort.Usart->DR);
  TerminalPort.DMAStream_Rx->M0AR = (uint32_t)TerminalPort.Rx.Buffer;
  TerminalPort.DMAStream_Rx->NDTR = TerminalPort.Rx.Size;
  // Reg. M1AR   Not used
  // Reg. FCR    Keep at reset val
  TerminalPort.DMAStream_Rx->CR = DMA_CHANNEL_4 | DMA_PRIORITY_VERY_HIGH | DMA_MINC_ENABLE | DMA_PERIPH_TO_MEMORY /*| DMA_IT_TC | DMA_IT_TE | DMA_IT_DME*/;
  TerminalPort.DMAStream_Rx->CR |= DMA_SxCR_EN;    // Enable Rx stream

  // ------ Tx Stream ------
  TerminalPort.DMAStream_Tx->CR &= ~DMA_SxCR_EN;      // Make sure stream is disabled before writing to it's registers
  DMA_ClearAllFlags(TerminalPort.DMAStream_Tx);

  TerminalPort.DMAStream_Tx->PAR = (uint32_t)&(TerminalPort.Usart->DR);
  TerminalPort.DMAStream_Tx->M0AR = (uint32_t)TerminalPort.Tx.Buffer;
  TerminalPort.DMAStream_Tx->NDTR = 0;
  // Reg. M1AR   Not used
  // Reg. FCR    Keep at reset val
  TerminalPort.DMAStream_Tx->CR = DMA_CHANNEL_4 | DMA_PRIORITY_VERY_HIGH | DMA_MINC_ENABLE | DMA_MEMORY_TO_PERIPH;
  
  /*
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
  */
}

// Checks if a message has been received (by testing IDLE LINE Flag) and if so returns the number of bytes received
uint16_t Uart_MessageReceived(UartPort *Port)
{
  uint16_t BytesReceived = 0;

  if (Port->Usart->SR & USART_SR_IDLE)
  {
    uint32_t TempReg = Port->Usart->DR;                    // To clear IDLE LINE FLAG
    BytesReceived = Port->Rx.Size - Port->DMAStream_Rx->NDTR;
  }

  return BytesReceived;
}

// Disables DMA Rx stream and Receiver
void Uart_StopReceiver(UartPort *Port)
{
  Port->DMAStream_Rx->CR &= ~DMA_SxCR_EN;   
  DMA_ClearAllFlags(Port->DMAStream_Rx);

  Port->Usart->CR3 &= ~USART_CR3_DMAR;
  Port->Usart->CR1 &= ~USART_CR1_RE;
}

// (Re)starts the Receiver and DMA stream
void Uart_StartReceiver(UartPort *Port)
{
  Port->DMAStream_Rx->CR &= ~DMA_SxCR_EN;        // Must disable stream before writing to it's registers
  DMA_ClearAllFlags(Port->DMAStream_Rx);
  Port->DMAStream_Rx->NDTR = Port->Rx.Size;      // Reload to full size
  
  Port->Usart->CR1 |= USART_CR1_RE;
  Port->Usart->CR3 |= USART_CR3_DMAR;
  
  Port->DMAStream_Rx->CR |= DMA_SxCR_EN;        // Enable stream
}

// Checks if transmission of a message has finished (by testing the TC flag)
bool Uart_TransmissionComplete(UartPort *Port)
{
  if (Port->Usart->SR & USART_SR_TC)    // Check if Transmission is complete
  {
    return TRUE;
  }
  else 
  {
    return FALSE;
  }
}

// Disables DMA Tx stream and Transmitter
void Uart_StopTransmitter(UartPort *Port)
{
  Port->DMAStream_Tx->CR &= ~DMA_SxCR_EN;
  DMA_ClearAllFlags(Port->DMAStream_Tx);

  Port->Usart->CR3 &= ~USART_CR3_DMAT;
  Port->Usart->CR1 &= ~USART_CR1_TE;
}

// (Re)starts the Transmitter and DMA stream if BytesToSend > 0
void Uart_StartTransmitter(UartPort *Port, uint16_t BytesToSend)
{
  if (BytesToSend > 0)                           // Is there anything to transmit?
  {
    Port->DMAStream_Tx->CR &= ~DMA_SxCR_EN;      // Must disable stream before writing to it's registers
    DMA_ClearAllFlags(Port->DMAStream_Tx);
    Port->DMAStream_Tx->NDTR = BytesToSend;

    Port->Usart->CR1 |= USART_CR1_TE;
    Port->Usart->CR3 |= USART_CR3_DMAT;

    Port->DMAStream_Tx->CR |= DMA_SxCR_EN;        // Enable stream
  }
}

static void Uart_TestUart(void)
{
  uint16_t ByteCount =  Uart_MessageReceived(&ModbusPort);
  static bool TxMode = FALSE;

  if(ByteCount > 0)
  {
    Uart_StopReceiver(&ModbusPort);

    // Check message
    Uart_PrintRegisters();
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);

    // Print to Terminal
    for (int i = 0; i < ByteCount; i++)
    {
      ModbusPort.Tx.Buffer[i] = ModbusPort.Rx.Buffer[i];
      UART_PRINTF("%X", USART6_RxBuff[i]);
    }
    Uart_TransmitTerminalBuffer();

    TxMode = TRUE;
    Uart_StartTransmitter(&ModbusPort, ByteCount);
  }
  else if (TxMode)
  {
    if (Uart_TransmissionComplete(&ModbusPort))
    {
      TxMode = FALSE;
      Uart_StopTransmitter(&ModbusPort);
      Uart_StartReceiver(&ModbusPort);
    }
  }
}

void Uart_20ms(void)
{
  // Check Receiver
  //Uart_TestUart();
}

void Uart_PrintRegisters(void)
{  
  UART_PRINTF("\r\nUSART6:\r\n");
  UART_PRINTF("SR:  %X\r\n", USART6->SR);
  UART_PRINTF("DR:  %X\r\n", USART6->DR);
  UART_PRINTF("BRR: %X\r\n", USART6->BRR);
  UART_PRINTF("CR1: %X\r\n", USART6->CR1);
  UART_PRINTF("CR2: %X\r\n", USART6->CR2);
  UART_PRINTF("CR3: %X\r\n", USART6->CR3);
  
  UART_PRINTF("\r\nDMA2 Stream 1 Channel 5 (USART6 Rx):\r\n");
  UART_PRINTF("NDTR: %d\r\n", DMA2_Stream1->NDTR);
  UART_PRINTF("CR:   %X\r\n", DMA2_Stream1->CR);
  UART_PRINTF("PAR:  %X\r\n", DMA2_Stream1->PAR);
  UART_PRINTF("M0AR: %X\r\n", DMA2_Stream1->M0AR);
  UART_PRINTF("FCR:  %X\r\n", DMA2_Stream1->FCR);

  UART_PRINTF("\nDMA2 Common:\r\n");
  UART_PRINTF("LISR:  %X\r\n", DMA2->LISR);
  UART_PRINTF("LIFCR: %X\r\n", DMA2->LIFCR);
}

// Call this function to print out the data stored in the Terminal Buffer
void Uart_TransmitTerminalBuffer(void)
{
  if (Uart_TransmissionComplete(&TerminalPort))    // Check if (previous) Transmission is complete
  {
    Uart_StartTransmitter(&TerminalPort, TerminalPort.Tx.Indx);
    TerminalPort.Tx.Indx = 0;
  }
}

bool Uart_TerminalBufferEmpty(void)
{
  return TerminalPort.Tx.Indx == 0;
}