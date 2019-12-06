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

#define MULTIPLIER  (4096)
#define MULTIPLIER2 (1024)

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
// Value gets extrapolated if outside range.
int32_t Util_Interpolate(int32_t x, const int16_t Xaxis[], const int16_t Yaxis[], uint32_t arrayLen)
{
  int32_t frac;
  uint32_t maxIndex = arrayLen - 1;
  uint32_t Idx;
  uint32_t i_xMax = maxIndex;
  uint32_t i_xMin = 0;

  if (Xaxis[0] > Xaxis[maxIndex]) { // If Xaxis values are decreasing
    i_xMax = 0;
    i_xMin = maxIndex;
  }
    
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

  // TODO: Add Division by zero check here or assert.

  frac = MULTIPLIER*(x - Xaxis[i_xMin]) / (Xaxis[i_xMax] - Xaxis[i_xMin]);  // Fixed point calculation

  return ((MULTIPLIER - frac)*Yaxis[i_xMin] + frac*Yaxis[i_xMax]) / MULTIPLIER;
}

int32_t Util_Interpolate2D(int32_t x, int32_t y, const int16_t Xaxis[], const int16_t Yaxis[], int16_t map[], uint32_t xArrayLen, uint32_t yArrayLen)
{
	uint32_t maxIndex = xArrayLen - 1;
	uint32_t index;
	int32_t fracX;
	uint32_t i_xMax = maxIndex;
	uint32_t i_xMin = 0;
	int32_t fracY;
	uint32_t i_yMax;
	uint32_t i_yMin = 0;
	int32_t q11, q12, q21, q22;

	if (Xaxis[0] > Xaxis[maxIndex]) { // If Xaxis values are decreasing
		i_xMax = 0;
		i_xMin = maxIndex;
	}

	/* Binary Search.  i_xMax is the index for the larger x value. Thus if Xaxis is decreasing i_xMax < i_XxMin */
	index = maxIndex / 2;

	while (abs(i_xMax - i_xMin) > 1)
	{
		if (x < Xaxis[index]) {
			i_xMax = index;
		}
		else {
			i_xMin = index;
		}

		index = (i_xMax + i_xMin) / 2;
	}

	// TODO: Add Division by zero check here or assert.
	fracX = MULTIPLIER2 * (x - Xaxis[i_xMin]) / (Xaxis[i_xMax] - Xaxis[i_xMin]);  // Fixed point calculation

	// Repeat for y axis
	maxIndex = yArrayLen - 1;
	i_yMax = maxIndex;

	if (Yaxis[0] > Yaxis[maxIndex]) {
		i_yMax = 0;
		i_yMin = maxIndex;
	}

	/* Binary Search */
	index = maxIndex / 2;

	while (abs(i_yMax - i_yMin) > 1)
	{
		if (y < Yaxis[index]) {
			i_yMax = index;
		}
		else {
			i_yMin = index;
		}

		index = (i_yMax + i_yMin) / 2;
	}

	// TODO: Add Division by zero check here or assert.
	fracY = MULTIPLIER2 * (y - Yaxis[i_yMin]) / (Yaxis[i_yMax] - Yaxis[i_yMin]);  // Fixed point calculation

	q11 = map[i_xMin*yArrayLen + i_yMin] * (MULTIPLIER2 - fracX) * (MULTIPLIER2 - fracY);  
	q21 = map[i_xMax*yArrayLen + i_yMin] * fracX  * (MULTIPLIER2 - fracY);                 
	q12 = map[i_xMin*yArrayLen + i_yMax] * (MULTIPLIER2 - fracX) *                fracY;   
	q22 = map[i_xMax*yArrayLen + i_yMax] * fracX  *                fracY;                  

	return (q11 + q21 + q12 + q22) / MULTIPLIER2 / MULTIPLIER2;
}


// Remaps a number from one range to another, it does NOT constrain values to within the range. 
// Example: newVal = map(val, 0, 1023, 128, 255), maps the value from range [0, 1023] to [128, 255]
int32_t Util_Map(int32_t x, int32_t x_min, int32_t x_max, int32_t y_min, int32_t y_max)
{
  if (x_max == x_min)
  {
    return y_min;  // Avoid division by zero
  }
  else
  {
    int32_t y = ((x - x_min) * (y_max - y_min)) / (x_max - x_min) + y_min;
    return y;
  }
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

// Backward Euler instead of Forward Euler, more natural and no risk for division by zero
// Note: Input * MULTIPLIER * filter->Samples shall be less than 2^31 to avoid overflow
int32_t Util_FilterState(Util_Filter* filter, int32_t Input) {
  filter->State = (Input * MULTIPLIER + (filter->Samples)*filter->State) / (filter->Samples + 1);
  return filter->State / MULTIPLIER;
}

void Util_SetFilterState(Util_Filter* filter, int32_t Input) {
  filter->State = Input * MULTIPLIER;
}

int32_t Util_GetFilterState(Util_Filter* filter) {
  return filter->State / MULTIPLIER;
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

int32_t Util_Abs(int32_t Input)
{
  if (Input < 0)
  {
    return -Input;
  }
  else
  {
    return Input;
  }
}

bool Util_InRange(int32_t Input, int32_t LowerLim, int32_t UpperLim)
{
  return ( Input >= LowerLim  &&  Input <= UpperLim );
}
