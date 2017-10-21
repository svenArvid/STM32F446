/**
******************************************************************************
* @file    /Src/FlashE2p.c
* @author  Joakim Carlsson
* @version V1.0
* @date    17-September-2017
* @brief   Eeprom is emulated in Flash sector 3 (16 kb) to be used for saving parameters in non-volatile memory.  
           Parameters are put in Ram mirror at Init.
*
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "ErrorHandler.h"
#include "FlashE2p.h"
#include "Util.h"
#include "Uart.h"

const tE2pDefault E2pDefault[E2P_NUM_PARAMETERS] =
{
  // Min, Max,      Default
  { 0, 1000,    E2P_DEFAULT(800,        E2P_ECR_PITCH_LEVER_AHEAD_A)   }, 
  { 0, 1000,    E2P_DEFAULT(600,        E2P_ECR_PITCH_LEVER_ZERO_A)    }, 
  { 0, 1000,    E2P_DEFAULT(200,        E2P_ECR_PITCH_LEVER_ASTERN_A)  }, 
  { 0, 1000,    E2P_DEFAULT(700,        E2P_ECR_RPM_LEVER_MAX_A)       }, 
  { 0, 1000,    E2P_DEFAULT(150,        E2P_ECR_RPM_LEVER_MIN_A)       }, 
  { 0, 2500,    E2P_DEFAULT(350,        E2P_CLUTCH_SPRING_PRESSURE)    }, 
  { 0, 2500,    E2P_DEFAULT(700,        E2P_CLUTCH_IN_PRE_PRESSURE)    }, 
};

static int16_t E2pRamMirror[E2P_NUM_PARAMETERS];
static uint32_t FlashE2p_InSynch[1 + (E2P_NUM_PARAMETERS / 32)] = { 0 };  // Bit field that tells if Ram mirror is in synch with Flash Eeprom: 1 = Synched, 0 = Not Synched

FlashSector Sector3;

// -----------------------------------------------------------------------
// ------ Functions ------

// Erase Flash sector (input) used for emulated Eeprom
HAL_StatusTypeDef FlashE2p_EraseSector(FlashSector *pSector)
{
  HAL_StatusTypeDef  FlashStatus = HAL_OK;
  uint32_t SectorError = 0;
  FLASH_EraseInitTypeDef pEraseInit;

  pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
  pEraseInit.Sector = pSector->SectorNum;
  pEraseInit.NbSectors = 1;
  pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;  // Used when device voltage range is 2.7V to 3.6V, the operation will be done by word (32-bit)

  FlashStatus = HAL_FLASHEx_Erase(&pEraseInit, &SectorError);          // Maybe add handling of FlashStatus ?
  pSector->NextWriteAddress = pSector->BaseAddress;                    // Reset adress to sector start
  (void)memset(FlashE2p_InSynch, 0, sizeof(FlashE2p_InSynch));         // Clear bit array since all parameters in Flash need to be updated

  pSector->EraseNeeded = FALSE;

  return FlashStatus;
}

static HAL_StatusTypeDef FlashE2p_ProgramWord(FlashSector *pSector, uint16_t E2pIndex, uint16_t Data)
{
  HAL_StatusTypeDef FlashStatus = HAL_OK;

  uint32_t Word = 0;  // Word will be programmed into Eeprom, it contains both the index and the data.

  if (pSector->NextWriteAddress >= pSector->BaseAddress + pSector->PageSize) // Page full
  {
    pSector->EraseNeeded = TRUE;
    return HAL_ERROR;
  }

  if (E2pIndex < E2P_NUM_PARAMETERS)
  {
    Word = Data << 16;    // Little endian so put data in High bytes since Least sign. byte first i.e. E2pIndex will be placed at lowest address. 
    Word |= E2pIndex;

    FlashStatus = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, pSector->NextWriteAddress, Word);    // Maybe add handling of FlashStatus ?
    FlashE2p_WriteSynchBit(E2pIndex, TRUE);
    
    pSector->NextWriteAddress += 4;
  }

  return FlashStatus;
}

// Called in 500ms loop, checks if there are parameters that have been updated, i.e. Ram mirror is ahead of Eeprom. 
// If so, those parameters are updated in Eeprom (copy Ram mirror value to Eeprom -> In synch). 
static void FlashE2p_UpdateEeprom(FlashSector *pSector)
{
  uint32_t NumWords = sizeof(FlashE2p_InSynch) / sizeof(FlashE2p_InSynch[0]);
  uint32_t BaseInd, E2pIndex;

  for (int Word = 0; Word < NumWords; Word++)
  {
    if (FlashE2p_InSynch[Word] != 0xFFFFFFFF)
    {
      BaseInd = 32 * Word;
      for (int Bit = 0; Bit < 32; Bit++)
      {
        E2pIndex = BaseInd + Bit;
        if (!FlashE2p_ReadSynchBit(E2pIndex))
        {
          FlashE2p_ProgramWord(pSector, E2pIndex, FlashE2p_ReadMirror(E2pIndex));
        }
      }
    }
  }
}

void FlashE2p_InitSector(FlashSector *pSector)
{
  uint16_t EepromZeroIndex = (*(uint16_t*)pSector->BaseAddress);
  uint32_t FlashAddress = pSector->BaseAddress;
  uint32_t FlashWord = 0;
  uint16_t E2pIndex = 0; 
  int16_t  Data = 0;
  HAL_StatusTypeDef  FlashStatus = HAL_OK;
  int16_t DefaultVal = 0;

  pSector->NextWriteAddress = pSector->BaseAddress;

  if (EepromZeroIndex == 0) // Flash Eeprom pSector is Initialized
  {
    FlashAddress = pSector->BaseAddress + pSector->PageSize - 4;  // Point to last word on the page, search backwards until reach a valid word, (i.e. NOT 0xFFFFFFFF)
    while (FlashAddress > pSector->BaseAddress)
    {
      if ( *(uint32_t*)FlashAddress != 0xFFFFFFFF)
      {
        break;
      }
      else {
        FlashAddress -= 4;
      }
    }
    pSector->NextWriteAddress = FlashAddress + 4;  // Next Flash Eeprom address to write to (i.e. first that is 0xFFFFFFFF)

    while (FlashAddress >= pSector->BaseAddress)  // valid word reached, now read values from Flash and copy to Ram mirror. Stop when beginning of page is reached.
    {
      FlashWord = *(uint32_t*)FlashAddress;
      Data  = (int16_t)(FlashWord >> 16);              // Little endian 
      E2pIndex = (uint16_t)FlashWord;
      
      if (FlashE2p_ReadSynchBit(E2pIndex) == 0)
      {
        if (Util_InRange(Data, FlashE2p_GetMinVal(E2pIndex), FlashE2p_GetMaxVal(E2pIndex)))
        {
          FlashE2p_WriteMirror(E2pIndex, Data);                          // Copy Flash to Ram mirror if the value is in range ...
          FlashE2p_WriteSynchBit(E2pIndex, TRUE);                        // and set synch bit
        }
      }
      FlashAddress -= 4;
    }

    // Check if the Synch bit field still contains zeros (can happen if loaded new SW with new parameters OR if some parameter value was out of range and thus rejected)
    // If so write default parameters to Ram mirror AND to Flash Eeprom
    for (E2pIndex = 0; E2pIndex < E2P_NUM_PARAMETERS; E2pIndex++)
    {
      if (FlashE2p_ReadSynchBit(E2pIndex) == 0)
      {
        DefaultVal = E2p_GetDefaultVal(E2pIndex);
        FlashE2p_WriteMirror(E2pIndex, DefaultVal);
        FlashStatus = FlashE2p_ProgramWord(pSector, E2pIndex, DefaultVal);
      }
    } 
    
    // Now Ram mirror is properly initialized with data from Flash and (possibly default parameters)
    // Last step is to check if sector is so full that it is time to ERASE it and start over with a clean sector. Since this is time consuming we prefer 
    // to do it at Init rather than after startup. Thus EraseWhenReachOffset is less than page size to make it more likely the erase is done at Startup.
    if (pSector->NextWriteAddress >= pSector->BaseAddress + pSector->InitEraseOffset)
    {
      FlashE2p_EraseSector(pSector);

      for (E2pIndex = 0; E2pIndex < E2P_NUM_PARAMETERS; E2pIndex++)
      {
        FlashStatus = FlashE2p_ProgramWord(pSector, E2pIndex, FlashE2p_ReadMirror(E2pIndex));  // After erase, write Ram mirror to Flash
      }
    }

    UART_PRINTF("Copied Flash Eeprom to Ram mirror\r\n");
  }
  else  // Memory not initialized properly. Use default parameters and initalize the Eeprom sector.
  {
    // Check if pSector is erased, i.e. only contains ones. Otherwise it must be erased
    for (FlashAddress = pSector->BaseAddress; FlashAddress < pSector->BaseAddress + EEPROM_PAGE_SIZE; FlashAddress += 4)
    {
      if (*(uint32_t*)FlashAddress != 0xFFFFFFFF)
      {
        pSector->EraseNeeded = TRUE;
        break;
      }
    }
    
    if (pSector->EraseNeeded)
    {
      FlashStatus = FlashE2p_EraseSector(pSector);  // Maybe add handling of FlashStatus ?
    }

    for (E2pIndex = 0; E2pIndex < E2P_NUM_PARAMETERS; E2pIndex++)
    {
      DefaultVal = E2p_GetDefaultVal(E2pIndex);                             // For all parameters: 
      FlashE2p_WriteMirror(E2pIndex, DefaultVal);                           // Write default values to Ram mirror ...
      FlashStatus = FlashE2p_ProgramWord(pSector, E2pIndex, DefaultVal);  // and to the (erased) Flash Eeprom
    }
    UART_PRINTF("Default values written to Eeprom\r\n");
  }
}

void FlashE2p_Init(void)
{
  HAL_FLASH_Unlock();   // unlock Flash write protection 

  Sector3.BaseAddress = EEPROM_BASE_ADDRESS;
  Sector3.EraseNeeded = FALSE;
  Sector3.InitEraseOffset = EEPROM_ERASE_OFFSET_AT_INIT;
  Sector3.NextWriteAddress = EEPROM_BASE_ADDRESS;
  Sector3.PageSize  = EEPROM_PAGE_SIZE;
  Sector3.SectorNum = FLASH_SECTOR_3;

  FlashE2p_InitSector(&Sector3);
}

// Print the parameters to Terminal
void FlashE2p_PrintToTerminal(void)
{
  uint32_t indx;

  UART_PRINTF("Ram mirror:\r\n");
  
  for (indx = 0; indx < E2P_NUM_PARAMETERS; indx++)
  {
    UART_PRINTF("%d\r\n", FlashE2p_ReadMirror(indx));
  }
  
  UART_PRINTF("Flash:\r\n");
  indx = Sector3.BaseAddress;
  while (TRUE)
  {
    UART_PRINTF("%d, %d\r\n", (*(uint16_t*)indx), (*(int16_t*)(indx + 2)));
    if (*(uint16_t*)indx == 0xFFFF)
    {
      break;
    }
    indx += 4;
  }
}

void FlashE2p_500ms(void)
{
  FlashE2p_UpdateEeprom(&Sector3);
}

// ====================================================
bool FlashE2p_ReadSynchBit(uint16_t E2pIndex)
{
  bool BitVal = FALSE;

  if (E2pIndex < E2P_NUM_PARAMETERS)
  {
    BitVal = Util_BitRead(FlashE2p_InSynch[E2pIndex / 32], E2pIndex % 32);
  }
  return BitVal;
}

void FlashE2p_WriteSynchBit(uint16_t E2pIndex, bool BitVal)
{
  if (E2pIndex < E2P_NUM_PARAMETERS)
  {
    Util_BitWrite(FlashE2p_InSynch[E2pIndex / 32], E2pIndex % 32, BitVal);
  }
}

int16_t FlashE2p_ReadMirror(tE2Index Index)
{
  if (Index < E2P_NUM_PARAMETERS)
  {
    return E2pRamMirror[Index];
  }
  return 0;
}

void FlashE2p_WriteMirror(tE2Index Index, int16_t Data)
{
  if (Index < E2P_NUM_PARAMETERS)
  {
    E2pRamMirror[Index] = Data;
  }
}

int16_t FlashE2p_GetMaxVal(tE2Index Index)
{
  if (Index < E2P_NUM_PARAMETERS)
  {
    return E2pDefault[Index].MaxVal;
  }
  return 0;
}

int16_t FlashE2p_GetMinVal(tE2Index Index)
{
  if (Index < E2P_NUM_PARAMETERS)
  {
    return E2pDefault[Index].MinVal;
  }
  return 0;
}

int16_t E2p_GetDefaultVal(tE2Index Index)
{
  if (Index < E2P_NUM_PARAMETERS)
  {
    return E2pDefault[Index].DefaultVal;
  }
  return 0;
}

// This function is used by application to change a parameter. 
// Only do the update if the new value is different from what is already in Ram mirror, (minimize writes to Flash)
// The Flash is NOT updated in this function, but since Synch bit is reset FlashE2p knows that it will need to update Flash (in 100ms loop) 
void FlashE2p_UpdateParameter(tE2Index Index, int16_t Data)
{
  if (FlashE2p_ReadMirror(Index) != Data)
  {
    FlashE2p_WriteMirror(Index, Data);
    FlashE2p_WriteSynchBit(Index, FALSE);
  }
}