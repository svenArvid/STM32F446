// Do unit of certain functions in Visual studio

#include <stdio.h>
#include <stdint.h>
#include "Util.h"
#include "Crc.h"


uint16_t Xaxis[] = { 554, 629, 716, 817, 933, 1067, 1226, 1410, 1623, 1872, 2159, 2492, 2874, 3314, 3818, 4390, 5024, 5725, 6495,
                     7321, 8191, 9093, 9987, 10865, 11703, 12483, 13202, 13835, 14364, 14809, 15174, 15468, 15700, 15880 };
                     
int16_t Yaxis[] = {1250, 1200, 1150, 1100, 1050, 1000, 950, 900, 850, 800, 750, 700, 650, 600, 550, 500, 450, 400, 350,
                   300, 250, 200, 150, 100, 50, 0, -50, -100, -150, -200, -250, -300, -350, -400};

// Axes with increasing Temperature:
const int16_t NTC3950_ADCArr[] = {15880, 15700, 15468, 15174, 14809, 14364, 13835, 13202, 12483, 11703, 10865, 9987, 9093, 8191, 7321, 
                                6495, 5725, 5024, 4390, 3818, 3314, 2874, 2492, 2159, 1872, 1623, 1410, 1226, 1067, 933, 817, 716, 629, 554 };

const int16_t NTC3950_TempArr[] = {-400, -350, -300, -250, -200, -150, -100, -50, 0, 50, 100, 150, 200, 250, 300, 350, 400, 450, 
                                  500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000, 1050, 1100, 1150, 1200, 1250};


static void UnitTest_Util_Interpolate(void)
{
  int32_t result;
  uint16_t ADC_Val[] = { 16000, 15880, 15879, 12000, 9578, 7468, 5123, 555, 554, 500 };
  int16_t Temperature[] = {-450, -400, -399, 31, 173, 292, 443, 1249, 1250, 1251};
  
  printf(" Testing Util_Interpolate\n");
  for (int i = 0; i < 10; i++) {
    result = Util_Interpolate(ADC_Val[i], Xaxis, Yaxis, sizeof(Xaxis) / sizeof(Xaxis[0]));
    
    printf("ADC Value: %6d, Temperature: %4d\n", ADC_Val[i], result);
  }

  printf("\nTest to go the other direction, from Temperature to ADC val:\n");
  for (int i = 0; i < 10; i++) {
    result = Util_Interpolate(Temperature[i], Yaxis, Xaxis, sizeof(Xaxis) / sizeof(Xaxis[0]));

    printf("ADC Value: %6d, Temperature: %4d\n", result, Temperature[i]);
  }

  // ------------------------
  printf(" \nTesting with Axes with increasing Temperature\n");
  for (int i = 0; i < 10; i++) {
    result = Util_Interpolate(ADC_Val[i], NTC3950_ADCArr, NTC3950_TempArr, sizeof(Xaxis) / sizeof(Xaxis[0]));

    printf("ADC Value: %6d, Temperature: %4d\n", ADC_Val[i], result);
  }

  printf("\nTest to go the other direction, from Temperature to ADC val:\n");
  for (int i = 0; i < 10; i++) {
    result = Util_Interpolate(Temperature[i], NTC3950_TempArr, NTC3950_ADCArr, sizeof(Xaxis) / sizeof(Xaxis[0]));

    printf("ADC Value: %6d, Temperature: %4d\n", result, Temperature[i]);
  }
}

// ------ CRC ------
/* The width of the CRC calculation and result.
 * Modify the typedef for a 16 or 32-bit CRC standard.
*/
typedef uint8_t crc;

#define WIDTH  (8 * sizeof(crc))
#define TOPBIT (1 << (WIDTH - 1))
#define POLYNOMIAL 0x31           /* CRC-8-Dallas/Maxim Generator Polynomial */
crc  crcTable[256];

void UnitTest_CrcTableGenerator(void)
{
  crc  remainder;

 /* Compute the remainder of each possible dividend. */
  for (int dividend = 0; dividend < 256; ++dividend)
  {
    /* Start with the dividend followed by zeros. */
    remainder = dividend << (WIDTH - 8);

    /* Perform modulo-2 division, a bit at a time. */
    for (uint8_t bit = 8; bit > 0; --bit)
    {
      /* Try to divide the current data bit. */
      if (remainder & TOPBIT)
      {
        remainder = (remainder << 1) ^ POLYNOMIAL;
      }
      else
      {
        remainder = (remainder << 1);
      }
    }

    /* Store the result into the table. */
    crcTable[dividend] = remainder;
  }

  printf("\nLookup table for CRC-8-Dallas/Maxim Generator Polynomial\n");
  for (int dividend = 0; dividend < 256; ++dividend)
  {
    printf("0x%0X, ", crcTable[dividend]);
  }
} 

void UnitTest_CrcCalcCrc8(void)
{
  uint32_t Msg = 0b01001000011100001001110000101110;
  uint8_t Crc = 0;
  uint8_t ByteArr[4];

  Crc = Crc_CalcCrc8((uint8_t *)&Msg, 4);     // NOTE: This tweak to point to &Msg and cast to uint8_t * did not work! 
                                              // It seems that &Msg is the adress of the least significant byte and &Msg+3 
                                              // is the adress of most significant byte so going in reverse order.
  printf("\nCalculated 8 bit CRC in Hex: ");
  printf("%0X \n", Crc);

  //ByteArr[0] = Msg >> 24;
  //ByteArr[1] = Msg >> 16;
  //ByteArr[2] = Msg >> 8;
  //ByteArr[3] = Msg;

  memcpy(ByteArr, &Msg, 4);                 // Also look here to see that ByteArr[0] gets least sig. byte of Msg.
  Crc = Crc_CalcCrc8(ByteArr, 4);

  printf("\nCalculated 8 bit CRC in Hex: ");
  printf("%0X \n", Crc);
}

// Main Entry point
int main()
{
  //UnitTest_Util_Interpolate();

  UnitTest_CrcTableGenerator();

  UnitTest_CrcCalcCrc8();

  system("pause");
}