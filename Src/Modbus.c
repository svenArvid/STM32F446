/**
******************************************************************************
* @file    /Src/Modbus.c
* @author  Joakim Carlsson
* @version V1.0
* @date    1-October-2017
* @brief   Implements a Modbus slave to communicate with PC, using a UartPort object that is setup in Uart.c
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "ProjectDefs.h"
#include "Util.h"
#include "Uart.h"
#include "Crc.h"
#include "FlashE2p.h"


#define MODBUS_RX_READY        0
#define MODBUS_TX_SENDING      1
#define MODBUS_TX_WAIT_FOR_TC  2

#define MODBUS_TIMEOUT    100

#define READ_E2P 1

static uint8_t Modbus_Address = 0xA;
static uint8_t Modbus_FunctionCode = 0;


// Request is considered valid if the following conditions are fullfilled:
// 1) Message Crc matches computed Crc
// 2) The address in Request matches this unit's address
// 3) The Function code in the request is supported by SW
// Returns TRUE if request is valid, otherwise FALSE
static bool Modbus_ValidRequest(uint16_t BytesReceived)
{
  uint16_t ComputedCrc, MessageCrc;
  bool Result = FALSE;
  uint8_t MessageFunctionCode;

  ComputedCrc = Crc_CalcCrc16(ModbusPort.Rx.Buffer, BytesReceived - 2);
  MessageCrc  = ((uint16_t)ModbusPort.Rx.Buffer[BytesReceived - 1]) << 8;  // Note: The high and low byte of CRC shall be swapped in Modbus protocol 
  MessageCrc += ModbusPort.Rx.Buffer[BytesReceived - 2];

  UART_PRINTF("\r\nComputed Crc: %X, Message Crc: %X\r\n", ComputedCrc, MessageCrc);
  UART_PRINTF("Slave address %X\r\n", ModbusPort.Rx.Buffer[0]);
  UART_PRINTF("Function Code %X\r\n", ModbusPort.Rx.Buffer[1]);

  if (MessageCrc == ComputedCrc)
  {
    if (ModbusPort.Rx.Buffer[0] == Modbus_Address)  // My address ?
    {
      switch (ModbusPort.Rx.Buffer[1])  // Function Code check
      {
      case 4:
        Modbus_FunctionCode = 4;
        UART_PRINTF("Run Function Code 4\r\n");
        Result = TRUE;
        break;
      case 6:
        Modbus_FunctionCode = 6;
        Result = TRUE;
        break;
      default:
        break;
      }
    }
  }
  return Result;
}

// Serve received request acc. to Function code and put response message in Tx buffer 
// Returns number of bytes to send
static uint16_t Modbus_ServeRequest(void)
{
  uint16_t ResponseCrc;
  uint16_t FirstAddress = 0;
  uint16_t NumRegisters = 0;
  int16_t TempInt;
  int16_t(*pReadFunc)() = NULL;
  uint16_t WriteIndx = 0;
  uint16_t ReadIndx  = 0;
  
  UART_PRINTF("Server Request\r\n");
  ModbusPort.Tx.Buffer[0] = Modbus_Address;       // All responses start with address and Function code
  ModbusPort.Tx.Buffer[1] = Modbus_FunctionCode;

  FirstAddress = (ModbusPort.Rx.Buffer[2] << 8) | ModbusPort.Rx.Buffer[3];  // Note: All Function codes send address of first register at this location

  if (Modbus_FunctionCode == 4)
  {
    UART_PRINTF("FC 4\r\n");
    NumRegisters = (ModbusPort.Rx.Buffer[4] << 8) | ModbusPort.Rx.Buffer[5];  // Number of registers to read
    ModbusPort.Tx.Buffer[2] = 2 * NumRegisters;                               // Byte count of response payload

    switch (FirstAddress / 0x1000)  
    {
    case READ_E2P:
      UART_PRINTF("case READ_E2P\r\n");
      pReadFunc = FlashE2p_ReadMirror;
      break;
    default:
      UART_PRINTF("default\r\n");
      return 0;  // This can happen if an illegal address is requested, thus we return 0 here and no response will be sent
    }

    // Write requested data (payload)
    for (WriteIndx = 3, ReadIndx = FirstAddress % 0x1000; ReadIndx < (FirstAddress % 0x1000) + NumRegisters; ReadIndx++)
    {
        TempInt = pReadFunc(ReadIndx);
        UART_PRINTF("Read: %d\r\n", TempInt);
        ModbusPort.Tx.Buffer[WriteIndx++] = (uint8_t)(TempInt >> 8);
        ModbusPort.Tx.Buffer[WriteIndx++] = (uint8_t)TempInt;
    }
  }
  else if (Modbus_FunctionCode == 6)
  {
    ;
  }

  // All responses end with Crc
  ResponseCrc = Crc_CalcCrc16(ModbusPort.Tx.Buffer, WriteIndx);
  ModbusPort.Tx.Buffer[WriteIndx++] = (uint8_t)ResponseCrc;          // Note: The high and low byte of CRC shall be swapped in Modbus protocol
  ModbusPort.Tx.Buffer[WriteIndx++] = (uint8_t)(ResponseCrc >> 8);

  return WriteIndx;
}

void Modbus_StateMachine(void)
{
  static int State = MODBUS_RX_READY;
  static uint32_t Timer = 0;
  static uint16_t BytesToSend = 0;
  uint16_t BytesReceived = 0;

  switch (State) 
  {
  case MODBUS_RX_READY:
    BytesReceived = Uart_MessageReceived(&ModbusPort);
    if (BytesReceived > 0)
    {
      if (Modbus_ValidRequest(BytesReceived))
      {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
        Uart_StopReceiver(&ModbusPort);
        State = MODBUS_TX_SENDING;
        
        BytesToSend = Modbus_ServeRequest();    // Note: Response is computed and put in Tx buffer, but it is transmitted next tick.
      }
      else
      {
        Uart_StartReceiver(&ModbusPort);  // Restart receiver
      }
    }
    break;

  case MODBUS_TX_SENDING:
    Uart_StartTransmitter(&ModbusPort, BytesToSend);
    State = MODBUS_TX_WAIT_FOR_TC;
    break;
  
  case MODBUS_TX_WAIT_FOR_TC:
    if (Uart_TransmissionComplete(&ModbusPort) || Timer++ > MODBUS_TIMEOUT)
    {
      Uart_StopTransmitter(&ModbusPort);
      Uart_StartReceiver(&ModbusPort);
      Timer = 0;
      State = MODBUS_RX_READY;
    }
    break;

  default:
    State = MODBUS_RX_READY;
    break;
  }

}


void Modbus_4ms(void)
{
  Modbus_StateMachine();
}
