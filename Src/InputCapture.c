/**
******************************************************************************
* @file    /Src/InputCapture.c
* @author  Joakim Carlsson
* @version V1.0
* @date    23-Jan-2017
* @brief   Use Timer 2 and Timer 5 which are 32-bit timers with 4 channels for Input Capture. For Timer 5 only use channel 1 and 4 because channel 2 and 3 are not connected to PCB. 
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
TIM_HandleTypeDef        Timer5Handle;


/**
* @brief  This function configures the TIM2 and TIM5 as a time base source used for Input Capture and time measurement.
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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Configure IO */
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;      // ;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;

  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;  // TIM2 uses Pins 8,9,10,11 on Port B 
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_3;  // TIM5 uses Pins 0,3 on Port A 
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


  /* Enable TIM2 and TIM5 clock */
  __HAL_RCC_TIM2_CLK_ENABLE();
  __HAL_RCC_TIM5_CLK_ENABLE();

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

  // ------ Setup Timer 2 ------
  /* Compute the prescaler value to have TIM2 counter clock equal to 10 MHz */
  uwPrescalerValue = (uint32_t)((uwTimclock / TIM2_CLOCK_FREQ) - 1U);

  /* Initialize TIM2 */
  Timer2Handle.Instance = TIM2;

  Timer2Handle.Init.Period = 0xFFFFFFFF;
  Timer2Handle.Init.Prescaler = uwPrescalerValue;
  Timer2Handle.Init.ClockDivision = 0;
  Timer2Handle.Init.CounterMode = TIM_COUNTERMODE_UP;

  HAL_TIM_IC_Init(&Timer2Handle);

  /* Configure the Input Capture channels for Timer 2 */
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

  // ------ Timer 2 finished ------

  // ------ Setup Timer 5 ------
  /* Compute the prescaler value to have TIM5 counter clock equal to 100 KHz */
  uwPrescalerValue = (uint32_t)((uwTimclock / 100000U) - 1U);

  /* Initialize TIM5 */
  Timer5Handle.Instance = TIM5;

  Timer5Handle.Init.Period = 0xFFFFFFFF;
  Timer5Handle.Init.Prescaler = uwPrescalerValue;
  Timer5Handle.Init.ClockDivision = 0;
  Timer5Handle.Init.CounterMode = TIM_COUNTERMODE_UP;

  HAL_TIM_IC_Init(&Timer5Handle);

  /* Configure the Input Capture channels for Timer 5 */
  sConfig.ICPolarity = TIM_ICPOLARITY_BOTHEDGE;
  sConfig.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfig.ICFilter = 0xF;                        // f_sampling = f_DTS/32, N=8 i.e. 1*16*16 = 256 -> glitch filter is 256 DTS (internal) clock cycles (think it is 90 MHz)
  sConfig.ICPrescaler = TIM_ICPSC_DIV1;          // Capture performed each time an edge is detected on the capture input


  HAL_TIM_IC_ConfigChannel(&Timer5Handle, &sConfig, TIM_CHANNEL_1);
  HAL_TIM_IC_ConfigChannel(&Timer5Handle, &sConfig, TIM_CHANNEL_4);

  /* Start Timer and Input Capture on channels 1 and 4 */
  /*Configure the TIM5 IRQ priority */
  HAL_NVIC_SetPriority(TIM5_IRQn, 13U, 0U);   // Interrupt prio number 13, i.e. low prio. 

  HAL_NVIC_EnableIRQ(TIM5_IRQn);              // Enable the TIM5 global Interrupt

  HAL_TIM_IC_Start_IT(&Timer5Handle, TIM_CHANNEL_1);
  HAL_TIM_IC_Start(&Timer5Handle, TIM_CHANNEL_4);
}

