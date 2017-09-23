
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PROJECT_DEFS_H
#define __PROJECT_DEFS_H

#include <stdint.h>

#ifdef  UNIT_TEST
#include "UnitTestDefs.h"
#else
  #include "stm32f4xx_hal.h"
#endif

typedef uint8_t bool;


#define FALSE 0
#define TRUE  1


#endif // __PROJECT_DEFS_H