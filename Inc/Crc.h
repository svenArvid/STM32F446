#ifndef __CRC_H
#define __CRC_H

#include "ProjectDefs.h"


uint8_t Crc_CalcCrc8(const uint8_t *data, const uint32_t size);
uint16_t Crc_CalcCrc16(const uint8_t *data, const uint32_t size);

#endif // __CRC_H