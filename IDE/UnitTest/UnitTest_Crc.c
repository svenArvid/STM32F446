// ------ CRC ------
/* The width of the CRC calculation and result.
* Modify the typedef for a 16 or 32-bit CRC standard.
*/
#include "UnitTest.h"
#include "Crc.h"

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

  (void)memcpy(ByteArr, &Msg, 4);                 // Also look here to see that ByteArr[0] gets least sig. byte of Msg.
  Crc = Crc_CalcCrc8(ByteArr, 4);

  printf("\nCalculated 8 bit CRC in Hex: ");
  printf("%0X \n", Crc);
}


#define PRINT_RESULT(comment) fprintf(fp, "0x%04X 	// %s\n", Crc, comment);
void UnitTest_Crc_CalcCrc16(void)
{
  uint16_t Crc;
  uint8_t buffer1[] = { 0x0A, 0x04, 0x10, 0x00, 0x00, 0x08 };
  uint8_t buffer2[] = { 0x0A, 0x04, 0x10, 0x00, 0x00, 0x08, 0xF4, 0x77 };
  uint8_t buffer3[] = { 0x0A, 0x04, 0x10, 0x01, 0xF5, 0x01, 0xF9, 0x00, 0xF7, 0x00, 0xF8, 0x03, 0x52, 0x01, 0x5E, 0x02, 0xBC, 0x00, 0x01 };
  
  fprintf(fp, "SignalList:\n");
  fprintf(fp, "  crc  \n--------------------------------------\n");
  
  Crc = Crc_CalcCrc16(buffer1, sizeof(buffer1) / sizeof(buffer1[0]));
  PRINT_RESULT("buffer: 0A 04 10 00 00 08  Expected Crc: 77 F4");

  Crc = Crc_CalcCrc16(buffer2, sizeof(buffer2) / sizeof(buffer2[0]));
  PRINT_RESULT("same buffer as above but appending Crc (high and low bytes swapped as in Modbus), thus expect Crc to be 0.");

  fprintf(fp, "\n");
  Crc = Crc_CalcCrc16(buffer3, sizeof(buffer3) / sizeof(buffer3[0]));
  PRINT_RESULT("buffer: 0A 04 10 01 F5 01 F9 00 F7 00 F8 03 52 01 5E 02 BC 00 01  Expected Crc: C4 BA");
}