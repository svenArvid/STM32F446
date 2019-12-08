/**
  ******************************************************************************
  * @file    FatFs/FatFs_USBDisk/Src/main.c 
  * @author  MCD Application Team
  * @brief   Main program body
  *          Use FatFs with USB disk drive.
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "ErrorHandler.h"
#include "Usb.h"
#include "Uart.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
FATFS USBDISKFatFs;          /* File system object for USB disk logical drive */
FIL MyFile;                  /* File object */
char USBDISKPath[4];         /* USB Host logical drive path */
USBH_HandleTypeDef hUSBHost; /* USB Host handle */

typedef enum {
  APPLICATION_IDLE = 0,  
  APPLICATION_CONNECTING,    
  APPLICATION_RUNNING,
}MSC_ApplicationTypeDef;

MSC_ApplicationTypeDef AppliState = APPLICATION_IDLE;

/* Private function prototypes -----------------------------------------------*/ 
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);
static void MSC_Application(void);

/* Private functions ---------------------------------------------------------*/
char *wtext = "This is STM32 working with FatFs\n/* Register the file system object to the FatFs module */\n/* Create and Open a new text file object with write access */\n/* Write data to the text file */\n";
char buffer[4096];
uint32_t bufferIndx = 0;

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
void Usb_Init(void)
{
  /*##-1- Link the USB Host disk I/O driver ##################################*/
  if(FATFS_LinkDriver(&USBH_Driver, USBDISKPath) == 0)
  {
    /*##-2- Init Host Library ################################################*/
    USBH_Init(&hUSBHost, USBH_UserProcess, 0);
    
    /*##-3- Add Supported Class ##############################################*/
    USBH_RegisterClass(&hUSBHost, USBH_MSC_CLASS);
    
    /*##-4- Start Host Process ###############################################*/
    USBH_Start(&hUSBHost);

    UART_PRINTF("Usb Host started, ms: %d\r\n", HAL_GetTick());
  } 
}

/*##-5- Run Application (Blocking mode) ##################################*/
void Usb_500ms(void)
{
  uint32_t startTic = HAL_GetTick();

  /* USB Host Background task. When connecting USB device, need to run state machine in while loop to speed up, but yield after 250ms */
  do {
    USBH_Process(&hUSBHost);
  } while (AppliState == APPLICATION_CONNECTING && HAL_GetTick() - startTic < 250);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
  
  /* Mass Storage Application State Machine */
  switch (AppliState)
  {
  case APPLICATION_RUNNING:
    MSC_Application();
    break;

  case APPLICATION_IDLE:
  default:
    break;
  }
}

/**
  * @brief  Main routine for Mass Storage Class
  * @param  None
  * @retval None
  */
static void MSC_Application(void)
{
  FRESULT res;                                          /* FatFs function common result code */
  uint32_t byteswritten, bytesread;                     /* File write/read counts */

  strcpy(&buffer[bufferIndx], wtext);
  bufferIndx += strlen(wtext);
  
  if (bufferIndx < 3600)
    return;

  /* Register the file system object to the FatFs module */
  if(f_mount(&USBDISKFatFs, (TCHAR const*)USBDISKPath, 0) != FR_OK)
  {
    /* FatFs Initialization Error */
    Error_Handler();
  }
  else
  { 
    /* Create and Open a new text file object with write access */
    if(f_open(&MyFile, "STM32.TXT", FA_OPEN_ALWAYS | FA_WRITE) != FR_OK)
    {
      /* 'STM32.TXT' file Open for write Error */
      Error_Handler();
    }
    else
    {
      /* Move to end of the file to append data */
      res = f_lseek(&MyFile, f_size(&MyFile));

      /* Write data to the text file */
      res = f_write(&MyFile, buffer, bufferIndx, (void *)&byteswritten);
      bufferIndx = 0;

      if((byteswritten == 0) || (res != FR_OK))
      {
        /* 'STM32.TXT' file Write or EOF Error */
        Error_Handler();
      }
      else
      {
        /* Close the open text file */
        f_close(&MyFile);
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);

        /* Open the text file object with read access */
        //if(f_open(&MyFile, "STM32.TXT", FA_READ) != FR_OK)
        //{
        //  /* 'STM32.TXT' file Open for read Error */
        //  Error_Handler();
        //}
        //else
        //{
        //  /* Read data from the text file */
        //  res = f_read(&MyFile, rtext, sizeof(rtext), (void *)&bytesread);
        //  
        //  /* Close the open text file */
        //  f_close(&MyFile);
      }
    }
  }
  
  /* Unlink the USB disk I/O driver */
  //FATFS_UnLinkDriver(USBDISKPath);
}

/**
  * @brief  User Process
  * @param  phost: Host handle
  * @param  id: Host Library user message ID
  * @retval None
  */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{  
  switch(id)
  { 
  case HOST_USER_CONNECTION:
    AppliState = APPLICATION_CONNECTING;
    UART_PRINTF("Usb HOST_USER_CONNECTION, ms: %d\r\n", HAL_GetTick());
    break;

  case HOST_USER_SELECT_CONFIGURATION:
  case HOST_USER_CLASS_SELECTED:
    AppliState = APPLICATION_CONNECTING;
    break;

  case HOST_USER_DISCONNECTION:
  case HOST_USER_UNRECOVERED_ERROR:
    AppliState = APPLICATION_IDLE;
    f_mount(NULL, (TCHAR const*)"", 0);
    
    if (id == HOST_USER_DISCONNECTION)
      UART_PRINTF("Usb HOST_USER_DISCONNECTION, ms: %d\r\n", HAL_GetTick());
    else
      UART_PRINTF("Usb HOST_USER_UNRECOVERED_ERROR, ms: %d\r\n", HAL_GetTick());
    break;
    
  case HOST_USER_CLASS_ACTIVE:
    AppliState = APPLICATION_RUNNING;
    UART_PRINTF("Usb HOST_USER_CLASS_ACTIVE, ms: %d\r\n", HAL_GetTick());
    break;
    
  default:
    break;
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
