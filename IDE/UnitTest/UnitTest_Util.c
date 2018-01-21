#include <string.h>
#include "UnitTest.h"
#include "Util.h"
#include "Crc.h"
#include "RadioReceive.h"


static uint16_t axis1[] = { 554, 629, 716, 817, 933, 1067, 1226, 1410, 1623, 1872, 2159, 2492, 2874, 3314, 3818, 4390, 5024, 5725, 6495,
7321, 8191, 9093, 9987, 10865, 11703, 12483, 13202, 13835, 14364, 14809, 15174, 15468, 15700, 15880 };

static int16_t axis2[] = { 1250, 1200, 1150, 1100, 1050, 1000, 950, 900, 850, 800, 750, 700, 650, 600, 550, 500, 450, 400, 350,
300, 250, 200, 150, 100, 50, 0, -50, -100, -150, -200, -250, -300, -350, -400 };

// Axes with increasing Temperature:
static const int16_t NTC3950_ADCArr[] = { 15880, 15700, 15468, 15174, 14809, 14364, 13835, 13202, 12483, 11703, 10865, 9987, 9093, 8191, 7321,
6495, 5725, 5024, 4390, 3818, 3314, 2874, 2492, 2159, 1872, 1623, 1410, 1226, 1067, 933, 817, 716, 629, 554 };

static const int16_t NTC3950_TempArr[] = { -400, -350, -300, -250, -200, -150, -100, -50, 0, 50, 100, 150, 200, 250, 300, 350, 400, 450,
500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000, 1050, 1100, 1150, 1200, 1250 };

#define PRINT_RESULT(x, y, comment) fprintf(fp, "%6d %6d 	// %s\n", x, y, comment);
void UnitTest_Util_Interpolate(void)
{
  int32_t result;
  uint16_t ADC_Val[] = { 16000, 15880, 15879, 15174, 9987, 7468, 5123, 555, 554, 500 };
  int16_t Temperature[] = { -450, -400, -399, 31, 173, 292, 443, 1249, 1250, 1251 };

  fprintf(fp, "SignalList:\n");
  fprintf(fp, "   X      Y  \n--------------------------------------\n");

  fprintf(fp, " \nTesting with Axes with decreasing Temperature and increasing ADC Vals\n");
  for (int i = 0; i < 10; i++) {
    result = Util_Interpolate(ADC_Val[i], axis1, axis2, sizeof(axis1) / sizeof(axis1[0]));
    PRINT_RESULT(ADC_Val[i], result, "");
  }

  fprintf(fp, "\nTest to go the other direction, from Temperature to ADC val:\n");
  for (int i = 0; i < 10; i++) {
    result = Util_Interpolate(Temperature[i], axis2, axis1, sizeof(axis1) / sizeof(axis1[0]));
    PRINT_RESULT(Temperature[i], result, "");
  }

  // ------------------------
  fprintf(fp, " \nTesting with reversed axes i.e. axes with increasing Temperature and decreasing ADC Vals\n");
  for (int i = 0; i < 10; i++) {
    result = Util_Interpolate(ADC_Val[i], NTC3950_ADCArr, NTC3950_TempArr, sizeof(NTC3950_ADCArr) / sizeof(NTC3950_ADCArr[0]));
    PRINT_RESULT(ADC_Val[i], result, "");
  }

  fprintf(fp, "\nTest to go the other direction, from Temperature to ADC val:\n");
  for (int i = 0; i < 10; i++) {
    result = Util_Interpolate(Temperature[i], NTC3950_TempArr, NTC3950_ADCArr, sizeof(NTC3950_ADCArr) / sizeof(NTC3950_ADCArr[0]));
    PRINT_RESULT(Temperature[i], result, "");
  }
}

//-----------------------------------------------------------------------
Util_SRLatch latch;