static uint32_t InputCapture_ReadCCRx(const InputCapture_t *Ic)
{
  uint32_t RegisterVal = 0;

  switch (Ic->Channel)
  {
  case TIM_CHANNEL_1:
    RegisterVal = Ic->TimInstance->CCR1;
    break;
  case TIM_CHANNEL_2:
    RegisterVal = Ic->TimInstance->CCR2;
    break;
  case TIM_CHANNEL_3:
    RegisterVal = Ic->TimInstance->CCR3;
    break;
  case TIM_CHANNEL_4:
    RegisterVal = Ic->TimInstance->CCR4;
    break;
  default:
    break;
  }

  return RegisterVal;
}

uint32_t InputCapture_GetCurrentTime(void)
{
  return TIM2->CNT;
}

void InputCapture_UpdatePeriod(InputCapture_t *Ic, const uint16_t FreqTimeout)
{
  uint32_t NewTrigTime;

  if (Ic->TimInstance->SR & Ic->Tim_SR_CCxIF)	 // If edge has been detected
  {
    NewTrigTime = InputCapture_ReadCCRx(Ic);  // Timer value when positive edge was detected. CCxIF flag is cleared when reading CCRx register 

    if (Ic->TimeOut < FreqTimeout)
    {
      Ic->Period = NewTrigTime - Ic->LastTrigTime;
    }
    else
    {
      Ic->Period = 0;
    }
    Ic->LastTrigTime = NewTrigTime;   //Save for next time
    Ic->TimeOut = 0;
  }
  else
  {
    if (++Ic->TimeOut >= FreqTimeout)
    {
      Ic->TimeOut = FreqTimeout;
      Ic->Period = 0;
    }
  }
}


uint16_t InputCapture_RelativePhaseLag(const InputCapture_t *Ic1, const InputCapture_t *Ic2)
{
  uint32_t Ic1_TimeStampOfPrevPulse;                         
  uint32_t PhaseLagRel;

  Ic1_TimeStampOfPrevPulse = Ic1->LastTrigTime - Ic1->Period;    // P1 = P1' - period
  
  //                            <------------------- 100 % --------------->
  // Timing of pulses:    |--- P1 -------------- P2 --------------------- P1' -------------- (P2') ---|
  //                            < -- Phase Lag -->

  PhaseLagRel = (1000UL * (Ic2->LastTrigTime - Ic1_TimeStampOfPrevPulse) / Ic1->Period);

  PhaseLagRel %= 1000UL;    // If Ic2->LastValTimer is P2', i.e. "newer" than P1', PhaseLag > 1000, thus use modulo. Ex: PhaseLag is 1250: 1250 % 1000 is 250

  return (uint16_t)PhaseLagRel;
}


bool InputCapture_MissedPulse(const InputCapture_t *IcPrimary, const InputCapture_t *IcNeighbour)
{
  uint32_t CurrentTime;
  uint32_t Neighbour_TimeStampOfPrevPulse;
  bool PrimaryMissedPulse;

  CurrentTime = InputCapture_GetCurrentTime();    //Read actual value of timer
  
  Neighbour_TimeStampOfPrevPulse = IcNeighbour->LastTrigTime - IcNeighbour->Period;                       // Timestamp of second last pulse for neighbour sensor
  PrimaryMissedPulse = (CurrentTime - Neighbour_TimeStampOfPrevPulse < CurrentTime - IcPrimary->LastTrigTime);

  return PrimaryMissedPulse;
}


bool InputCapture_SignalInRange(const InputCapture_t *Ic, uint32_t MinFreqHz, uint32_t MaxFreqHz)
{
  uint32_t MaxPeriod = MinFreqHz;      // Note that Min frequency corresponds to max period and vice versa
  uint32_t MinPeriod = MaxFreqHz;

  return (MinPeriod < Ic->Period) && (Ic->Period < MaxPeriod);
}


uint32_t InputCapture_GetFrequency(const InputCapture_t *Ic)
{ 
  uint32_t CentiHz;
  
  if (Ic->Period != 0)
  {
    switch ((uint32_t)(Ic->TimInstance))
    {
    case (uint32_t)TIM2:
      CentiHz = (TIM2_CLOCK_FREQ / Ic->Period);  // Frequency in centi Hz
      break;

    default:
      CentiHz = 0;
      break;
    }
  }
  else {
    CentiHz = 0;
  }
  return CentiHz;
}


