// Do unit tests of certain functions in Visual studio
#include "UnitTest.h"

FILE* fp;

static UnitTest_TestCaseWrapper(char* FileName,  void(*TestCase)(void)) 
{
  char NewFile[100] = "Generated\\";
  char OldFile[100] = "Generated_old\\";
  char TempStr[100];

  char* DotPtr;

  (void)strcat(NewFile, FileName);
  (void)strcat(OldFile, FileName);

  // ------ File Handling ------
  if (remove(OldFile) == 0)
  {
    printf("File %s deleted.\n", OldFile);
  }
  else
  {
    printf("File %s not found.\n", OldFile);
  }

  if (rename(NewFile, OldFile) == 0)
  {
    printf("%s has been renamed %s.\n", NewFile, OldFile);
  }
  else
  {
    printf("File %s not found.\n", NewFile);
  }

  fp = fopen(NewFile, "w");

  // ------ Start Tests ------
  printf("\nStart Test %s\n", FileName);
  
  strcpy(TempStr, FileName);
  DotPtr = TempStr;
  while(1) 
  {
    if (*DotPtr == '.') {
      *DotPtr = '\0';    // Remove file extension from name when we write name to file.
    }
    if (*DotPtr == '\0') {
      break;
    }
    DotPtr++;
  }
  fprintf(fp, "%s\n", TempStr);
  TestCase();

  fclose(fp);
}

// Main Entry point
int main()
{
  UnitTest_CrcTableGenerator();

  UnitTest_CrcCalcCrc8();

  UnitTest_TestCaseWrapper("TC_Crc_CalcCrc16.txt", UnitTest_Crc_CalcCrc16);
  
  UnitTest_RadioReceive();

  UnitTest_TestCaseWrapper("TC_FlashE2p.txt", UnitTest_FlashE2p);

  UnitTest_TestCaseWrapper("TC_Util_SRLatch.txt", UnitTest_Util_SRLatch);

  UnitTest_TestCaseWrapper("TC_Util_FilterState.txt", UnitTest_Util_FilterState);

  UnitTest_TestCaseWrapper("TC_Util_Interpolate.txt", UnitTest_Util_Interpolate);

  UnitTest_TestCaseWrapper("TC_Util_Interpolate2D.txt", UnitTest_Util_Interpolate2D);

  UnitTest_TestCaseWrapper("TC_Util_Map.txt", UnitTest_Util_Map);

  printf("Floating: %.1f", 45.0);
  system("pause");
}