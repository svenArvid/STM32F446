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

/*
typedef struct 
{
	float PressureFactor;               // [0, 1]
	int16_t EngineSetRpm;
	int16_t AuxTorqAtPropeller;      // [Nm]
	int16_t AuxTorqAtShaft;          // [Nm] 
	uint16_t PropValveCtrlSignal;      // [0, 1000] 
	bool ClutchOutValveCmd;
	bool ClutchInValveCmd;
} ExtU_PlantModel_t;

typedef struct 
{
	Util_Filter Filter_ClutchPressure;
	float EngineTorq;             // [Nm]
	float PropellerTorq;          // [Nm]
	float ClutchTransferTorq;     // [Nm]
	float ShaftRpm;
	float EngineActualRpm;
	int16_t EngineScaledPower;      // [0, 1000]
	uint16_t ClutchPressure;        // 0-40,000  [mBar]
	Util_SRLatch ClutchEngaged;
} ExtY_PlantModel_t;

#define RPM_TO_RAD  0.10472
#define RAD_TO_RPM  9.5493
#define GEAR_RATIO  7.78
#define MAX_RPM  1800
#define P_MAX    2500000
#define PROP_MAX_RPM  (MAX_RPM / GEAR_RATIO)
#define PROP_OMEGA_MAX   (PROP_MAX_RPM * RPM_TO_RAD)                      // angular speed [rad/s]
#define PROP_MAX_TORQ    (P_MAX / PROP_OMEGA_MAX)                         // [Nm]
#define PROP_TORQ_FAC  (PROP_MAX_TORQ / (PROP_OMEGA_MAX*PROP_OMEGA_MAX))

#define ENGINE_FRIC_TORQ  -1000    //  [Nm]

#define CLUTCH_MAX_PRESSURE       25000          // [mBar]
#define CLUTCH_MAX_SLIP_PRESSURE  7000           // [mBar]
#define CLUTCH_MAX_TORQ_TRANSFER  16000          // [Nm]
#define NUM_CLUTCH_PLATES         25
#define CLUTCH_PLATE_AREA_FAC     38.2179

#define USTREAM_INERTIA                     45.7088        // [kg*m^2]
#define DOWN_STREAM_INERTIA_INC_GEAR_RATIO  42.8828        // [kg*m^2]  värdet Fredrik Olin hade skrivit var mycket lägre men tyckte rpm steg så snabbt så ökade inertia

// 3512C Max Limit Curves:
int16_t rpmVec_3512C[] = { 0, 650, 700, 900, 1000, 1100, 1200, 1300, 1500, 1600, 1800 };
int16_t PowerVec_3512C[] = { 1, 275, 297, 403, 475, 670, 975, 1647, 1789, 1875, 1902 };               // [kW], 1 kW at 0 rpm to avoid division by zero
int16_t TorqVec_3512C[] = { 100, 4040, 4046, 4273, 4536, 5818, 7759, 12096, 11388, 11191, 10088 };    // [Nm]

int16_t PercentVec[] = { 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100 };
int16_t PropValvePressureVec[] = { 0, 100, 300, 700, 1200, 2100, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 11000, 12000, 13000, 14000, 15000, 16000, 17000 };

int16_t ClutchPlateDistPressureVec[] = { 3650, 3720, 3900, 4280, 4780, 5400, 6150, 7000, 10000, 15000 };
int16_t ClutchPlateDistFacVec[] = { 25000, 4586, 1062, 377, 161, 69, 24, 8, 2, 5 };  // Dont know about units, think it is ad hoc

ExtY_PlantModel_t PlantY;
ExtU_PlantModel_t PlantU;

void Plant_Init(void)
{
  PlantY.Filter_ClutchPressure.Samples = 8;  // 80 ms time constant
  PlantY.EngineActualRpm = 0.25;                    // Note: Non-zero for numeric reasons
}

void Plant_PropellerModel(ExtY_PlantModel_t *plantY)
{
  float propRpm = plantY->ShaftRpm / GEAR_RATIO;
  plantY->PropellerTorq = PROP_TORQ_FAC * propRpm * propRpm * Util_Sign(propRpm);
}


void Plant_EngineModel(ExtY_PlantModel_t *plantY)
{
	float EngineTorqRef = 0; // TODO: Engine controller
  float EnginePower;
  int16_t EngineTorqMaxLim = Util_Interpolate((int16_t)plantY->EngineActualRpm, rpmVec_3512C, TorqVec_3512C, ARR_SIZE(rpmVec_3512C));  
  int16_t EnginePowerMaxLim = Util_Interpolate((int16_t)plantY->EngineActualRpm, rpmVec_3512C, PowerVec_3512C, ARR_SIZE(rpmVec_3512C)); //  [kW]

  // Lower limit needed because controller may request negative torque, but the engine will in that case
  // only stop fuel injection, i.e. the lower limit is the engine internal friction.
  // Upper limit is acc. to the engine max limit curve
  plantY->EngineTorq = Util_Limit(EngineTorqRef, ENGINE_FRIC_TORQ, EngineTorqMaxLim);  

  EnginePower = (plantY->EngineActualRpm * RPM_TO_RAD) * (plantY->EngineTorq + UTIL_ABS(ENGINE_FRIC_TORQ));  // P = w * T  [W]
  plantY->EngineScaledPower = (int16_t)(EnginePower / EnginePowerMaxLim);                                    // Note: [W] divided by [kW] so range will be [0, 1000]
}

void Plant_ClutchModel(ExtU_PlantModel_t *plantU, ExtY_PlantModel_t *plantY)
{
  const float pressureSpanAboveSlip = (CLUTCH_MAX_PRESSURE - CLUTCH_MAX_SLIP_PRESSURE);
  float pressureAboveSlip;
  int16_t distFac;
  float torq, torqLockedClutch, torqSlippingClutch;
  float engAcc, shaftAcc;   // rad / s^2

  // Clutch Pressure control valves
  bool clutchLocked = Util_SetSRLatchState(&plantY->ClutchEngaged, plantU->ClutchInValveCmd, plantU->ClutchOutValveCmd);
  int16_t pressureFromPropValve = Util_Interpolate(plantU->PropValveCtrlSignal / 10, PercentVec, PropValvePressureVec, ARR_SIZE(PercentVec));
  int16_t pressure = clutchLocked ? CLUTCH_MAX_PRESSURE : pressureFromPropValve;

  pressure = (int16_t)(plantU->PressureFactor * pressure);
  plantY->ClutchPressure = Util_FilterState(&plantY->Filter_ClutchPressure, pressure);

  // From Torques, calculate angular acceleration for engine and shaft and integrate to get angular speed
  engAcc   = (plantY->EngineTorq - plantY->ClutchTransferTorq) / USTREAM_INERTIA;
  shaftAcc = (plantY->ClutchTransferTorq + plantU->AuxTorqAtShaft + (plantU->AuxTorqAtPropeller - plantY->PropellerTorq) / GEAR_RATIO) / DOWN_STREAM_INERTIA_INC_GEAR_RATIO;
  plantY->EngineActualRpm += engAcc * RAD_TO_RPM;
  plantY->ShaftRpm += shaftAcc * RAD_TO_RPM;


  // Torque transfer during plate mechanical contact
  pressureAboveSlip = UTIL_MAX(plantY->ClutchPressure - CLUTCH_MAX_SLIP_PRESSURE, 0);     // Remaining pressure to lock clutch
  torq = (float)CLUTCH_MAX_TORQ_TRANSFER * (pressureAboveSlip / pressureSpanAboveSlip);
  torqLockedClutch = Util_Limit(torq, -UTIL_ABS(plantY->EngineTorq), UTIL_ABS(plantY->EngineTorq));


  // Torque transfer during slipping clutch
  distFac = Util_Interpolate(plantY->ClutchPressure, ClutchPlateDistPressureVec, ClutchPlateDistFacVec, ARR_SIZE(ClutchPlateDistPressureVec));
  torq = (plantY->EngineActualRpm - plantY->ShaftRpm) * CLUTCH_PLATE_AREA_FAC / distFac;   // Torque per plate
  torqSlippingClutch = Util_Limit(torq * NUM_CLUTCH_PLATES, -2500, 2500);

  plantY->ClutchTransferTorq = torqLockedClutch + torqSlippingClutch;
}

*/