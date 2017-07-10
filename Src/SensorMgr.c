/**
******************************************************************************
* @file    /Src/SensorMgr.c
* @author  Joakim Carlsson
* @version V1.0
* @date    31-Dec-2016
* @brief   Handles Temperature sensors cconnected to ADC inputs. Maybe add more sensors later.
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "Util.h"
#include "SensorMgr.h"
#include "Adc.h"


// Temeratures and computed ADC values (14 bits) for these temeratures for a NTC_3950 sensor. The constant resistor is 10K [Ohm]
// Shall be possible to add more Sensor tables here to support several Types. 
const int16_t NTC3950_TempArr[] = { -400, -350, -300, -250, -200, -150, -100, -50, 0, 50, 100, 150, 200, 250, 300, 350, 400, 450,
500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000, 1050, 1100, 1150, 1200, 1250 };

const int16_t NTC3950_ADCArr[] = { 15880, 15700, 15468, 15174, 14809, 14364, 13835, 13202, 12483, 11703, 10865, 9987, 9093, 8191, 7321,
6495, 5725, 5024, 4390, 3818, 3314, 2874, 2492, 2159, 1872, 1623, 1410, 1226, 1067, 933, 817, 716, 629, 554 };

TempSensor RoomTempSnsr = { .Temperature = 0,.Status = NO_FAULT,.Type = NTC_3950 };


int16_t SensorMgr_SetTemperature(TempSensor *Snsr, uint16_t ADC_Val)
{
  const int16_t *AdcArr;
  const int16_t *TempArr;
  int32_t ShortToGndLim;
  int32_t ShortToVddLim;
  uint32_t MaxIndex;

  switch (Snsr->Type)
  {
  case NTC_3950:
    AdcArr = NTC3950_ADCArr;
    TempArr = NTC3950_TempArr;
    ShortToGndLim = NTC_3950_SHORT_TO_GND_LIMIT;
    ShortToVddLim = NTC_3950_SHORT_TO_VDD_LIMIT;
    MaxIndex = sizeof(NTC3950_ADCArr) / sizeof(NTC3950_ADCArr[0]) - 1;
    break;
  
  default:
    // Shall call ErrorHandler here
    break;
  }

  // Check for electric faults. If a fault is present the Temperature value is not updated, i.e. last "good" value is kept.
  if (ADC_Val < ShortToGndLim) {
    Snsr->Status = SHORT_TO_GND;
  }
  else if (ADC_Val > ShortToVddLim) {
    Snsr->Status = SHORT_TO_VDD;
  }
  else {  
    Snsr->Status = NO_FAULT;
    Snsr->Temperature = Util_Interpolate(ADC_Val, AdcArr, TempArr, MaxIndex);
  }

  return Snsr->Temperature;
}


void SensorMgr_20ms(void)
{
  //uint16_t ADC_Val = Adc_Read();
  RoomTempSnsr.ADCVal = Adc_Read(0);
  SensorMgr_SetTemperature(&RoomTempSnsr, RoomTempSnsr.ADCVal);
}