#define PRINT_RESULT(comment) fprintf(fp, "%3d %3d %3d %3d		// %s\n", Set, Reset, latch.State, latch.Toggled, comment);
void UnitTest_Util_SRLatch(void) {
  bool Set = FALSE;
  bool Reset = FALSE;

  fprintf(fp, "SignalList:\n");
  fprintf(fp, "Set  Reset  latch.State  latch.Toggled\n--------------------------------------\n");
  PRINT_RESULT("Init Values");

  Set = TRUE;
  Reset = FALSE;
  Util_SetSRLatchState(&latch, Set, Reset);
  PRINT_RESULT("Set")

  Util_SetSRLatchState(&latch, Set, Reset);
  PRINT_RESULT("Inputs unchanged. state unchanged, thus Toggled is 0")

  Set   = FALSE;
  Reset = FALSE;
  Util_SetSRLatchState(&latch, Set, Reset);
  PRINT_RESULT("Inputs zero, state is latched")

  Set   = FALSE;
  Reset = TRUE;
  Util_SetSRLatchState(&latch, Set, Reset);
  PRINT_RESULT("Reset")

  Set   = FALSE;
  Reset = FALSE;
  Util_SetSRLatchState(&latch, Set, Reset);
  PRINT_RESULT("Inputs zero, state is latched")

  Set   = TRUE;
  Reset = FALSE;
  Util_SetSRLatchState(&latch, Set, Reset);
  PRINT_RESULT("Set")

  Set   = TRUE;
  Reset = TRUE;
  Util_SetSRLatchState(&latch, Set, Reset);
  PRINT_RESULT("Both inputs active, Reset dominates")
}

//-----------------------------------------------------------------------
Util_Filter filter;

#define PRINT_RESULT(comment) fprintf(fp, "%4d %4d %4d 	// %s\n", Input, Output, filter.Samples, comment);
void UnitTest_Util_FilterState(void) 
{
  int32_t Input = 100;
  int32_t Output;
  filter.Samples = 20;

  fprintf(fp, "SignalList:\n");
  fprintf(fp, "Input  Output  filter.Samples\n--------------------------------------\n");
  Util_SetFilterState(&filter, Input);
  Output = Util_FilterState(&filter, Input);
  PRINT_RESULT("Init Values. State is set to Input");
  
  Input = 0;
  Output = Util_FilterState(&filter, Input);
  PRINT_RESULT("Step response 100 -> 0");

  for (int i = 0; i < 60; i++) {
    Output = Util_FilterState(&filter, Input);
    PRINT_RESULT("");
  }

  Input = 200;
  Output = Util_FilterState(&filter, Input);
  PRINT_RESULT("Step response 0 -> 200");

  for (int i = 0; i < 60; i++) {
    Output = Util_FilterState(&filter, Input);
    PRINT_RESULT("");
  }

  filter.Samples = 50;
  Input = 0;
  Output = Util_FilterState(&filter, Input);
  PRINT_RESULT("Increase Time constant. Step response 200 -> 0");

  for (int i = 0; i < 60; i++) {
    Output = Util_FilterState(&filter, Input);
    PRINT_RESULT("");
  }
}

#define PRINT_RESULT(comment) fprintf(fp, "%5d %5d %5d %5d %5d %5d	// %s\n", x, y, x_min, x_max, y_min, y_max, comment);
void UnitTest_Util_Map(void)
{
  int32_t x, y;
  int32_t x_min, x_max;
  int32_t y_min, y_max;

  fprintf(fp, "SignalList:\n");
  fprintf(fp, "   x    y   x_min  x_max  y_min  y_max\n--------------------------------------\n");
  
  x_min = 640;
  x_max = 3200;
  y_min = 650;
  y_max = 1600;

  x = 600;
  y = Util_Map(x, x_min, x_max, y_min, y_max);
  PRINT_RESULT("AD value -> engine rpm");

  x = 640;
  y = Util_Map(x, x_min, x_max, y_min, y_max);
  PRINT_RESULT("Boundary value");

  x = 892;
  y = Util_Map(x, x_min, x_max, y_min, y_max);
  PRINT_RESULT("");

  x = 2000;
  y = Util_Map(x, x_min, x_max, y_min, y_max);
  PRINT_RESULT("");

  x = 3200;
  y = Util_Map(x, x_min, x_max, y_min, y_max);
  PRINT_RESULT("Boundary value");

  x = 3300;
  y = Util_Map(x, x_min, x_max, y_min, y_max);
  PRINT_RESULT("values outside range are extrapolated");

  x_min = 600;
  x_max = 0;
  y_min = 0;
  y_max = 64;
  
  x = 100;
  y = Util_Map(x, x_min, x_max, y_min, y_max);
  PRINT_RESULT("Note that x_min < x_max, and x_min is mapped to y_min");

  x = 500;
  y = Util_Map(x, x_min, x_max, y_min, y_max);
  PRINT_RESULT("");

  x = 700;
  y = Util_Map(x, x_min, x_max, y_min, y_max);
  PRINT_RESULT("");

  x_min = 600;
  x_max = 600;
  y = Util_Map(x, x_min, x_max, y_min, y_max);
  PRINT_RESULT("x_min == x_max is illegal input. To avoid division by 0, return value is set to y_min");

  x_min = 15174;
  x_max = 15468;
  y_min = -250;
  y_max = -300;

  x = x_max;
  y = Util_Map(x, x_min, x_max, y_min, y_max);
  PRINT_RESULT("");
}


