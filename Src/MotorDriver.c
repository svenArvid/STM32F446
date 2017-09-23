/**
******************************************************************************
* @file    /Src/MotorDriver.c
* @author  Joakim Carlsson
* @version V1.0
* @date    27-July-2017
* @brief   Device driver for motor H-driver SN754410.
*          For one motor two non-inverting driver inputs and an Enable signal that enables these 2 channels are used.
*          The two driver inputs are controlled with Pwm-signals and the Enable signal with a Digital output.
*          SN754410 can driver two motors this way:
*          Motor1 Pins: 1,2EN; 1A, 2A
*          Motor2 Pins: 3,4EN; 3A, 4A
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "ErrorHandler.h"
#include "ProjectDefs.h"
#include "Pwm.h"
#include "MotorDriver.h"
#include "Util.h"

MotorDriver Driver1;

void MotorDriver_Init(void)
{
  GPIO_InitTypeDef      GPIO_InitStruct;

  /* Enable Port F, (GPIO PF12 is used for Channel Enable signal) */
  __HAL_RCC_GPIOF_CLK_ENABLE();

  /* Configure Pin */
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; //  GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  Driver1.PwmTimer = TIM4;
  Driver1.Pwm1 = TIM_CHANNEL_3;
  Driver1.Pwm2 = TIM_CHANNEL_4;
  Driver1.GpioPort = GPIOF;
  Driver1.EnablePin = GPIO_PIN_12;
  Driver1.State = MOTOR_STOP;
}


static enum MotorDriverState UpdateState(MotorDriver *driver, int32_t controlSig)
{
  enum MotorDriverState DesiredState;
  enum MotorDriverState NewState = driver->State;  // Initiate NewState to current state

  // Maybe Check for Errors first
  if (controlSig > 0)
  {
    DesiredState = MOTOR_RUN_FORWARD;
  }
  else if (controlSig < 0)
  {
    DesiredState = MOTOR_RUN_BACKWARD;
  }
  else {
    DesiredState = MOTOR_STOP;
  }

  switch (driver->State)
  {
  case MOTOR_RUN_FORWARD:
    if (DesiredState != MOTOR_RUN_FORWARD)
    {
      NewState = MOTOR_STOP;
    }
    break;
  case MOTOR_RUN_BACKWARD:
    if (DesiredState != MOTOR_RUN_BACKWARD)
    {
      NewState = MOTOR_STOP;
    }
    break;
  case MOTOR_STOP:
    NewState = DesiredState;
    break;
  default:
  case MOTOR_ERROR:
    if (DesiredState != MOTOR_ERROR)
    {
      NewState = MOTOR_STOP;
    }
    break;
  }
  return NewState;
}

static void DoActions(MotorDriver *driver, int32_t controlSig)
{
  switch (driver->State)
  {
  case MOTOR_RUN_FORWARD:
    HAL_GPIO_WritePin(driver->GpioPort, driver->EnablePin, GPIO_PIN_SET);
    Pwm_SetDuty(driver->PwmTimer, driver->Pwm1, Util_Abs(controlSig));
    Pwm_SetDuty(driver->PwmTimer, driver->Pwm2, 0);
    break;
  case MOTOR_RUN_BACKWARD:
    HAL_GPIO_WritePin(driver->GpioPort, driver->EnablePin, GPIO_PIN_SET);
    Pwm_SetDuty(driver->PwmTimer, driver->Pwm1, 0);
    Pwm_SetDuty(driver->PwmTimer, driver->Pwm2, Util_Abs(controlSig));
    break;

  default:
  case MOTOR_STOP:
  case MOTOR_ERROR:
    HAL_GPIO_WritePin(driver->GpioPort, driver->EnablePin, GPIO_PIN_RESET);
    Pwm_SetDuty(driver->PwmTimer, driver->Pwm1, 0);
    Pwm_SetDuty(driver->PwmTimer, driver->Pwm2, 0);
    break;
  }
}

static void StateMachine(MotorDriver *driver, int32_t controlSig)
{
  driver->State = UpdateState(driver, controlSig);

  DoActions(driver, controlSig);
}

static Util_Ramp Ramp_Control = { 5, 5, 0 };
void MotorDriver_20ms()
{
  static int32_t Counter = 0;
  static int32_t Setpoint = 600;
  int32_t ControlSig;
 
  if (++Counter >= 3000) // 60 seconds
  {
    Counter = 0;
    Setpoint = -Setpoint;
  }
  ControlSig = Util_SetRampState(&Ramp_Control, Setpoint);
  StateMachine(&Driver1, ControlSig);
}
