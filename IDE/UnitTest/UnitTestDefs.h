/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UNIT_TEST_DEFS_H
#define __UNIT_TEST_DEFS_H

#include <stdint.h>

// Put declarations here needed for Unit Test to avoid having to include files that includes HAL-files.
typedef enum
{
  HAL_OK = 0x00U,
  HAL_ERROR = 0x01U,
  HAL_BUSY = 0x02U,
  HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

typedef struct
{
  uint32_t TypeErase;   /*!< Mass erase or sector Erase.
                        This parameter can be a value of @ref FLASHEx_Type_Erase */

  uint32_t Banks;       /*!< Select banks to erase when Mass erase is enabled.
                        This parameter must be a value of @ref FLASHEx_Banks */

  uint32_t Sector;      /*!< Initial FLASH sector to erase when Mass erase is disabled
                        This parameter must be a value of @ref FLASHEx_Sectors */

  uint32_t NbSectors;   /*!< Number of sectors to be erased.
                        This parameter must be a value between 1 and (max number of sectors - value of Initial sector)*/

  uint32_t VoltageRange;/*!< The device voltage range which defines the erase parallelism
                        This parameter must be a value of @ref FLASHEx_Voltage_Range */

} FLASH_EraseInitTypeDef;

#define FLASH_SECTOR_3     ((uint32_t)3U) /*!< Sector Number 3   */
#define FLASH_VOLTAGE_RANGE_3        ((uint32_t)0x02U)  /*!< Device operating range: 2.7V to 3.6V                */
#define FLASH_TYPEPROGRAM_WORD        ((uint32_t)0x02U)  /*!< Program a word (32-bit) at a specified address        */
#define FLASH_TYPEERASE_SECTORS         ((uint32_t)0x00U)  /*!< Sectors erase only          */

extern uint32_t UnitTest_EmulatedSector[4096];

extern HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *SectorError);
extern HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint64_t Data);

#endif // __UNIT_TEST_DEFS_H