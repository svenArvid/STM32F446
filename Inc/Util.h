/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UTIL_H
#define __UTIL_H

#include "ProjectDefs.h"


#define Util_BitRead(value, bit) (((value) >> (bit)) & 0x01)      // Reads a bit of a numeric variable.
#define Util_BitSet(value, bit)  ((value) |= (1UL << (bit)))      // Sets a bit of a numeric variable.
#define Util_BitClear(value, bit) ((value) &= ~(1UL << (bit)))    // Clears a bit of a numeric variable.
#define Util_BitWrite(value, bit, bitvalue) (bitvalue ? Util_BitSet(value, bit) : Util_BitClear(value, bit))

#define Util_Min(a,b) ((a)<(b) ? (a):(b))
#define Util_Max(a,b) ((a)>(b) ? (a):(b))

#define MULTIPLIER (4096)

#define TIMER_ZERO     0
#define TIMER_RUNNING  1
#define TIMER_FINISHED 2

typedef struct {
  uint16_t CurrentTimerVal;
  uint16_t TimeoutVal;
} Util_Timer;

typedef struct {
  uint16_t RateUp;      // units per tick
  uint16_t RateDown;    // units per tick, note it must be positive
  int32_t  CurrentVal;
} Util_Ramp;

typedef struct {
  int32_t State;
  uint16_t Samples;
} Util_Filter;

typedef struct {
  bool State;
  bool Toggled;
} Util_SRLatch;


extern int32_t Util_SetRampState(Util_Ramp* ramp, int32_t Input);
extern bool Util_SetSRLatchState(Util_SRLatch *latch, bool Set, bool Reset);
extern int32_t Util_Interpolate(int32_t x, const int16_t Xaxis[], const int16_t Yaxis[], uint32_t maxIndex);

extern int32_t Util_Map(int32_t x, int32_t x_min, int32_t x_max, int32_t y_min, int32_t y_max);
extern bool Util_SetTimerState(Util_Timer* timer, bool Start, bool Reset);
extern uint8_t Util_GetTimerState(const Util_Timer* timer);

extern int32_t Util_SetRampState(Util_Ramp* ramp, int32_t Input);
extern int32_t Util_FilterState(Util_Filter* filter, int32_t Input);
extern void Util_SetFilterState(Util_Filter* filter, int32_t Input);

extern int32_t Util_Limit(int32_t Input, int32_t Min, int32_t Max);
#endif // __UTIL_H