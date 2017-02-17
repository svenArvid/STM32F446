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

// Reset dominant SR latch. Toggled is True when state changes, i.e. for one tick.
bool Util_SetSRLatchState(Util_SRLatch *latch, bool Set, bool Reset)
{
  bool PrevState = latch->State;

  latch->State = (Set || latch->State) && !Reset;
  latch->Toggled = latch->State != PrevState;

  return latch->State;
}


// Linear Interpolation. To get correct integer calculations, some calculations multiply and divide with MULT_FAC (2^12).
// Xaxis (and Yaxis) is allowed to be decreasing. 
// Note: last parameter is maxIndex, i.e. ArraySize - 1
#define MULT_FAC (4096)   
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

    frac = MULT_FAC*(x - Xaxis[i_xMin]) / (Xaxis[i_xMax] - Xaxis[i_xMin]);  // Fixed point calculation
  }

  return ((MULT_FAC - frac)*Yaxis[i_xMin] + frac*Yaxis[i_xMax] + MULT_FAC/2) / MULT_FAC; // + MULT_FAC/2 to get correct rounding
}