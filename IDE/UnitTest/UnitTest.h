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

void UnitTest_CrcTableGenerator(void);
void UnitTest_CrcCalcCrc8(void);
void UnitTest_Crc_CalcCrc16(void);

void UnitTest_Util_Interpolate(void);
void UnitTest_Util_Interpolate2D(void);
void UnitTest_Util_SRLatch(void);
void UnitTest_Util_FilterState(void);
void UnitTest_Util_Map(void);

void UnitTest_RadioReceive(void);
void UnitTest_FlashE2p(void);

#endif // __UNIT_TEST_H