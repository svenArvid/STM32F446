/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __INPUT_CAPTURE_H
#define __INPUT_CAPTURE_H

#include "ProjectDefs.h"

extern TIM_HandleTypeDef        Timer2Handle;
extern TIM_HandleTypeDef        Timer5Handle;


extern void InputCapture_Init(void);
extern void InputCapture_20ms(void);

extern uint32_t InputCapture_ReadCCRx(TIM_TypeDef *TimInstance, uint32_t Channel);
#endif // __INPUT_CAPTURE_H
