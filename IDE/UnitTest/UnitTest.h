/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UNIT_TEST_H
#define __UNIT_TEST_H

#include <stdio.h>
#include <stdint.h>

extern FILE* fp;

extern void UnitTest_CrcTableGenerator(void);
extern void UnitTest_CrcCalcCrc8(void);

extern void UnitTest_Util_Interpolate(void);
extern void UnitTest_Util_SRLatch(void);
extern void UnitTest_Util_FilterState(void);
extern void UnitTest_Util_Map(void);
#endif // __UNIT_TEST_H