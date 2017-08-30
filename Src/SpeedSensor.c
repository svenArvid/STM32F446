/**
******************************************************************************
* @file    /Src/SpeedSensor.c
* @author  Joakim Carlsson
* @version V1.0
* @date    19-Aug-2017
* @brief   Use timer setup in InputCapture.c to measure frequency of speed sensors and compute speed of shafts. Direction of shaft attainable if 2 sensors are connected on a shaft.
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
//#include "ErrorHandler.h"
//#include "ProjectDefs.h"
#include "Uart.h"
#include "InputCapture.h"
#include "SpeedSensor.h"

ShaftSpeedSensor SensorIG53A;     
ShaftSpeedSensor SensorIG53B;

//-----------------------------------------------------------
void SpeedSensor_Init()  
{  
	(void)memset(&SensorIG53A, 0, sizeof(ShaftSpeedSensor));
  SensorIG53A.TimInstance = TIM2;
  SensorIG53A.Channel = TIM_CHANNEL_1;
  SensorIG53A.Tim_SR_CCxIF = TIM_SR_CC1IF;

  (void)memset(&SensorIG53B, 0, sizeof(ShaftSpeedSensor));
  SensorIG53B.TimInstance = TIM2;
  SensorIG53B.Channel = TIM_CHANNEL_2;
  SensorIG53B.Tim_SR_CCxIF = TIM_SR_CC2IF;
}


// Takes a pointer to ShaftSpeedSensor as parameter, and computes period time, i.e. time between positive edges.
// It uses a 32 bit timer.
static void UpdatePeriod(ShaftSpeedSensor *Snsr)
{
  uint32_t NewTrigTime;

  if (Snsr->TimInstance->SR & Snsr->Tim_SR_CCxIF)	 // If edge has been detected
  {
    NewTrigTime = InputCapture_ReadCCRx(Snsr->TimInstance, Snsr->Channel);  // Timer value when positive edge was detected. CCxIF flag is cleared when reading CCRx register 
                                     
    if (Snsr->TimeOut < FREQ_TIMEOUT)
    {
      Snsr->Period = NewTrigTime - Snsr->LastTrigTime;
    }
    else
    {
      Snsr->Period = 0;
    }
    Snsr->LastTrigTime = NewTrigTime;   //Save for next time
    Snsr->TimeOut = 0;
  }
	else
  {
    if (++Snsr->TimeOut >= FREQ_TIMEOUT)
    {
	    Snsr->TimeOut = FREQ_TIMEOUT;
	    Snsr->Period = 0;
    }
  }
}


void SpeedSensor_1ms()
{
 UpdatePeriod(&SensorIG53A);
 UpdatePeriod(&SensorIG53B);
}

// Compute Rpm based on Pulse period for a speed sensor.
// Note that Period = 0 has special meaning (No pulses have been captured for a while) -> Rpm shall be 0
static int ComputeShaftRpm(const ShaftSpeedSensor *Snsr) 
{
	uint32_t MotorRpm;  // Speed before gearbox
  uint32_t ShaftRpm;  // Speed after gearbox
  uint32_t Period = Snsr->Period;

  if (MIN_PERIOD < Period && Period < MAX_PERIOD)
	{
    MotorRpm = TIMER_CLOCK_FREQ * 60 / Period / PULSES_PER_REV;
    ShaftRpm = MotorRpm * GEAR_RATIO;	
	}
	else 
	{
    ShaftRpm = 0;
	}
  return ShaftRpm;
}


Util_Filter Filter_PhaseLag = { 75, 10 };    // Very fast LP-filter which removes the speed sensor noise but the lag is very low. Initiate to 45 degrees (in the middle)

// Computes phase lag between 2 shaft speed sensors and from that determines rotation direction
static enum RotationDirection CheckShaftRotationDirection(const ShaftSpeedSensor *Snsr1, const ShaftSpeedSensor *Snsr2)
{
  uint32_t Snsr1_TimeStampOfPrevPulse;
  uint32_t PhaseLagRaw, PhaseLagFild;
  uint32_t DegreesPerCycle;

  if (Snsr1->SensorFaultTimeOut || Snsr2->SensorFaultTimeOut) 
  {
    return SensorError;
  }
  else if (Snsr1->UnreliableSignal || Snsr2->UnreliableSignal)
  {
    return UnreliableSignal;
  }
  else if (0 == Snsr1->Period || 0 == Snsr2->Period) // Note: Thus ok to divide with Snsr1->Period (below) since it is not 0.
  {
    return ZeroSpeed;
  }

  else                        //  Timing of pulses:    |--- P1 ----- P2 ---------- P1' ------ (P2') ---|                   
  {
    Snsr1_TimeStampOfPrevPulse = Snsr1->LastTrigTime - Snsr1->Period;   // Snsr1->LastValTimer is P1' and Snsr1_TimeStampOfPrevPulse is P1.
      
    // Compute PhaseLag i.e. how many degrees P2 comes in after P1
    DegreesPerCycle = 360UL / PULSES_PER_REV;
    PhaseLagRaw = (DegreesPerCycle * (Snsr2->LastTrigTime - Snsr1_TimeStampOfPrevPulse) / Snsr1->Period );
    
    PhaseLagRaw %= DegreesPerCycle;    // If Snsr2->LastValTimer is P2', i.e. "newer" than P1', PhaseLag > 120 degrees, thus use modulo. Ex: PhaseLag is 155: 155 % 120 is 35

    // Apply Low-pass filter on phase lag, since it can be a bit jumpy  
    PhaseLagFild = Util_FilterState(&Filter_PhaseLag, PhaseLagRaw);


    if (15 <= PhaseLagFild && PhaseLagFild <= 45) {    // Distance between sensors is 90 degrees, 120 - 90 = 30
      return ClockWise;
    }
    else if (75 <= PhaseLagFild && PhaseLagFild <= 105) { // Phase lag is 120-30 = 90 degrees, i.e. rotation in other direction. 
      return CounterClockWise;                                        
    }
    else {
      return Undefined;
    }
  }
}

// Used for clutch unit to determine sign of output shaft speed signal, 
// The sign of speed is considered negative if: Current Rotation Direction is counter clockwise OR if it is Undefined but it was previously Counter clock wise.
// Else it is considered positive. Note: this implies that if SensorError OR UnreliableSignal, the speed is considered  positive.
static int SetOutputShaftSpeedSign(enum RotationDirection CurrRotDir)
{
  static enum RotationDirection RotDirLastTick = ZeroSpeed;
  static enum RotationDirection PreviousRotDir = ZeroSpeed;
  bool CurrRotDirNegative;
  bool PreviousRotDirNegative;

  int SignOfOutputShaftSpeed = 1;
  
  if (RotDirLastTick != CurrRotDir) {
    PreviousRotDir = RotDirLastTick;
  }
  
  // Compare rotation direction with engine's rotation direction.
  CurrRotDirNegative = CurrRotDir == CounterClockWise;

  PreviousRotDirNegative = PreviousRotDir == CounterClockWise;

  if (CurrRotDirNegative || (CurrRotDir == Undefined && PreviousRotDirNegative)) {
    SignOfOutputShaftSpeed = -1;
  }
  else {
    SignOfOutputShaftSpeed = 1;
  }

  RotDirLastTick = CurrRotDir;

  return SignOfOutputShaftSpeed;
}

// Do Speed sensor fault checks
/*
static void OutputShaftFaultHandling(sint SensorDi16Rpm, sint SensorDi13Rpm)
{
  bool StartCond, ResetCond;
  bool SensorA_NoSignal;
  bool SensorB_NoSignal;
  bool BothSpeedSensorsDead;
  ulong CurrentTime;
  ulong SecondLastValTimer;
      
  
  BothSpeedSensorsDead = FALSE;   //  && SensorDi16Rpm == 0 && SensorDi13Rpm == 0;
  
  // Condition SensorA_NoSignal is a flag that indicates that SensorA does not get pulses when B does. It is TRUE If one of the following conditions is true, else it is FALSE.
  // 1) Sensor A has received a pulse within the last X seconds BUT Sensor B has received at least 2 pulses after the last Sensor A pulse. 
  //    Thus the sensor A signal just dissapeared and calculation in Freq_CheckShaftRotationDirection cannot be done.
  // 2) Sensor A has not received a signal the last X seconds, BUT Sensor B has received pulses within the last X seconds. 
  
  //DisableInterrupts   // Doing some time critical stuff here, using variables that are updated in 1ms loop, i.e. in Interrupt

  CurrentTime = Freq_GetCurrentTime(); //Read actual value of timer
  
  SecondLastValTimer = SensorDi13.LastValTimer - SensorDi13.Period; // Timestamp of second last pulse for neighbour sensor
  SensorA_NoSignal = (SensorDi16Rpm > 0 && CurrentTime - SecondLastValTimer < CurrentTime - SensorDi16.LastValTimer) || (SensorDi16Rpm == 0 && SensorDi13Rpm > 0);
  
  SecondLastValTimer = SensorDi16.LastValTimer - SensorDi16.Period; // Timestamp of second last pulse for neighbour sensor
  SensorB_NoSignal = (SensorDi13Rpm > 0 && CurrentTime - SecondLastValTimer < CurrentTime - SensorDi13.LastValTimer) || (SensorDi13Rpm == 0 && SensorDi16Rpm > 0);
  
  //EnableInterrupts
  
  // Check if output sensors are disconnected. 
  // An output shaft sensor is considered unreliable if any of the following conditions are TRUE: 
  // 1) Sensor does not receive pulses, when its neighbour sensor does (condition SensorA_NoSignal)
  // 2) Clutch pressure is above a limit but none of the speed sensors are receiving pulses (condition BothSpeedSensorsDead)
  // Boolean SensorFaultTimeOut is set to TRUE when sensor has been considered unreliable a certain time. It is cleared when pulses are again sensed (Rpm > 0).

  SensorDi16.UnreliableSignal = SensorA_NoSignal || BothSpeedSensorsDead;

  StartCond = SensorDi16.UnreliableSignal && Db_GetUchar(dbActualClutchMode) > ClutchState_Slipping;  // Timer cannot be started in Disengaged state.
  ResetCond = SensorDi16Rpm > 0;
  SensorDi16.SensorFaultTimeOut = Util_SetTimerState(&SensorDi16.Timer_SensorDisconnected, StartCond, ResetCond);

  // ---- The same logic applies to SensorDi13. ----
  SensorDi13.UnreliableSignal = SensorB_NoSignal || BothSpeedSensorsDead;

  StartCond = SensorDi13.UnreliableSignal && Db_GetUchar(dbActualClutchMode) > ClutchState_Slipping;
  ResetCond = SensorDi13Rpm > 0;
  SensorDi13.SensorFaultTimeOut = Util_SetTimerState(&SensorDi13.Timer_SensorDisconnected, StartCond, ResetCond);
}
*/

