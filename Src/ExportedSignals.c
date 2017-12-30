/**
******************************************************************************
* @file    /Src/ExportedSignals.c
* @author  Joakim Carlsson
* @version V1.0
* @date    19-Nov-2017
* @brief   Signals to be exported to PC over Modbus interface
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "ExportedSignals.h"
#include "NeoPixel.h"
#include "sensorMgr.h"
#include "Adc.h"
#include "SpeedSensor.h"

#define NUM_SIGNALS 8

uint16_t Signals[NUM_SIGNALS];

uint16_t ExportedSignals_Read(uint16_t indx)
{
  if (indx < NUM_SIGNALS)
  {
    return Signals[indx];
  }
  else {
    return 0;
  }
}

void ExportedSignals_Update(void)
{
  Signals[0] = RoomTempSnsr.Temperature;
  Signals[1] = RoomTempSnsr.ADCVal;
  Signals[2] = SensorIG53A_Rpm;
  Signals[3] = SensorIG53B_Rpm;
  Signals[4] = SensorIG53A_RpmFild;
  Signals[5] = SensorIG53B_RpmFild;
  Signals[6] = SensorM5_Rpm;
  Signals[7] = SensorM5_RpmFild;

}