//uint32_t RxBuff[RX_BUF_SIZE] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1412,1032,432,1030,1412,1032,1412,1032,432,1030,450,1030,1412,1032,1410,1034,1408,1034,424,1034,436,1032,430,1042,1402,1042,1398,1042,1400,1044,1398,1040,420,1050,418,1050,414,1040,426,1060,1380,1048,1398,1044,1398,1044,416,1046,1396,1052,1388,1050,422,1048,416,1054,1386,1050,1390,1048,422,1056,1388,1046,412,1054,1390,1056,1386,1052,414,1068,1372,1054,412,1050,412,1056,1386,122500 };
uint32_t RxBuff[RX_BUF_SIZE] = {1412, 2444, 2876, 3906, 5318, 6350, 7762, 8794, 9226, 10256, 10706, 11736, 13148, 14180, 15590, 16624, 18032, 19066, 19490, 20524, 20960, 21992, 22422, 23464, 24866, 25908, 27306, 28348, 29748, 30792, 32190, 33230, 33650, 34700, 35118, 36168, 36582, 37622, 38048, 39108, 40488, 41536, 42934, 43978, 45376, 46420, 46836, 47882, 49278, 50330, 51718, 52768, 53190, 54238, 54654, 55708, 57094, 58144, 59534, 60582, 61004, 62060, 63448, 64494, 64906, 65960, 67350, 68406, 69792, 70844, 71258, 72326, 73698, 74752, 75164, 76214, 76626, 77682, 79068, 201568, 203000};
static uint32_t PulseLengths[RX_BUF_SIZE];

// Returns TRUE if message is a valid TSS320 message and FALSE otherwise.
static bool TSS320_CheckMessage(uint32_t SilenceIndx, TSS320_MsgStruct *Msg)
{
  uint32_t TempMessage = 0;
  uint8_t  TempChecksum = 0;
  uint8_t  StartInd;
  uint8_t  StopInd;
  uint8_t  Bit;
  uint8_t  ByteArr[4];

  // Search message backwards starting from the index of the "silence". The Last byte in message is the CRC so start with it
  // Strategy: Check that the pulse length of the LOW pulses are within tolerance. If not abort and return FALSE. If Ok, continue and check if the HIGH pulse immediately after is a ONE.
  StartInd = (SilenceIndx - 2) % RX_BUF_SIZE;
  StopInd = (StartInd - 16) % RX_BUF_SIZE;
  Bit = 0;
  for (uint8_t i = StartInd; i != StopInd; i = (i - 2) % RX_BUF_SIZE)
  {
    if (Util_InRange(PulseLengths[i], TSS320_LOW_PULSE_LOWER_LIM, TSS320_LOW_PULSE_UPPER_LIM))
    {
      if (Util_InRange(PulseLengths[(i + 1) % RX_BUF_SIZE], TSS320_HIGH_ONE_PULSE_LOWER_LIM, TSS320_HIGH_ONE_PULSE_UPPER_LIM))
      {
        Util_BitSet(TempChecksum, Bit);
      }
    }
    else
    {
      return FALSE;  // Message not according to TSS320 protocol
    }
    Bit++;
  }

  // Continue with the message part, start from behind.
  Util_BitSet(TempMessage, 30);  // In TSS320 messages, the first 4 bits are always 0100, and since noise is most likely at beginning do not attempt to read them, i.e. only read last 28 bits.
  StartInd = StopInd;
  StopInd = (StartInd - 56) % RX_BUF_SIZE;
  Bit = 0;
  for (uint8_t i = StartInd; i != StopInd; i = (i - 2) % RX_BUF_SIZE)
  {
    if (Util_InRange(PulseLengths[i], TSS320_LOW_PULSE_LOWER_LIM, TSS320_LOW_PULSE_UPPER_LIM))
    {
      if (Util_InRange(PulseLengths[(i + 1) % RX_BUF_SIZE], TSS320_HIGH_ONE_PULSE_LOWER_LIM, TSS320_HIGH_ONE_PULSE_UPPER_LIM))
      {
        Util_BitSet(TempMessage, Bit);
      }
    }
    else
    {
      return FALSE;  // Message not according to TSS320 protocol
    }
    Bit++;
  }

  // Message seems to be ok according to TSS320 protocol, verify Crc
  // Compute Crc, the data is contained in the 32 bit variable but Crc function expect array of uint8 so copy data to the array.
  ByteArr[0] = TempMessage >> 24;
  ByteArr[1] = TempMessage >> 16;
  ByteArr[2] = TempMessage >> 8;
  ByteArr[3] = TempMessage;

  if (TempChecksum == Crc_CalcCrc8(ByteArr, 4))
  {
    Msg->DeviceID  = (TempMessage >> 28) & 0x0F;        // Device ID, 4 bits
    Msg->Battery   = Util_BitRead(TempMessage, 27);
    Msg->Channel   = ((TempMessage >> 24) & 0x07) + 1;  // Channel, 3 bits, add 1 to make it 1-based
    Msg->SerialNum = (TempMessage >> 20) & 0x0F;        // Serial number, 4 bits
   
    Msg->Temperature = (TempMessage >> 8) & 0x7FF;   // Temperature, 11 bits
    Msg->Temperature = Util_BitRead(TempMessage, 19) ? -Msg->Temperature : Msg->Temperature;  // Get right sign of temperature
    Msg->Humidity    = TempMessage & 0xFF;           // Humidity, 8 least significant bits
    
    return TRUE;
  }
  else
  {
    return FALSE;  // Crc check failed
  }
}