// ---- Engine Speed sensor fault handling ----
/*
static void Freq_InputShaftFaultHandling(sint SensorDi14Rpm)
{
  bool StartCond, ResetCond;

  SensorDi14.UnreliableSignal = SensorDi14Rpm == 0 && Db_GetUchar(dbActualClutchMode) > ClutchState_Slipping; 

  StartCond = SensorDi14.UnreliableSignal;
  ResetCond = SensorDi14Rpm > 0;
  SensorDi14.SensorFaultTimeOut = Util_SetTimerState(&SensorDi14.Timer_SensorDisconnected, StartCond, ResetCond);
}
*/

Util_Filter FilterRpm_SensorIG53A = { 0, 10 };
Util_Filter FilterRpm_SensorIG53B = { 0, 10 };

void SpeedSensor_4ms(void)
{
  int32_t SensorIG53A_Rpm = ComputeShaftRpm(&SensorIG53A);
  int32_t SensorIG53B_Rpm = ComputeShaftRpm(&SensorIG53B);
 
  Util_FilterState(&FilterRpm_SensorIG53A, SensorIG53A_Rpm);
  Util_FilterState(&FilterRpm_SensorIG53B, SensorIG53B_Rpm);
}

void SpeedSensor_20ms(void)
{
  int32_t ShaftRotationDir = CheckShaftRotationDirection(&SensorIG53A, &SensorIG53B);
  int32_t SignOfShaftSpeed = SetOutputShaftSpeedSign(ShaftRotationDir);
  int32_t StrLength;
  
  StrLength = sprintf(USART3_TxBuff, "%ld,%ld,%ld, %ld\n", 
    Util_GetFilterState(&FilterRpm_SensorIG53A), Util_GetFilterState(&FilterRpm_SensorIG53B), ShaftRotationDir, Util_GetFilterState(&Filter_PhaseLag));

  // Print to Terminal
  HAL_UART_DMAStop(&USART3Handle);
  HAL_UART_Transmit_DMA(&USART3Handle, USART3_TxBuff, StrLength);
}

