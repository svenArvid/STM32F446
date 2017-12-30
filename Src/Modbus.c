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
#include "Modbus.h"
#include "Util.h"
#include "Uart.h"
#include "Crc.h"
#include "FlashE2p.h"
#include "ExportedSignals.h"


#define MODBUS_RX_READY        0
#define MODBUS_TX_SENDING      1
#define MODBUS_TX_WAIT_FOR_TC  2

#define MODBUS_TIMEOUT    100

#define READ_SIGNALS  0
#define READ_E2P      1
#define WRITE_E2P     1

#define SLAVE_ADDRESS_INDX  0
#define FUNCTION_CODE_INDX  1

static uint8_t Modbus_Address = 0xA;


// Request is considered valid if the following conditions are fullfilled:
// 1) The address in Request matches this unit's address
// 2) The Function code in the request is supported by SW
// 3) Message Crc matches computed Crc
// Returns TRUE if request is valid, otherwise FALSE
static bool Modbus_ValidRequest(uint16_t BytesReceived)
{
  uint16_t ComputedCrc, MessageCrc;
  bool Result = FALSE;

  if (ModbusPort.Rx.Buffer[SLAVE_ADDRESS_INDX] == Modbus_Address)  // My address ?
  {
    switch (ModbusPort.Rx.Buffer[FUNCTION_CODE_INDX])  // Function Code check
    {
    case 4:
    case 6:
      ComputedCrc = Crc_CalcCrc16(ModbusPort.Rx.Buffer, BytesReceived - 2);
      MessageCrc = ((uint16_t)ModbusPort.Rx.Buffer[BytesReceived - 1]) << 8;  // Note: The high and low byte of CRC shall be swapped in Modbus protocol 
      MessageCrc += ModbusPort.Rx.Buffer[BytesReceived - 2];

      if (MessageCrc == ComputedCrc)
      {
        Result = TRUE;
      }
      break;
    
    default:
      break;
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
  uint16_t TempInt;
  uint16_t(*pReadFunc)() = NULL;
  void (*pWriteFunc)()  = NULL;

  uint16_t WriteIndx = 0;
  uint16_t ReadIndx  = 0;
  uint16_t EndIndx = 0;
  
  ModbusPort.Tx.Buffer[0] = Modbus_Address;       // All responses start with address and Function code
  ModbusPort.Tx.Buffer[1] = ModbusPort.Rx.Buffer[FUNCTION_CODE_INDX];

  FirstAddress = (ModbusPort.Rx.Buffer[2] << 8) | ModbusPort.Rx.Buffer[3];  // Note: All Function code requests send address of first register at this location

  switch (ModbusPort.Rx.Buffer[FUNCTION_CODE_INDX])
  {
  case 4:
  {
    NumRegisters = (ModbusPort.Rx.Buffer[4] << 8) | ModbusPort.Rx.Buffer[5];  // Number of registers to read
    ModbusPort.Tx.Buffer[2] = 2 * NumRegisters;                               // Byte count of response payload

    switch (FirstAddress / 0x1000)
    {
    case READ_SIGNALS:
      ExportedSignals_Update();
      pReadFunc = ExportedSignals_Read;
      break;

    case READ_E2P:
      pReadFunc = FlashE2p_ReadMirror;
      break;

    default:
      return 0;  // This can happen if an illegal address is requested, thus we return 0 here and no response will be sent
    }

    // Write requested data (payload)
    EndIndx = (FirstAddress % 0x1000) + NumRegisters;
    for (WriteIndx = 3, ReadIndx = FirstAddress % 0x1000; ReadIndx < EndIndx; ReadIndx++)
    {
      TempInt = pReadFunc(ReadIndx);
      ModbusPort.Tx.Buffer[WriteIndx++] = (uint8_t)(TempInt >> 8);
      ModbusPort.Tx.Buffer[WriteIndx++] = (uint8_t)TempInt;
    }
    break;
  }
  case 6:
  {
    switch (FirstAddress / 0x1000)
    {
    case WRITE_E2P:
      pWriteFunc = FlashE2p_UpdateParameter;
      pReadFunc = FlashE2p_ReadMirror;
      break;

    default:
      return 0;  // This can happen if an illegal address is requested, thus we return 0 here and no response will be sent
    }

    pWriteFunc(FirstAddress % 0x1000, (ModbusPort.Rx.Buffer[4] << 8) | ModbusPort.Rx.Buffer[5]);  // Write received data on given address
    TempInt = pReadFunc(FirstAddress % 0x1000);

    ModbusPort.Tx.Buffer[2] = (uint8_t)(FirstAddress >> 8);    // FC 6: If register address is within limits the response will be an echo of the request
    ModbusPort.Tx.Buffer[3] = (uint8_t)FirstAddress;
    ModbusPort.Tx.Buffer[4] = (uint8_t)(TempInt >> 8);
    ModbusPort.Tx.Buffer[5] = (uint8_t)TempInt;
    WriteIndx = 6;
    break;
  }
  default:
    break;
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
