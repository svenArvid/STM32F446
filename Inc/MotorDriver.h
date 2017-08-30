/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MOTOR_DRIVER_H
#define __MOTOR_DRIVER_H

#include "Pwm.h"

enum MotorDriverState
{
  MOTOR_STOP = 0,
  MOTOR_RUN_FORWARD,
  MOTOR_RUN_BACKWARD,
  MOTOR_ERROR
};


typedef struct {
  TIM_TypeDef * PwmTimer;
  uint16_t Pwm1;
  uint16_t Pwm2;
  GPIO_TypeDef * GpioPort;
  uint32_t EnablePin;
  enum MotorDriverState State;
} MotorDriver;


extern void MotorDriver_Init(void);
extern void MotorDriver_20ms();

#endif  // __MOTOR_DRIVER_H