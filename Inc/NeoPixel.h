/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __NEO_PIXEL_H
#define __NEO_PIXEL_H

#include "ProjectDefs.h"

#define NUM_LEDS  16
#define MOST_SIG_COLOR_BIT  23
#define NEO_PIXEL_DATA_PIN  GPIO_PIN_9

#define TIME_ZERO_HIGH  21   // 0.467 us
#define TIME_ONE_HIGH   36   // 0.8 us
#define PERIOD_TIME    224   // 224 to get 5 us (225/45MHz = 5 us)

#define BIT_B0                 ((uint32_t)0x0001U)    // Blue color least significant bit 
#define BIT_B1                 ((uint32_t)0x0002U)  
#define BIT_B2                 ((uint32_t)0x0004U)  
#define BIT_B3                 ((uint32_t)0x0008U)  
#define BIT_B4                 ((uint32_t)0x0010U)  
#define BIT_B5                 ((uint32_t)0x0020U)  
#define BIT_B6                 ((uint32_t)0x0040U)  
#define BIT_B7                 ((uint32_t)0x0080U)    // Blue color most significant bit
#define BIT_R0                 ((uint32_t)0x0100U)    // Red color least significant bit 
#define BIT_R1                 ((uint32_t)0x0200U)  
#define BIT_R2                 ((uint32_t)0x0400U)  
#define BIT_R3                 ((uint32_t)0x0800U)  
#define BIT_R4                 ((uint32_t)0x1000U)  
#define BIT_R5                 ((uint32_t)0x2000U)  
#define BIT_R6                 ((uint32_t)0x4000U)  
#define BIT_R7                 ((uint32_t)0x8000U)    // Red color most significant bit
#define BIT_G0                 ((uint32_t)0x010000U)  // Green color least significant bit 
#define BIT_G1                 ((uint32_t)0x020000U)  
#define BIT_G2                 ((uint32_t)0x040000U)  
#define BIT_G3                 ((uint32_t)0x080000U)  
#define BIT_G4                 ((uint32_t)0x100000U)  
#define BIT_G5                 ((uint32_t)0x200000U)  
#define BIT_G6                 ((uint32_t)0x400000U)  
#define BIT_G7                 ((uint32_t)0x800000U)  // Green color most significant bit

typedef struct {
  uint32_t ColorData[NUM_LEDS];
  int32_t PreComputedDuty;
  int32_t currLED;
  int32_t ColorBit;
  bool TxBusy;
} NeoPixel_TxData;

extern TIM_HandleTypeDef        Timer14Handle;


extern void NeoPixel_Init(void);
extern void NeoPixel_100ms(void);
extern void NeoPixel_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif // __NEO_PIXEL_H
