// ------ Unit test Flash E2p ------
#include <string.h>
#include "UnitTest.h"
#include "UnitTestDefs.h"
#include "FlashE2p.h"


uint32_t UnitTest_EmulatedSector[4096] = { 0 };


// Mockups for some HAL functions 
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *SectorError)
{
  (void)memset(UnitTest_EmulatedSector, 0xFF, sizeof(UnitTest_EmulatedSector));
  *SectorError = 0xFFFFFFFF;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint64_t Data)
{
  *(uint32_t*)Address = (uint32_t)Data;
  return HAL_OK;
}
// END OF Mockup functions


FLASH_EraseInitTypeDef DummyEraseInit;
uint32_t DummySectorError = 0;

FlashSector TestSector;

void UnitTest_FlashE2p(void)
{ 
  uint32_t FlashWord;
  int16_t Data;
  uint16_t E2pIndex;

  TestSector.BaseAddress = UnitTest_EmulatedSector;
  TestSector.EraseNeeded = FALSE;
  TestSector.InitEraseOffset = EEPROM_ERASE_OFFSET_AT_INIT;
  TestSector.NextWriteAddress = UnitTest_EmulatedSector;
  TestSector.PageSize = EEPROM_PAGE_SIZE;
  TestSector.SectorNum = FLASH_SECTOR_3;

  FlashE2p_EraseSector(&TestSector);    // Start with erasing dummy Eeprom

  printf("\nTest FlashE2p Init\n");
  printf("\nRun Init routine with Flash erased. Default parameters shall be written to Flash and Ram mirror\n");
  FlashE2p_InitSector(&TestSector);          // Run Init when Flash is erased
  printf("Ram mirror: %d, %d, %d, %d, %d\n", FlashE2p_ReadMirror(0), FlashE2p_ReadMirror(1), FlashE2p_ReadMirror(2), FlashE2p_ReadMirror(3), FlashE2p_ReadMirror(4));
  
  FlashWord = *(uint32_t*)UnitTest_EmulatedSector;
  Data = (int16_t)(FlashWord >> 16);              // Little endian 
  E2pIndex = (uint16_t)FlashWord;
  printf("Flash: index %d, Data %d \n", E2pIndex, Data);
  FlashWord = *(uint32_t*)(UnitTest_EmulatedSector + 4);
  Data = (int16_t)(FlashWord >> 16);              // Little endian 
  E2pIndex = (uint16_t)FlashWord;
  printf("Flash: index %d, Data %d \n", E2pIndex, Data);

  printf("\nRun Init routine with initialized Flash. Parameters shall be read from Flash and written to Ram mirror\nClear Ram mirror to simulate a reset.\n");
  for (int i = 0; i < E2P_NUM_PARAMETERS; i++) {
    FlashE2p_WriteMirror(i, 0);
    FlashE2p_WriteSynchBit(i, FALSE);
  }
  FlashE2p_InitSector(&TestSector);
  printf("Ram mirror: %d, %d, %d, %d, %d\n", FlashE2p_ReadMirror(0), FlashE2p_ReadMirror(1), FlashE2p_ReadMirror(2), FlashE2p_ReadMirror(3), FlashE2p_ReadMirror(4));

  FlashWord = *(uint32_t*)UnitTest_EmulatedSector;
  Data = (int16_t)(FlashWord >> 16);              // Little endian 
  E2pIndex = (uint16_t)FlashWord;
  printf("Flash: index %d, Data %d \n", E2pIndex, Data);
  FlashWord = *(uint32_t*)(UnitTest_EmulatedSector + 4);
  Data = (int16_t)(FlashWord >> 16);              // Little endian 
  E2pIndex = (uint16_t)FlashWord;
  printf("Flash: index %d, Data %d \n", E2pIndex, Data);
}