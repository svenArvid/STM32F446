#ifndef __SENSOR_MGR_H
#define __SENSOR_MGR_H

#include "ProjectDefs.h"

// Sensor status values
#define NO_FAULT     0
#define SHORT_TO_GND 1
#define SHORT_TO_VDD 2

// Temperature Sensor Types
#define NTC_3950     1

// Electric faults threshholds
#define NTC_3950_SHORT_TO_GND_LIMIT   500 
#define NTC_3950_SHORT_TO_VDD_LIMIT 16000

typedef struct {
  uint16_t ADCVal;
  int16_t Temperature;      
  uint8_t Status;         
  uint8_t Type;
} TempSensor;


extern TempSensor RoomTempSnsr;


extern void SensorMgr_20ms(void);

#endif // __SENSOR_MGR_H