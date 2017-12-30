/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef SPEED_SENSOR_H
#define SPEED_SENSOR_H

#include "Util.h"


#define FREQ_TIMEOUT 1000
#define MIN_PERIOD   10000     // Timer clock is 10MHz and we check the capture register every 1ms, so not allowed to be faster than 1 KHz
#define MAX_PERIOD   10000000  // 1 sec

#define TIMER_CLOCK_FREQ  10000000
#define PULSES_PER_REV    3
#define GEAR_RATIO        100 / 5273    // ratio is 52.73, thus multiply with 100 and THEN divide by 5273, hence no parenthesises

typedef struct {		
  volatile TIM_TypeDef  *TimInstance;     // Timer Register base address 
  volatile uint16_t Channel;
  uint16_t Tim_SR_CCxIF;                  // Timer Capture/Compare interrupt Flag 
  
  volatile uint32_t Period;
  volatile uint32_t LastTrigTime;

  uint16_t TimeOut;
  
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