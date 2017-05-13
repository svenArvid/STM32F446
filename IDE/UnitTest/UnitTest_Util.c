#include "UnitTest.h"
#include "Util.h"


uint16_t Xaxis[] = { 554, 629, 716, 817, 933, 1067, 1226, 1410, 1623, 1872, 2159, 2492, 2874, 3314, 3818, 4390, 5024, 5725, 6495,
7321, 8191, 9093, 9987, 10865, 11703, 12483, 13202, 13835, 14364, 14809, 15174, 15468, 15700, 15880 };

int16_t Yaxis[] = { 1250, 1200, 1150, 1100, 1050, 1000, 950, 900, 850, 800, 750, 700, 650, 600, 550, 500, 450, 400, 350,
300, 250, 200, 150, 100, 50, 0, -50, -100, -150, -200, -250, -300, -350, -400 };

// Axes with increasing Temperature:
const int16_t NTC3950_ADCArr[] = { 15880, 15700, 15468, 15174, 14809, 14364, 13835, 13202, 12483, 11703, 10865, 9987, 9093, 8191, 7321,
6495, 5725, 5024, 4390, 3818, 3314, 2874, 2492, 2159, 1872, 1623, 1410, 1226, 1067, 933, 817, 716, 629, 554 };

const int16_t NTC3950_TempArr[] = { -400, -350, -300, -250, -200, -150, -100, -50, 0, 50, 100, 150, 200, 250, 300, 350, 400, 450,
500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000, 1050, 1100, 1150, 1200, 1250 };

void UnitTest_Util_Interpolate(void)
{
  int32_t result;
  uint16_t ADC_Val[] = { 16000, 15880, 15879, 12000, 9578, 7468, 5123, 555, 554, 500 };
  int16_t Temperature[] = { -450, -400, -399, 31, 173, 292, 443, 1249, 1250, 1251 };

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