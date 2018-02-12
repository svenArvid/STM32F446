/**
******************************************************************************
* @file    /Src/RadioReceive.c
* @author  Joakim Carlsson
* @version V1.0
* @date    02-Sep-2017
* @brief   Radio Receiver pin is listened to by using Input Capture on TIMER5. See InputCapture for setup of GPIO, TIMER5 IRQ etc.

******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "RadioTransmit.h"
#include "RadioReceive.h"
#include "InputCapture.h"
#include "Util.h"
#include "Crc.h"
#include "Uart.h"

static volatile uint32_t RxBuff[RX_BUF_SIZE];
static volatile uint32_t RxBuffIndx = 0;

static uint32_t PulseLengths[RX_BUF_SIZE];

static TSS320_MsgStruct Rx_TSS320;


void RadioReceive_Init(void)
{
  ;
}

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
    Msg->DeviceID = (TempMessage >> 28) & 0x0F;        // Device ID, 4 bits
    Msg->Battery = Util_BitRead(TempMessage, 27);
    Msg->Channel = ((TempMessage >> 24) & 0x07) + 1;  // Channel, 3 bits, add 1 to make it 1-based
    Msg->SerialNum = (TempMessage >> 20) & 0x0F;        // Serial number, 4 bits

    Msg->Temperature = (TempMessage >> 8) & 0x7FF;   // Temperature, 11 bits
    Msg->Temperature = Util_BitRead(TempMessage, 19) ? -Msg->Temperature : Msg->Temperature;  // Get right sign of temperature
    Msg->Humidity = TempMessage & 0xFF;           // Humidity, 8 least significant bits

    return TRUE;
  }
  else
  {
    return FALSE;  // Crc check failed
  }
}


// Check RxBuff for new messages
static void RadioReceiver_CheckBuffer(void)
{
  static uint32_t LatestTimestampChecked = 0;
  uint32_t PulseLen;
  uint32_t PulseIndx;

  uint32_t StartInd = (RxBuffIndx - 1) % RX_BUF_SIZE;    // Most recent Input capture value
  uint32_t StopInd  = (RxBuffIndx + 1) % RX_BUF_SIZE;    // Skip the "oldest" index because this is next in turn to be written to and might get a new value anytime
  bool Result = FALSE;
  int32_t StrLength = 0;

  (void)memset(PulseLengths, 0, sizeof(PulseLengths));
  PulseIndx = RX_BUF_SIZE;

  // ------ PART I ------
  // Input capture timestamps reside in RxBuff, from these compute Pulse lenghts and put in array PulseLengths so that the most recent pulse is put at
  // last index of PulseLengths. We search backwards in RxBuff starting from the most recent input capture value. Note that RxBuff is circular array. 
  for (uint32_t i = StartInd; i != StopInd; i = (i - 1) % RX_BUF_SIZE)
  {
    if (RxBuff[i] <= LatestTimestampChecked) // Maybe Add Timer Wrap around TEST:  CurrentTime - A < CurrentTime - B
    {
      break;    // We can stop because we have reached an old value that we checked previous tick
    }
    else
    {
      PulseLen = RxBuff[i] - RxBuff[(i - 1) % RX_BUF_SIZE];
      PulseLengths[--PulseIndx] = PulseLen;
    }
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
      Result = TSS320_CheckMessage(i, &Rx_TSS320);

      // Possibly check other protocols if TSS320 did not match

      LatestTimestampChecked = RxBuff[(i - (RX_BUF_SIZE - 1 - StartInd)) % RX_BUF_SIZE];  // (RX_BUF_SIZE - 1 - StartInd) is the index offset between PulseLengths and RxBuff
    }
    else if (PulseLengths[i] == 0)
    {
      break;    // No more pulses, PulseLengths is not always filled because we only fill it until we reach LatestTimestampChecked
    }
  }
}


void RadioReceieve_100ms(void)
{
  RadioReceiver_CheckBuffer();
}


/**
* @brief  Capture IRQ in non blocking mode

*/
void TIM5_IRQHandler(void)
{  
  // TIM 5 Input Capture interrupt on Channel 1
  if (__HAL_TIM_GET_FLAG(&Timer5Handle, TIM_SR_CC1IF) != RESET)
  {
    if (__HAL_TIM_GET_IT_SOURCE(&Timer5Handle, TIM_DIER_CC1IE) != RESET)
    {
      RxBuff[RxBuffIndx] = Timer5Handle.Instance->CCR1;        // CCxIF flag is cleared when reading CCRx register 
      RxBuffIndx = (RxBuffIndx + 1) % RX_BUF_SIZE;
    }
  }
}

