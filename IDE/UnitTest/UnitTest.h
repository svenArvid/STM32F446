/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UNIT_TEST_H
#define __UNIT_TEST_H

#include <stdio.h>
#include <stdint.h>
#include "ProjectDefs.h"


typedef struct {
  int16_t Temperature;
  uint8_t Humidity;
  uint8_t DeviceID;
  bool    Battery;
  uint8_t Channel;
  uint8_t SerialNum;
  bool    PendingRequest;
} TSS320_MsgStruct;

extern FILE* fp;

extern void UnitTest_CrcTableGenerator(void);
extern void UnitTest_CrcCalcCrc8(void);

extern void UnitTest_Util_Interpolate(void);
extern void UnitTest_Util_SRLatch(void);
extern void UnitTest_Util_FilterState(void);
extern void UnitTest_Util_Map(void);

extern void UnitTest_RadioReceive(void);
extern void UnitTest_FlashE2p(void);

#endif // __UNIT_TEST_H