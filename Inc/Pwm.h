
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PWM_H
#define __PWM_H

#include "ProjectDefs.h"

#define  PWM_MAX_DUTY       (uint32_t)(1000)             


extern void Pwm_Init(void);
extern void Pwm_20ms(void);
extern void Pwm_500ms(void);
extern void Pwm_SetDuty(TIM_TypeDef *TIMx_Pwm, uint32_t Channel, uint32_t Duty);
extern void Pwm_SetPeriod(TIM_HandleTypeDef *htim, uint32_t Period);

#endif // __PWM_H
