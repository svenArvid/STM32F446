/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TICTOC_H
#define __TICTOC_H

#include "stm32f4xx_hal.h"
#include "InputCapture.h"

// Perform all TicToc measurements inside #ifdef TIC_TOC so that they are not done when undefining TIC_TOC
#define TIC_TOC 

#ifdef TIC_TOC
extern volatile uint32_t TicToc_Tim13IrqStart;
extern volatile uint32_t TicToc_Tim13IrqEnd;


//extern void TicToc_Init(void);
extern void TicToc_20ms(void);

#endif


#endif // __TIC_TOC_H