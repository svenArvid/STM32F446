/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __INPUT_CAPTURE_H
#define __INPUT_CAPTURE_H

#include "stm32f4xx_hal.h"

extern TIM_HandleTypeDef        Timer2Handle;


extern void InputCapture_Init(void);
extern void InputCapture_4ms(void);

#endif // __INPUT_CAPTURE_H
