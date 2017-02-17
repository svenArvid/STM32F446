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

  HAL_TIM_Base_Init(&Timer2Handle);

  /* Start Timer */
  HAL_TIM_Base_Start(&Timer2Handle);

  /* Enable GPIO Clock */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Configure IO in input mode with pull internal pulldown */
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;  // GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  GPIO_InitStruct.Pin = GPIO_PIN_8;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void InputCapture_4ms(void)
{
  int32_t StrLength;
  int32_t PinState;
  
  PinState = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8);
  StrLength = sprintf(USART3_TxBuff, "%ld,\n", PinState);

  // Print to Terminal
  HAL_UART_DMAStop(&USART3Handle);
  HAL_UART_Transmit_DMA(&USART3Handle, USART3_TxBuff, StrLength);
}