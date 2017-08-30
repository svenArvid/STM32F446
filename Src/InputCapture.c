/**
******************************************************************************
* @file    /Src/InputCapture.c
* @author  Joakim Carlsson
* @version V1.0
* @date    23-Jan-2017
* @brief   Use Timer 2 which is a 32-bit timer with 4 channels for Input Capture.
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "ErrorHandler.h"
#include "ProjectDefs.h"
#include "InputCapture.h"
#include "Uart.h"
#include "Util.h"

/* Timer handler declaration */
TIM_HandleTypeDef        Timer2Handle;


/**
* @brief  This function configures the TIM2 as a time base source used for Input Capture and time measurement.
*/
void InputCapture_Init(void)
{
  RCC_ClkInitTypeDef    clkconfig;
  uint32_t              uwTimclock;
  uint32_t              uwAPB1Prescaler = 0U;
  uint32_t              uwPrescalerValue = 0U;
  uint32_t              pFLatency;
  
  GPIO_InitTypeDef   GPIO_InitStruct;
  TIM_IC_InitTypeDef sConfig;


  /* Enable GPIO Clock */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Configure IO */
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;      // ;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;

  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Enable TIM2 clock */
  __HAL_RCC_TIM2_CLK_ENABLE();

  /* Get clock configuration */
  HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

  /* Get APB1 prescaler */
  uwAPB1Prescaler = clkconfig.APB1CLKDivider;

  /* Compute TIM2 clock */
  if (uwAPB1Prescaler == RCC_HCLK_DIV1)
  {
    uwTimclock = HAL_RCC_GetPCLK1Freq();
  }
  else
  {
    uwTimclock = 2 * HAL_RCC_GetPCLK1Freq();
  }

  /* Compute the prescaler value to have TIM2 counter clock equal to 10 MHz */
  uwPrescalerValue = (uint32_t)((uwTimclock / 10000000U) - 1U);

  /* Initialize TIM2 */
  Timer2Handle.Instance = TIM2;

  Timer2Handle.Init.Period = 0xFFFFFFFF;
  Timer2Handle.Init.Prescaler = uwPrescalerValue;
  Timer2Handle.Init.ClockDivision = 0;
  Timer2Handle.Init.CounterMode = TIM_COUNTERMODE_UP;

  HAL_TIM_IC_Init(&Timer2Handle);

  /* Configure the Input Capture channel */
  sConfig.ICPolarity = TIM_ICPOLARITY_RISING;     // Capture rising edges
  sConfig.ICSelection = TIM_ICSELECTION_DIRECTTI; 
  sConfig.ICFilter = 0xF;                        // f_sampling = f_DTS/32, N=8 i.e. 0.1*32*8 = 25.6 -> glitch filter is 25.6 us
  sConfig.ICPrescaler = TIM_ICPSC_DIV1;          // Capture performed each time an edge is detected on the capture input


  HAL_TIM_IC_ConfigChannel(&Timer2Handle, &sConfig, TIM_CHANNEL_1);
  HAL_TIM_IC_ConfigChannel(&Timer2Handle, &sConfig, TIM_CHANNEL_2);
  HAL_TIM_IC_ConfigChannel(&Timer2Handle, &sConfig, TIM_CHANNEL_3);
  HAL_TIM_IC_ConfigChannel(&Timer2Handle, &sConfig, TIM_CHANNEL_4);
  

  /* Start Timer and Input Capture on all 4 channels */
  HAL_TIM_IC_Start(&Timer2Handle, TIM_CHANNEL_1);
  HAL_TIM_IC_Start(&Timer2Handle, TIM_CHANNEL_2);
  HAL_TIM_IC_Start(&Timer2Handle, TIM_CHANNEL_3);
  HAL_TIM_IC_Start(&Timer2Handle, TIM_CHANNEL_4);
}


uint32_t InputCapture_ReadCCRx(TIM_TypeDef *TimInstance, uint32_t Channel)
{
  uint32_t RegisterVal = 0;

  switch (Channel)
  {
  case TIM_CHANNEL_1:
    RegisterVal = TimInstance->CCR1;
    break;
  case TIM_CHANNEL_2:
    RegisterVal = TimInstance->CCR2;
    break;
  case TIM_CHANNEL_3:
    RegisterVal = TimInstance->CCR3;
    break;
  case TIM_CHANNEL_4:
    RegisterVal = TimInstance->CCR4;
    break;
  default:
    break;
  }

  return RegisterVal;
}

void InputCapture_4ms(void)
{
  /*
  int32_t StrLength;
  int32_t PinStateA;
  int32_t PinStateB;
  uint32_t TimestampA = 0;
  uint32_t TimestampB = 0;

  PinStateA = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8);
  PinStateB = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9);

  if (Timer2Handle.Instance->SR & TIM_SR_CC1IF)
  {
    TimestampA = Timer2Handle.Instance->CCR1;
  }
  if (Timer2Handle.Instance->SR & TIM_SR_CC2IF)
  {
    TimestampB = Timer2Handle.Instance->CCR2;
  }

  StrLength = sprintf(USART3_TxBuff, "%ld,%ld,%lu,%lu,%lu,%lu\n", PinStateA, PinStateB, Timer2Handle.Instance->CNT, Timer2Handle.Instance->SR, TimestampA, TimestampB);

  // Print to Terminal
  HAL_UART_DMAStop(&USART3Handle);
  HAL_UART_Transmit_DMA(&USART3Handle, USART3_TxBuff, StrLength);
  */
}

