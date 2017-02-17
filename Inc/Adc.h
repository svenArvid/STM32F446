/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ADC_H
#define __ADC_H

#include "stm32f4xx_hal.h"


#define ADC_ARR_SIZE 8

extern void Adc_Init(void);
extern void Adc_500ms(void);
extern uint16_t Adc_Read(uint32_t index);

#endif // __ADC_H
