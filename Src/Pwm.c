/**
******************************************************************************
* @file    /Src/Pwm.c
* @author  Joakim Carlsson
* @version V1.0
* @date    31-Dec-2016
* @brief   PWM is setup. Both Timer 3 and Timer 4 are used. 
*          Thus 4+4 Pwm channels are possible. Timer 3 and 4 are configured to have different Pwm frequency
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "ErrorHandler.h"
#include "ProjectDefs.h"
#include "Pwm.h"


/* Timer handler declaration */
static TIM_HandleTypeDef    Timer4Handle;
static TIM_HandleTypeDef    Timer3Handle;


void Pwm_Init(void)
{
  /*##-1- Configure the TIM peripheral #######################################*/
  /* -----------------------------------------------------------------------
  TIM3 and TIM4 input clocks are set to APB1 clock x 2,
  since APB1 prescaler is equal to 2.
  TIM3CLK = APB1CLK*2
  APB1CLK = HCLK/4
  => TIM3CLK = HCLK / 2 = SystemCoreClock / 2    (90 MHz)

  The prescaler is computed as follows:
  Prescaler = (TIM3CLK / TIM3 counter clock) - 1

  Note:
  SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f4xx.c file.
  Each time the core clock (HCLK) changes, user had to update SystemCoreClock
  variable value. Otherwise, any configuration based on this variable will be incorrect.
  This variable is updated in three ways:
  1) by calling CMSIS function SystemCoreClockUpdate()
  2) by calling HAL API function HAL_RCC_GetSysClockFreq()
  3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
  ----------------------------------------------------------------------- */

  TIM_OC_InitTypeDef sConfig;        /* Timer Output Compare Configuration Structure declaration */

  uint32_t uhPrescalerValue = 0;     /* Counter Prescaler value */

  // Compute the prescaler value to have TIM4 counter clock equal to 400 000 Hz to get 400 Hz Pwm signals (400000 / 1000) 
  uhPrescalerValue = (uint32_t)((SystemCoreClock / 2) / 400000) - 1;

  Timer4Handle.Instance = TIM4;

  Timer4Handle.Init.Prescaler = uhPrescalerValue;
  Timer4Handle.Init.Period = PWM_MAX_DUTY - 1;    // Note: Should be -1 to get 100% duty when Pulse == PWM_MAX_DUTY
  Timer4Handle.Init.ClockDivision = 0;
  Timer4Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
  Timer4Handle.Init.RepetitionCounter = 0;

  // Compute the prescaler value to have TIM3 counter clock equal to 800 000 Hz to get 800 Hz Pwm signals (roundoff makes it ~803 Hz)
  uhPrescalerValue = (uint32_t)((SystemCoreClock / 2) / 800000) - 1;
  
  Timer3Handle = Timer4Handle;                    // Timer 3 configuration is same as Timer 4, except it uses 800 Hz frequency
  Timer3Handle.Instance = TIM3;
  Timer3Handle.Init.Prescaler = uhPrescalerValue;

  if (HAL_TIM_PWM_Init(&Timer4Handle) != HAL_OK) { /* Initialization Error */ Error_Handler(); }
  if (HAL_TIM_PWM_Init(&Timer3Handle) != HAL_OK) { /* Initialization Error */ Error_Handler(); }


  /*##-2- Configure the PWM channels #########################################*/
  /* Common configuration for all channels (both Timer 3 and Timer 4)*/
  sConfig.OCMode       = TIM_OCMODE_PWM1;
  sConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;
  sConfig.OCFastMode   = TIM_OCFAST_DISABLE;
  sConfig.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
  sConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  sConfig.OCIdleState  = TIM_OCIDLESTATE_RESET;
  
  /* Set the pulse values for TIMER 4 channels, one by one. Then Start PWM signals generation, one by one */
  sConfig.Pulse = PULSE1_VALUE;
  if (HAL_TIM_PWM_ConfigChannel(&Timer4Handle, &sConfig, TIM_CHANNEL_1) != HAL_OK) { /* Configuration Error */ Error_Handler(); }
  
  sConfig.Pulse = PWM_MAX_DUTY;
  if (HAL_TIM_PWM_ConfigChannel(&Timer4Handle, &sConfig, TIM_CHANNEL_3) != HAL_OK) { /* Configuration Error */ Error_Handler(); }

  sConfig.Pulse = PWM_MAX_DUTY/10;
  if (HAL_TIM_PWM_ConfigChannel(&Timer4Handle, &sConfig, TIM_CHANNEL_4) != HAL_OK) { /* Configuration Error */ Error_Handler(); }

  /* Start channels, one by one, TIMER 4 */
  if (HAL_TIM_PWM_Start(&Timer4Handle, TIM_CHANNEL_1) != HAL_OK) { /* PWM Generation Error */ Error_Handler(); }

  if (HAL_TIM_PWM_Start(&Timer4Handle, TIM_CHANNEL_3) != HAL_OK) { /* PWM Generation Error */ Error_Handler(); }

  if (HAL_TIM_PWM_Start(&Timer4Handle, TIM_CHANNEL_4) != HAL_OK) { /* PWM generation Error */ Error_Handler(); }
  

  /* Set the pulse values for TIMER 3 channels, one by one. Then Start PWM signals generation, one by one */
  sConfig.Pulse = PWM_MAX_DUTY/4;
  if (HAL_TIM_PWM_ConfigChannel(&Timer3Handle, &sConfig, TIM_CHANNEL_1) != HAL_OK) { /* Configuration Error */ Error_Handler(); }

  sConfig.Pulse = PWM_MAX_DUTY;
  if (HAL_TIM_PWM_ConfigChannel(&Timer3Handle, &sConfig, TIM_CHANNEL_2) != HAL_OK) { /* Configuration Error */ Error_Handler(); }

  sConfig.Pulse = 0;
  if (HAL_TIM_PWM_ConfigChannel(&Timer3Handle, &sConfig, TIM_CHANNEL_4) != HAL_OK) { /* Configuration Error */ Error_Handler(); }
  
  /* Start channels, one by one, TIMER 3 */
  if (HAL_TIM_PWM_Start(&Timer3Handle, TIM_CHANNEL_1) != HAL_OK) { /* PWM Generation Error */ Error_Handler(); }

  if (HAL_TIM_PWM_Start(&Timer3Handle, TIM_CHANNEL_2) != HAL_OK) { /* PWM Generation Error */ Error_Handler(); }

  if (HAL_TIM_PWM_Start(&Timer3Handle, TIM_CHANNEL_4) != HAL_OK) { /* PWM generation Error */ Error_Handler(); }
}

