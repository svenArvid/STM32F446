/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __INPUT_CAPTURE_H
#define __INPUT_CAPTURE_H

#include "ProjectDefs.h"

#define FREQ_TIMEOUT 1000
#define MIN_PERIOD   10000U     // Timer clock is 10MHz and we check the capture register every 1ms, so not allowed to be faster than 1 KHz
#define MAX_PERIOD   10000000U  // 1 sec

#define FREQ_1p0_HZ   MAX_PERIOD 
#define FREQ_1000_HZ  MIN_PERIOD

#define TIM2_CLOCK_FREQ  10000000U

typedef struct 
{
  volatile TIM_TypeDef  *TimInstance;     // Timer Register base address 
  volatile uint16_t Channel;
  uint16_t Tim_SR_CCxIF;                  // Timer Capture/Compare interrupt Flag 
  
  volatile uint32_t Period;
  volatile uint32_t LastTrigTime;

  uint16_t TimeOut;
} InputCapture_t;

extern TIM_HandleTypeDef        Timer2Handle;
extern TIM_HandleTypeDef        Timer5Handle;

/**
 * Initializes the Input Capture module.
 * All other functions in the module will assume this function has been called first.
 */
extern void InputCapture_Init(void);

/**
 * Return raw Counter value of TIM2.
 */
uint32_t InputCapture_GetCurrentTime(void);

/**
 * for the given Input capture object, period time  i.e. time between positive edges is updated.
 */
void InputCapture_UpdatePeriod(InputCapture_t *Ic, const uint16_t FreqTimeout);
/**
 * Returns the time phase lag relative to the period time, i.e. how much Ic2 lags Ic1 as a permille value [0, 1000]
 * Example: If Ic1 Period is 100 ms and Ic2 pulse is captured 40 ms after Ic1, the return value will be 400
 *
 */
uint16_t InputCapture_RelativePhaseLag(const InputCapture_t *Ic1, const InputCapture_t *Ic2);

/*
 * Test if the IcNeighbour has received (at least) 2 pulses AFTER the last pulse captured by IcPrimary, 
 * which indicates that IcPrimary has missed a pulse
 */
bool InputCapture_MissedPulse(const InputCapture_t *IcPrimary, const InputCapture_t *IcNeighbour);

/*
 * Test if the frequency signal is in the range specified by MinFreqHz and MaxFreqHz
 * @param MinFreqHz    Defines the lower limit in Hz and shall be one of the macros FREQ_XXX_HZ 
 * @param MaxFreqHz    Defines the upper limit in Hz and shall be one of the macros FREQ_XXX_HZ
 */
bool InputCapture_SignalInRange(const InputCapture_t *Ic, uint32_t MinFreqHz, uint32_t MaxFreqHz);

/*
 * Returns the frequency in centi Hz. 
 * Example: f=50 Hz -> return 50*100 = 5000 cHz
*/
uint32_t InputCapture_GetFrequency(const InputCapture_t *Ic);

#endif // __INPUT_CAPTURE_H
