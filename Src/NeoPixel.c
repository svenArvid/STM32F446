/**
******************************************************************************
* @file    /Src/NeoPixel.c
* @author  Joakim Carlsson
* @version V1.0
* @date    13-May-2017
* @brief   Timer 14 is used to produce command signal to NeoPixel LEDs.
*
******************************************************************************
*/
#include <string.h>
#include "NeoPixel.h"
#include "Util.h"
#include "Pwm.h"
#include "SensorMgr.h"

/* Timer handler declaration */
TIM_HandleTypeDef        Timer14Handle;

NeoPixel_TxData NeoTx;
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

  TIM_OC_InitTypeDef sConfig;

  /* Enable Port F, (GPIO PF9 is used) */
  __HAL_RCC_GPIOF_CLK_ENABLE();

  /* Configure Pin */
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF9_TIM14;
  GPIO_InitStruct.Pin = NEO_PIXEL_DATA_PIN;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Enable TIM14 interrupt and configure the IRQ priority */
  HAL_NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn, 5U, 0U);   // Interrupt prio need to be high since IRQ is very time critical. 
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

  HAL_TIM_OC_ConfigChannel(&Timer14Handle, &sConfig, TIM_CHANNEL_1);
}

static int16_t NeoPixel_NextLED(int16_t LEDindx, int16_t inc)
{
  int NextLEDIndx = LEDindx + inc;

  if (NextLEDIndx < 0) {
    NextLEDIndx = NextLEDIndx + NUM_LEDS;
  }
  else if (NextLEDIndx >= NUM_LEDS) {
    NextLEDIndx = NextLEDIndx - NUM_LEDS;
  }
  return NextLEDIndx;
}
//------------------------------------------------------
void NeoPixel_TxStart(void)
{
  int32_t bitVal;

  NeoTx.TxBusy = TRUE;
  NeoTx.currLED = 0;
  NeoTx.ColorBit = MOST_SIG_COLOR_BIT;
  
  bitVal = Util_BitRead(NeoTx.ColorData[NeoTx.currLED], NeoTx.ColorBit);
  NeoTx.PreComputedDuty = bitVal ? TIME_ONE_HIGH : TIME_ZERO_HIGH;

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

//------------------------------------------------------
static const uint32_t TivoliColor[NUM_LEDS] = { BIT_R6, BIT_G2, BIT_B4 | BIT_B0, BIT_R4 | BIT_B5, BIT_G5 | BIT_B5, BIT_G5 | BIT_R6, BIT_G5 | BIT_R4 | BIT_B3,
                                              BIT_G3 | BIT_R3 | BIT_B3, BIT_R5 | BIT_B4, BIT_G4 | BIT_R5 | BIT_B3, BIT_R2 | BIT_B5, BIT_G2 | BIT_R1 | BIT_B5,
                                              BIT_G3 | BIT_R3 | BIT_B4, BIT_G3 | BIT_R4 | BIT_B3, BIT_G4 | BIT_R3 | BIT_B2, BIT_G5 | BIT_R4 | BIT_B3 };

#define TURN_ON_LEDS     1
#define TIMER_ACTIVE     2
#define TURN_OFF_LEDS    3
#define TIVOLI_TIMEOUT  10

static void NeoPixel_TivoliApp(void)
{
  static int16_t TivoliState = TURN_ON_LEDS;
  static int16_t SetVal = NUM_LEDS - 1;
  static int16_t Timer = 0;
  static int16_t CurrentLED = 0;

  switch (TivoliState)
  {
  case TURN_ON_LEDS:
      NeoTx.ColorData[CurrentLED] = TivoliColor[CurrentLED];
      if (CurrentLED == SetVal) 
      {
        TivoliState = TURN_OFF_LEDS;
      }
      break;
  case TURN_OFF_LEDS:
    NeoTx.ColorData[CurrentLED] = 0;
    if (CurrentLED == SetVal) 
    {
      TivoliState = TURN_ON_LEDS;
      SetVal = NeoPixel_NextLED(SetVal, -1);
      NeoTx.ColorData[CurrentLED] = TivoliColor[CurrentLED];
      CurrentLED = NeoPixel_NextLED(CurrentLED, +1);
      NeoTx.ColorData[CurrentLED] = TivoliColor[CurrentLED];
    }
    break;
  case TIMER_ACTIVE:               // NOT USED NOW
    if (++Timer >= TIVOLI_TIMEOUT)
    {
      Timer = 0;
      TivoliState = SetVal == 0 ? TURN_ON_LEDS : TURN_OFF_LEDS;  // Switch ramp direction
    }
    break;
  }

  NeoPixel_TxStart();
  CurrentLED = NeoPixel_NextLED(CurrentLED, +1);
}

//----------------------------------------------
#define MIN_RED    0
#define MAX_RED   64
#define MIN_BLUE   0
#define MAX_BLUE  64
#define MIN_GREEN  0
#define MAX_GREEN 24

static void NeoPixel_TemeratureApp(void)
{
  int32_t Blue, Red, Green;
  uint32_t TempColor = 0;

  Red = Util_Map(RoomTempSnsr.Temperature, 200, 500, MIN_RED, MAX_RED);
  Red = Util_Limit(Red, MIN_RED, MAX_RED);
  
  Blue = Util_Map(RoomTempSnsr.Temperature, 500, 200, MIN_BLUE, MAX_BLUE);
  Blue = Util_Limit(Blue, MIN_BLUE, MAX_BLUE);

  Green = Util_Map(RoomTempSnsr.Temperature, 500, 200, MIN_GREEN, MAX_GREEN);
  Green = Util_Limit(Green, MIN_GREEN, MAX_GREEN);

  TempColor  = Green << 16;
  TempColor |= Red << 8;
  TempColor |= Blue;

  for (int i = 0; i < NUM_LEDS; i++) {
    NeoTx.ColorData[i] = TempColor;
  }

  NeoPixel_TxStart();

}

void NeoPixel_100ms(void)
{
  NeoPixel_TivoliApp();
  //NeoPixel_TemeratureApp();
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
      if (NeoTx.currLED < NUM_LEDS)
      {
        TIM14->CCR1 = NeoTx.PreComputedDuty; // Note: Writing duty is very time critical. Shall be done within 0.4 us after IRQ occurs. Thus the value is precomputed

        if (NeoTx.ColorBit != 0) {
          NeoTx.ColorBit--;
        }
        else // Move to next LED
        {
          NeoTx.ColorBit = MOST_SIG_COLOR_BIT;
          NeoTx.currLED++;
        }
        // Precompute value that shall be used next time    
        NeoTx.PreComputedDuty = Util_BitRead(NeoTx.ColorData[NeoTx.currLED], NeoTx.ColorBit) ? TIME_ONE_HIGH : TIME_ZERO_HIGH;  // Note: When currLED == NUM_LEDS we will read outside array, but that value will never be used.
      }
      else  // All Neo Pixel data sent. Stop timer and disable Interrupt. 
      {  
        NeoTx.currLED = 0;
        NeoTx.TxBusy = FALSE;
        HAL_TIM_OC_Stop(&Timer14Handle, TIM_CHANNEL_1);
        HAL_TIM_Base_Stop_IT(&Timer14Handle);
      }
      //------------------ END OF NeoPixel part ------------------
      __HAL_TIM_CLEAR_IT(&Timer14Handle, TIM_IT_UPDATE);
    }
  }
}