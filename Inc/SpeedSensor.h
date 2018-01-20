/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef SPEED_SENSOR_H
#define SPEED_SENSOR_H

#include "Util.h"
#include "InputCapture.h"

#define PULSES_PER_REV    3
#define GEAR_RATIO        100 / 5273    // ratio is 52.73, thus multiply with 100 and THEN divide by 5273, hence no parenthesises

typedef struct 
{
  InputCapture_t Ic;
  Util_Timer Timer_SensorDisconnected;
  bool SensorFaultTimeOut;
  bool UnreliableSignal;
} ShaftSpeedSensor;

enum RotationDirection
{
  ZeroSpeed = 0,
  ClockWise,
  CounterClockWise,
  Undefined,
  SensorError,
  UnreliableSignal
};

extern ShaftSpeedSensor SensorIG53A;
extern ShaftSpeedSensor SensorIG53B;

extern int16_t SensorIG53A_Rpm;
extern int16_t SensorIG53B_Rpm;
extern int16_t SensorIG53A_RpmFild;
extern int16_t SensorIG53B_RpmFild;
extern int16_t SensorM5_Rpm;
extern int16_t SensorM5_RpmFild;


extern void SpeedSensor_Init(void);
extern void SpeedSensor_1ms(void);
extern void SpeedSensor_4ms(void);
extern void SpeedSensor_20ms(void);

#endif  // SPEED_SENSOR_H