/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FLASH_E2P_H
#define __FLASH_E2P_H

#include "ProjectDefs.h"

#ifdef E2P_TEST
#define E2P_DEFAULT(DEFAULT, TEST)  TEST
#else
#define E2P_DEFAULT(DEFAULT, TEST)  DEFAULT
#endif

typedef enum {
  E2P_ECR_PITCH_LEVER_AHEAD_A = 0,
  E2P_ECR_PITCH_LEVER_ZERO_A,		  
  E2P_ECR_PITCH_LEVER_ASTERN_A,	  
  E2P_ECR_RPM_LEVER_MAX_A,  		  
  E2P_ECR_RPM_LEVER_MIN_A,
  E2P_CLUTCH_SPRING_PRESSURE,
  E2P_CLUTCH_IN_PRE_PRESSURE,

  E2P_NUM_PARAMETERS            // Always last - let the toolchain count the parameters
} tE2Index;


// ----------------------------------------------------------------------------
// Default values array, checked during startup
// ----------------------------------------------------------------------------
typedef struct
{
  int16_t MinVal;
  int16_t MaxVal;
  int16_t DefaultVal;
}tE2pDefault;

extern const tE2pDefault Default[E2P_NUM_PARAMETERS];

// -----------------------------------------------------------------------------
typedef struct {
  uint32_t BaseAddress;
  uint32_t PageSize;
  uint32_t InitEraseOffset;
  uint32_t NextWriteAddress;
  uint8_t  SectorNum;
  bool EraseNeeded;
} FlashSector;

//------------------------------------------------------------------------------
/* EEPROM emulation start address in Flash. Sector 3, 16 Kbyte memory */
#define EEPROM_BASE_ADDRESS  ((uint32_t)0x0800C000)  

/* Sector 3 Page size = 16KByte */
#define EEPROM_PAGE_SIZE               (uint32_t)0x4000  
#define EEPROM_ERASE_OFFSET_AT_INIT   (uint32_t)14000      // Use this in FlashE2p_InitSector so that when this address has been reached, reinitalize the Eeprom sector
                                                           // to minimize risk that the sector needs to be erased in the loop, i.e. it is preferred to do it in Init

extern HAL_StatusTypeDef FlashE2p_EraseSector(FlashSector *Sector);
extern void FlashE2p_InitSector(FlashSector *Sector);
extern void FlashE2p_Init(void);
extern void FlashE2p_500ms(void);
extern void FlashE2p_PrintToTerminal(void);

extern bool FlashE2p_ReadSynchBit(uint16_t E2pIndex);
extern void FlashE2p_WriteSynchBit(uint16_t E2pIndex, bool BitVal);

extern int16_t FlashE2p_ReadMirror(tE2Index Index);
extern void FlashE2p_WriteMirror(tE2Index Index, int16_t Data);
extern int16_t FlashE2p_GetMaxVal(tE2Index Index);
extern int16_t FlashE2p_GetMinVal(tE2Index Index);
extern int16_t E2p_GetDefaultVal(tE2Index Index);

extern void FlashE2p_UpdateParameter(tE2Index Index, int16_t Data);

#endif  // __FLASH_E2P_H