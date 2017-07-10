/**
******************************************************************************
* @file    /Src/Util.c
* @author  Joakim Carlsson
* @version V1.0
* @date    5-Jan-2017
* @brief   General help functions that can be used in the application
*
******************************************************************************
*/

#include "Util.h"
#include <stdlib.h>

// Reset dominant SR latch. Toggled is True when state changes, i.e. for one tick.
bool Util_SetSRLatchState(Util_SRLatch *latch, bool Set, bool Reset)
{
  bool PrevState = latch->State;

  latch->State = (Set || latch->State) && !Reset;
  latch->Toggled = latch->State != PrevState;

  return latch->State;
}


// Linear Interpolation. To get correct integer calculations, some calculations multiply and divide with MULTIPLIER (2^12).
// Xaxis (and Yaxis) is allowed to be decreasing. 
// Note: last parameter is maxIndex, i.e. ArraySize - 1
int32_t Util_Interpolate(int32_t x, const int16_t Xaxis[], const int16_t Yaxis[], uint32_t maxIndex)
{
  int32_t frac;
  uint32_t Idx;
  uint32_t i_xMax = maxIndex;
  uint32_t i_xMin = 0;


  if (Xaxis[0] > Xaxis[maxIndex]) { // If Xaxis values are decreasing
    i_xMax = 0;
    i_xMin = maxIndex;
  }
    
  // Check if x is out of bounds and if so return max/min value
  if (x <= Xaxis[i_xMin]) {
    return Yaxis[i_xMin];
  }
  else if (x >= Xaxis[i_xMax]) {
    return Yaxis[i_xMax];
  }
  else 
  {
    /* Binary Search.  i_xMax is the index for the larger x value. Thus if Xaxis is decreasing i_xMax < i_XxMin */
    Idx = maxIndex / 2;
  
    while (abs(i_xMax - i_xMin) > 1) 
    {
      if (x < Xaxis[Idx]) {
        i_xMax = Idx;
      }
      else {
        i_xMin = Idx;
      }

      Idx = (i_xMax + i_xMin) / 2;
    }

    // Maybe Add Division by zero check here!

    frac = MULTIPLIER*(x - Xaxis[i_xMin]) / (Xaxis[i_xMax] - Xaxis[i_xMin]);  // Fixed point calculation
  }

  return ((MULTIPLIER - frac)*Yaxis[i_xMin] + frac*Yaxis[i_xMax] + MULTIPLIER/2) / MULTIPLIER; // + MULTIPLIER/2 to get correct rounding
}


// Remaps a number from one range to another, it does NOT constrain values to within the range.
// (x_max - x_min)/2 is added to get correct rounding, (equal to adding 0.5) 
// Example: newVal = map(val, 0, 1023, 128, 255), maps the value from range [0, 1023] to [128, 255]
int32_t Util_Map(int32_t x, int32_t x_min, int32_t x_max, int32_t y_min, int32_t y_max)
{
  int32_t y = ((x - x_min) * (y_max - y_min) + (x_max - x_min) / 2) / (x_max - x_min) + y_min;
  return y;
}

// Timer object, updates a timer. Shall be called periodically. Output is TRUE when timer has finished. 
// If TimeoutVal parameter is 0 the timer is disabled, i.e. return value is always False.
bool Util_SetTimerState(Util_Timer* timer, bool Start, bool Reset)
{
  bool TimerRunning = (Start || timer->CurrentTimerVal >= timer->TimeoutVal) && !Reset;

  if (timer->TimeoutVal == 0)
  {
    timer->CurrentTimerVal = 0;
    return FALSE;
  }
  else
  {
    if (TimerRunning)
    {
      if (timer->CurrentTimerVal < timer->TimeoutVal) {
        timer->CurrentTimerVal++;
      }
      //else timer has finsihed
    }
    else {  // timer not running
      timer->CurrentTimerVal = 0;
    }
    return timer->CurrentTimerVal >= timer->TimeoutVal; // TRUE when timer has finished
  }
}


uint8_t Util_GetTimerState(const Util_Timer* timer)
{
  if (timer->CurrentTimerVal == 0) {
    return TIMER_ZERO;
  }
  else if (timer->CurrentTimerVal >= timer->TimeoutVal) {
    return TIMER_FINISHED;
  }
  else {
    return TIMER_RUNNING;
  }
}


// Rate limiter. Rates per tick for ramping up and down shall be specified, both shall be positive values.  
int32_t Util_SetRampState(Util_Ramp* ramp, int32_t Input)
{
  int32_t TempVal = 0;

  if (ramp->CurrentVal < Input)
  {
    TempVal = ramp->CurrentVal + ramp->RateUp;
    ramp->CurrentVal = Util_Min(TempVal, Input);
  }
  else if (ramp->CurrentVal > Input)
  {
    TempVal = ramp->CurrentVal - ramp->RateDown;
    ramp->CurrentVal = Util_Max(TempVal, Input);
  }
  return ramp->CurrentVal;
}

int32_t Util_FilterState(Util_Filter* filter, int32_t Input) {
  filter->State = (Input * MULTIPLIER + (filter->Samples - 1)*filter->State) / filter->Samples;
  return filter->State / MULTIPLIER;
}

void Util_SetFilterState(Util_Filter* filter, int32_t Input) {
  filter->State = Input * MULTIPLIER;
}

int32_t Util_Limit(int32_t Input, int32_t Min, int32_t Max)
{
  if (Input < Min)
    return Min;
  else if (Input > Max)
    return Max;
  else
    return Input;  
}
