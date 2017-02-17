
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PWM_H
#define __PWM_H

#include "stm32f4xx_hal.h"

#define  PWM_MAX_DUTY       (uint32_t)(1000)             
#define  PULSE1_VALUE       (uint32_t)(PWM_MAX_DUTY)     /* Capture Compare 1 Value  */
#define  PULSE4_VALUE       (uint32_t)(PWM_MAX_DUTY*12/100) /* Capture Compare 4 Value  */


extern void Pwm_Init(void);
extern void Pwm_20ms(void);
extern void Pwm_500ms(void);
extern void Pwm_SetDuty(TIM_TypeDef *TIMx_Pwm, uint32_t Channel, uint32_t Duty);

#endif // __PWM_H