/**
* @brief TIM MSP Initialization. Function is called from HAL_TIM_PWM_Init and overrides the weak HAL implementation
*        It configures the hardware resources needed for Pwm Generation:
*           - Peripheral's clock enable
*           - Peripheral's GPIO Configuration
* @param htim: TIM handle pointer
* @retval None
*/
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
  GPIO_InitTypeDef   GPIO_InitStruct;
  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* TIMx Peripheral clock enable. Timer 3 and 4 are used for Pwm generation */
  __HAL_RCC_TIM3_CLK_ENABLE();
  __HAL_RCC_TIM4_CLK_ENABLE();

  /* Enable all GPIO Channels Clock requested. Timer 3 on Port B and Timer 4 on Port D */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /* Common configuration for all channels on both Timer 3 and 4*/
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;               //GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  /* Configure TIM3. Note: Channel 3 is NOT configured since PB0 is already used by LED1.
  PB.04 (pin 19 in CN7  connector) (TIM3_Channel1),
  PB.05 (pin 13 in CN7  connector) (TIM3_Channel2),
  PB.00 (pin 31 in CN10 connector) (TIM3_Channel3),
  PB.01 (pin  7 in CN10 connector) (TIM3_Channel4) in output, push-pull, alternate function mode
  */

  GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
  GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5 /*| GPIO_PIN_0*/ | GPIO_PIN_1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);


  /* Configure TIM4.
  PD.12 (pin 21 in CN10 connector) (TIM4_Channel1),
  PD.13 (pin 19 in CN10 connector) (TIM4_Channel2),
  PD.14 (pin 16 in CN7  connector) (TIM4_Channel3),
  PD.15 (pin 18 in CN7  connector) (TIM4_Channel4) in output, push-pull, alternate function mode
  */

  GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
  GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}


// Function that sets duty by writing to the input TIMER capture/compare register directly which is faster
// than the HAL way which first calls HAL_TIM_PWM_ConfigChannel and then HAL_TIM_PWM_Start.
// Shall only be called after the PWM TIMER has been  configured and started.
void Pwm_SetDuty(TIM_TypeDef *TIMx_Pwm, uint32_t Channel, uint32_t Duty)
{
  switch (Channel)
  {
  case TIM_CHANNEL_1:
    TIMx_Pwm->CCR1 = Duty;
    break;
  case TIM_CHANNEL_2:
    TIMx_Pwm->CCR2 = Duty;
    break;
  case TIM_CHANNEL_3:
    TIMx_Pwm->CCR3 = Duty;
    break;
  case TIM_CHANNEL_4:
    TIMx_Pwm->CCR4 = Duty;
    break;
  
  default:  // Do nothing
    break;
  }
}

void Pwm_SetPeriod(TIM_HandleTypeDef *htim, uint32_t Period)
{
  /* Set the Auto-reload value */
  htim->Instance->ARR = Period;
}

//=====================================================
void Pwm_20ms(void)
{
  static uint32_t PwmDuty = 0;

  PwmDuty += 4;
  if (PwmDuty >= PWM_MAX_DUTY)
  {
    PwmDuty = 0;
  }

  Pwm_SetDuty(TIM3, TIM_CHANNEL_4, PwmDuty);
}

void Pwm_500ms(void)
{
  static uint32_t PwmDuty = 1;

  PwmDuty *= 2;
  if (PwmDuty >= 4*PWM_MAX_DUTY)
  {
    PwmDuty = 1;
  }

  Pwm_SetDuty(TIM4, TIM_CHANNEL_1, PwmDuty);

}