static TSS320_MsgStruct Rx_TSS320;

void UnitTest_RadioReceive(void)
{
  static uint32_t LatestTimestampChecked = 0;
  uint32_t PulseLen;
  uint32_t PulseIndx;

  uint32_t StartInd = 80;                             // Need to read this value
  uint32_t StopInd = (StartInd + 1) % RX_BUF_SIZE;
  bool Result;

  (void)memset(PulseLengths, 0, sizeof(PulseLengths));
  PulseIndx = RX_BUF_SIZE;

  // ------ PART I ------
  // Input capture timestamps reside in RxBuff, from these compute Pulse lenghts and put in array PulseLengths so that the most recent pulse is put at
  // last index of PulseLengths. We search backwards in RxBuff starting from the most recent input capture value. Note that RxBuff is circular array. 
  for (uint32_t i = StartInd; i != StopInd; i = (i - 1) % RX_BUF_SIZE)
  {
    if (RxBuff[i] <= LatestTimestampChecked) // Maybe Add Timer Wrap around TEST:  CurrentTime - A < CurrentTime - B
    {
      break;    // We can stop because we have reached an old value that we have already checked
    }
    else 
    {
      PulseLen = RxBuff[i] - RxBuff[(i - 1) % RX_BUF_SIZE];
      PulseLengths[--PulseIndx] = PulseLen;
    }
    printf("PulseIndx: %d\n", PulseIndx);
  }

  // ------ PART II ------
  // Search in PulseLengths for pulses that are longer than threshhold value and thus are candidates for the silence that appear immediately after a received message.
  // If such a candidate is found call the CheckMessage function. 
  // Also save the timestamp of this pulse so that we next time can decide to stop the search when we have reached an old timestamp that we already checked. 
  // Note that PulseLengths is NOT a circular array.
  for (uint32_t i = PulseIndx; i < RX_BUF_SIZE; i++)
  {
    if (PulseLengths[i] > RX_MIN_SILENCE)
    {
      printf("Radio receiver: %d, %d\n\n", i, PulseLengths[i]);
      
      Result = TSS320_CheckMessage(i, &Rx_TSS320);
      
      // Possibly check other protocols if TSS320 did not match
      
      LatestTimestampChecked = RxBuff[(i - (RX_BUF_SIZE - 1 - StartInd)) % RX_BUF_SIZE];  // (RX_BUF_SIZE - 1 - StartInd) is the index offset between PulseLengths and RxBuff
    }
    printf("%d\n", i);
  }
}