/*
// Compute Rpm for clutch node in 10 ms loop, i.e. same as controller tick time.
void Freq_10ms(void)
{
  sint SensorDi16Rpm;
  sint SensorDi13Rpm;
  sint SensorDi14Rpm;
  sint SignOfOutputShaftSpeed = 1;

 
  SensorDi16Rpm = Freq_ComputeShaftRpm(&SensorDi16);
  SensorDi13Rpm = Freq_ComputeShaftRpm(&SensorDi13);

  Freq_OutputShaftFaultHandling(SensorDi16Rpm, SensorDi13Rpm);

  // Check shaft rotation direction
  Db_SetUchar(dbOutputShaftRotationDir, (uchar)Freq_CheckShaftRotationDirection(&SensorDi16, &SensorDi13));
	SignOfOutputShaftSpeed = Freq_SetOutputShaftSpeedSign(Db_GetUchar(dbOutputShaftRotationDir));

  // Output shaft Speed sensor fault handling:
  // Sensor Di16 is primary sensor and is used if it is ok, else sensor Di13 is used as backup, if FaultTimeOut for both sensors are TRUE speed is set to 0. 
  // Else, dbClutchOutputShaftRpm is NOT updated, i.e. speed signal is frozen. 
  // Thus the strategy when a fault occurs is:
  // 1. Switch to the other sensor
  // 2. If both sensors disappear, freeze the last ok speed value. 
  // 3. When no signal has been received for a calibratable time (FaultTimeOut is set) set speed signal to 0.
    
  if (!SensorDi16.SensorFaultTimeOut && !SensorDi16.UnreliableSignal) {
    Db_SetInt(dbClutchOutputShaftRpm, SignOfOutputShaftSpeed * SensorDi16Rpm);
  }
  else if (!SensorDi13.SensorFaultTimeOut && !SensorDi13.UnreliableSignal) {
    Db_SetInt(dbClutchOutputShaftRpm, SignOfOutputShaftSpeed * SensorDi13Rpm);
  }
  else if (SensorDi16.SensorFaultTimeOut && SensorDi13.SensorFaultTimeOut) {    // Set speed to 0
    Db_SetInt(dbClutchOutputShaftRpm, 0);
  }
  else {                                          // Freeze speed
    ;
  }

  // Engine speed, use dbEngActSpeed if SensorDi14 is unreliable, and if that one is also faulty set to IdleRpm
  SensorDi14Rpm = Freq_ComputeShaftRpm(&SensorDi14);
  Freq_InputShaftFaultHandling(SensorDi14Rpm);

  if (!SensorDi14.UnreliableSignal && !SensorDi14.SensorFaultTimeOut) {
    Db_SetInt(dbClutchInputShaftRpm, SensorDi14Rpm);
  }
  else if(Db_GetInt(dbEngActSpeed) > 0){                                     
    Db_SetInt(dbClutchInputShaftRpm, Util_Map(Db_GetInt(dbEngActSpeed), 0, 1000, 0, Db_GetInt(dbBatEngineMaxRpm) ) );  // Map 0 -> 0 and 1000 -> Max Rpm 
  }
  else {
    Db_SetInt(dbClutchInputShaftRpm, Db_GetInt(dbBatEngineIdleRpm));
  }
  
    
  // Speed sensor values used for display in Gui
  Db_SetInt(dbClutchNodeDi14Rpm, SensorDi14Rpm);                          // Engine speed
  Db_SetInt(dbClutchNodeDi16Rpm, SensorDi16Rpm * SignOfOutputShaftSpeed); // Output shaft speed (primary sensor)
  Db_SetInt(dbClutchNodeDi13Rpm, SensorDi13Rpm * SignOfOutputShaftSpeed); // Output shaft speed (secondary sensor)


}
*/
/*
void Freq_1s(void)
{
  if (E2p_UnitIdentifier == unitCLUTCH)
  {
    SensorDi16.Timer_SensorDisconnected.TimeoutVal = 50 * Db_GetUchar(dbSpeedSensorFaultTime);    // Multiply by 50 since timer runs in 10 ms loop and parameter resolution is 500 ms
    SensorDi13.Timer_SensorDisconnected.TimeoutVal = 50 * Db_GetUchar(dbSpeedSensorFaultTime);
    SensorDi14.Timer_SensorDisconnected.TimeoutVal = 50 * Db_GetUchar(dbSpeedSensorFaultTime);
  }
}
*/
//************EOF