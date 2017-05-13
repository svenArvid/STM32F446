/**
* @param  None
* @retval None
*/

#include "NeoPixel.h"
#include "Util.h"
#include "Pwm.h"

static int32_t PreComputedDuty = 0;
static int32_t currLED = 0;
static int32_t ColorBit = 0;
static bool NeoPixelBusy = FALSE;
static uint32_t ColorData[NUM_LEDS] = { BIT_R7, BIT_G0, BIT_B4 | BIT_B0 };

/* Timer handler declaration */
TIM_HandleTypeDef        Timer14Handle;

//======================================================================
/**
* @brief  This function configures the TIM14 as a time base source and output compare
*         setup of GPIO pin connected to the Neo Pixel data input.
*         It Enables the Timer 14 interrupt and set it's priority.
*/
void NeoPixel_Init(void)
{
  GPIO_InitTypeDef      GPIO_InitStruct;
  RCC_ClkInitTypeDef    clkconfig;
  uint32_t              uwTimclock;
  uint32_t              uwAPB1Prescaler = 0U;
  uint32_t              uwPrescalerValue = 0U;
  uint32_t              pFLatency;

  TIM_OC_InitTypeDef sConfig;        /* Timer Output Compare Configuration Structure declaration */

  /* Enable Port F, (GPIO PF9 is used)*/
  __HAL_RCC_GPIOF_CLK_ENABLE();

  /* Configure Pin */
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF9_TIM14;
  GPIO_InitStruct.Pin = NEO_PIXEL_DATA_PIN;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure the TIM14 IRQ priority */
  HAL_NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn, 10U, 0U);   // Interrupt prio number 10, i.e. middle high prio. 

  /* Enable the TIM14 global Interrupt */
  HAL_NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);

  /* Enable TIM14 clock */
  __HAL_RCC_TIM14_CLK_ENABLE();

  /* Get clock configuration */
  HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

  /* Get APB1 prescaler */
  uwAPB1Prescaler = clkconfig.APB1CLKDivider;

  /* Compute TIM14 clock */
  if (uwAPB1Prescaler == RCC_HCLK_DIV1)
  {
    uwTimclock = HAL_RCC_GetPCLK1Freq();
  }
  else
  {
    uwTimclock = 2 * HAL_RCC_GetPCLK1Freq();
  }

  /* Compute the prescaler value to have TIM14 counter clock equal to 45 MHz */
  uwPrescalerValue = (uint32_t)((uwTimclock / 45000000U) - 1U);
  
  /* Initialize TIM14 */
  Timer14Handle.Instance = TIM14;

  Timer14Handle.Init.Period = PERIOD_TIME;
  Timer14Handle.Init.Prescaler = uwPrescalerValue;
  Timer14Handle.Init.ClockDivision = 0;
  Timer14Handle.Init.CounterMode = TIM_COUNTERMODE_UP;

  HAL_TIM_OC_Init(&Timer14Handle);

  /*##-2- Configure the Output Compare channel #########################################*/
  sConfig.OCMode = TIM_OCMODE_PWM1;
  sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
  //sConfig.Pulse = ;
  HAL_TIM_OC_ConfigChannel(&Timer14Handle, &sConfig, TIM_CHANNEL_1);
}

//------------------------------------------------------
void NeoPixel_TxStart(void)
{
  int32_t bitVal;

  NeoPixelBusy = TRUE;
  currLED = 0;
  ColorBit = MOST_SIG_COLOR_BIT;
  
  bitVal = Util_BitRead(ColorData[currLED], ColorBit);
  PreComputedDuty = bitVal ? TIME_ONE_HIGH : TIME_ZERO_HIGH;

  // Note: Timing is very critical for NeoPxel. Important to set counter here to get well defined behavior.
  // Counter shall be larger than duty at this point, so that pin is low. 
  // When counter wraps around pin will be set high and the interrupt will be called which will update the duty value. 
  Pwm_SetDuty(TIM14, TIM_CHANNEL_1, TIME_ZERO_HIGH);
  Pwm_SetPeriod(&Timer14Handle, PERIOD_TIME);
  Timer14Handle.Instance->CNT = PERIOD_TIME / 2; 

  /* Start TIMER 14 in interrupt mode */
  HAL_TIM_OC_Start(&Timer14Handle, TIM_CHANNEL_1);
  HAL_TIM_Base_Start_IT(&Timer14Handle);
}


void NeoPixel_100ms(void)
{
  NeoPixel_TxStart();
}

/**
* @brief  Period elapsed interrupt (non blocking mode)
* @note   To minimize overhead all code is executed directly in IRQ handler. I.e. NOT via HAL_TIM_IRQHandler().
* The IRQ sends the data waveform to NeoPixel. One interrupt for each bit sent. 
* The first part of this IRQ i.e. writing of the duty is very time critical. Thus this is done directly instead of making function calls. 
* Also the duty is precomputed (for next time) later in the IRQ when timing is less critical.
*/
void TIM8_TRG_COM_TIM14_IRQHandler(void)
{
  /* TIM Update event */
  if (__HAL_TIM_GET_FLAG(&Timer14Handle, TIM_FLAG_UPDATE) != RESET)
  {
    if (__HAL_TIM_GET_IT_SOURCE(&Timer14Handle, TIM_IT_UPDATE) != RESET)
    {
      //------------------ NeoPixel part starts ------------------
      if (currLED < NUM_LEDS)
      {
        TIM14->CCR1 = PreComputedDuty; // Note: Writing duty is very time critical. Shall be done within 0.4 us after IRQ occurs. Thus the value is precomputed

        if (ColorBit != 0) {
          ColorBit--;
        }
        else // Move to next LED
        {
          ColorBit = MOST_SIG_COLOR_BIT;
          currLED++;
        }
        // Precompute value that shall be used next time    
        PreComputedDuty = Util_BitRead(ColorData[currLED], ColorBit) ? TIME_ONE_HIGH : TIME_ZERO_HIGH;  // Note: When currLED == NUM_LEDS we will read outside array, but that value will never be used.
      }
      else  // All Neo Pixel data sent. Stop timer and disable Interrupt. 
      {  
        currLED = 0;
        NeoPixelBusy = FALSE;
        HAL_TIM_OC_Stop(&Timer14Handle, TIM_CHANNEL_1);
        HAL_TIM_Base_Stop_IT(&Timer14Handle);
      }
      //------------------ END OF NeoPixel part ------------------
      __HAL_TIM_CLEAR_IT(&Timer14Handle, TIM_IT_UPDATE);
    }
  }
}