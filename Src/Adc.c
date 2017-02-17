/**
******************************************************************************
* @file    /Src/Adc.c
* @author  Joakim Carlsson
* @version V1.0
* @date    08-Jan-2017
* @brief   ADC is setup and configured with DMA
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "ErrorHandler.h"
#include "ProjectDefs.h"
#include "Adc.h"
#include "Uart.h"


/* ADC handler declaration */
ADC_HandleTypeDef    AdcHandle;

/* Variable used to get converted value */
__IO uint16_t ADC1ConvValArray[ADC_ARR_SIZE] = { 0 };

void Adc_Init(void)
{
  ADC_ChannelConfTypeDef sConfig;

  /*##-1- Configure the ADC peripheral #######################################*/
  AdcHandle.Instance = ADC1;
  AdcHandle.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  AdcHandle.Init.Resolution = ADC_RESOLUTION_12B;
  AdcHandle.Init.ScanConvMode = ENABLE; //DISABLE;             /* Sequencer enabled (ADC conversion on multiple channel:s start with rank 1) */
  AdcHandle.Init.ContinuousConvMode = ENABLE;                  /* Continuous mode, i.e. continue forever */
  AdcHandle.Init.DiscontinuousConvMode = DISABLE;              /* Parameter discarded if sequencer is disabled */
  AdcHandle.Init.NbrOfDiscConversion = 0;
  AdcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;        /* Conversion start trigged at each external event */
  AdcHandle.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
  AdcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  AdcHandle.Init.NbrOfConversion = 2; //1;
  AdcHandle.Init.DMAContinuousRequests = ENABLE;
  AdcHandle.Init.EOCSelection = DISABLE;

  if (HAL_ADC_Init(&AdcHandle) != HAL_OK) { /* ADC initialization Error */ Error_Handler(); }

  /*##-2- Configure ADC regular channel ######################################*/
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  sConfig.Offset = 0;

  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = 1;

  if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK) { /* Channel Configuration Error */ Error_Handler(); }

  sConfig.Channel = ADC_CHANNEL_13;
  sConfig.Rank = 2;

  if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK) { /* Channel Configuration Error */ Error_Handler(); }

  /*##-3- Start the conversion process #######################################*/
  /* Note: Considering IT occurring after each number of ADC conversions      */
  /*       (IT by DMA end of transfer), select sampling time and ADC clock    */
  /*       with sufficient duration to not create an overhead situation in    */
  /*        IRQHandler. */
  if (HAL_ADC_Start_DMA(&AdcHandle, (uint32_t*)ADC1ConvValArray, ADC_ARR_SIZE) != HAL_OK) { /* Start Conversation Error */ Error_Handler(); }
}

/**
* @brief ADC MSP Initialization. Function is called from HAL_ADC_Init and overrides the weak HAL implementation.
*        This function configures the hardware resources needed for the ADC:
*           - Peripheral's clock enable
*           - Peripheral's GPIO Configuration
* @param hadc: ADC handle pointer
* @retval None
*/
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
  GPIO_InitTypeDef          GPIO_InitStruct;
  static DMA_HandleTypeDef  hdma_adc;

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* ADC1 Periph clock enable */
  __HAL_RCC_ADC1_CLK_ENABLE();
  /* Enable GPIO clock ****************************************/
  __HAL_RCC_GPIOC_CLK_ENABLE();
  /* Enable DMA2 clock */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /*##-2- Configure peripheral GPIO ##########################################*/
  /* ADC Channel GPIO pin configuration */
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*##-3- Configure the DMA streams ##########################################*/
  /* Set the parameters to be configured */
  hdma_adc.Instance = DMA2_Stream0;
  hdma_adc.Init.Channel = DMA_CHANNEL_0;
  hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_adc.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_adc.Init.MemInc = DMA_MINC_ENABLE;
  hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_adc.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;     // In Direct mode source and destination data widths are equal and defined by PSIZE 
  hdma_adc.Init.Mode = DMA_CIRCULAR;                        // DMA is flow controller. In ADC continuous mode, DMA must be configured in circular mode 
  hdma_adc.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  hdma_adc.Init.FIFOMode = DMA_FIFOMODE_DISABLE;            /* i.e. Direct mode */
  hdma_adc.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
  hdma_adc.Init.MemBurst = DMA_MBURST_SINGLE;               // Burst transfers are not possible in Direct mode
  hdma_adc.Init.PeriphBurst = DMA_PBURST_SINGLE;

  HAL_DMA_Init(&hdma_adc);

  /* Associate the initialized DMA handle to the ADC handle */
  __HAL_LINKDMA(hadc, DMA_Handle, hdma_adc);

  /*##-4- Configure the NVIC for DMA #########################################*/
  /* NVIC configuration for DMA transfer complete interrupt */
  //HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  //HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

uint16_t Adc_Read(uint32_t index)
{
  return ADC1ConvValArray[index] * 4 + 2;  // convert to 14 bits
}


// Use this function to print out data for testing/debugging
void Adc_500ms(void)
{
  /*
  int32_t StrLength;
  StrLength = sprintf(USART3_TxBuff, "\n\r ADC Array:\t");

  for (int i = 0; i < ADC_ARR_SIZE; i++)
  {
    StrLength += sprintf(USART3_TxBuff + StrLength, "%d\t", ADC1ConvValArray[i]);
  }
  StrLength += sprintf(USART3_TxBuff + StrLength, "\n\r");

  HAL_UART_DMAStop(&USART3Handle);
  HAL_UART_Transmit_DMA(&USART3Handle, USART3_TxBuff, StrLength);
  */
}