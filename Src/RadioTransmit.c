/**
******************************************************************************
* @file    /Src/RadioTransmit.c
* @author  Joakim Carlsson
* @version V1.0
* @date    31-Dec-2016
* @brief   Radio transmitter pin is controlled in order to get correct waveform. Timer 13 is setup to produce interrupts
*          so that the radio transmitter pin is toggled at the right time.
*
*          This file overrides the HAL __HAL_TIM_PeriodElapsedCallback function (defined as weak) 
           which is called from HAL_TIM_IRQHandler, which in turn is called from the actual Interrupt TIM13_IRQHandler.
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "RadioTransmit.h"
#include "Util.h"
#include "Crc.h"
#include "SensorMgr.h"
#include "Pwm.h"


/* Timer handler declaration */
TIM_HandleTypeDef        Timer13Handle;

TSP200_CmdStruct HeatControlCmd;
TSS320_MsgStruct TemperatureMsg = { .DeviceID = 4, .Battery = 1, .Channel = 1, .SerialNum  = 7};

ExtY_RadioTransmit_TxMgr_T RadioTransmit_TxMgr_Y;

/**
* @brief  This function configures the TIM13 as a time base source and output compare 
*         setup of GPIO pin connected to the radio transmitter
*         It Enables the Timer 13 interrupt and set it's priority.
*/
void RadioTransmit_Init(void)
{
  GPIO_InitTypeDef      GPIO_InitStruct;
  RCC_ClkInitTypeDef    clkconfig;
  uint32_t              uwTimclock;
  uint32_t              uwAPB1Prescaler = 0U;
  uint32_t              uwPrescalerValue = 0U;
  uint32_t              pFLatency;

  TIM_OC_InitTypeDef sConfig;        /* Timer Output Compare Configuration Structure declaration */

  /* Enable Port A, (GPIO PA6 is used)*/
  __HAL_RCC_GPIOA_CLK_ENABLE();
  
  /* Configure Pin */
  GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF9_TIM13;
  GPIO_InitStruct.Pin   = RADIO_TX_PIN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


  /*Configure the TIM13 IRQ priority */
  HAL_NVIC_SetPriority(TIM8_UP_TIM13_IRQn, 13U, 0U);   // Interrupt prio number 13, i.e. low prio. 

  /* Enable the TIM13 global Interrupt */
  HAL_NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);

  /* Enable TIM13 clock */
  __HAL_RCC_TIM13_CLK_ENABLE();

  /* Get clock configuration */
  HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
  
  /* Get APB1 prescaler */
  uwAPB1Prescaler = clkconfig.APB1CLKDivider;

  /* Compute TIM13 clock */
  if (uwAPB1Prescaler == RCC_HCLK_DIV1)
  {
    uwTimclock = HAL_RCC_GetPCLK1Freq();
  }
  else
  {
    uwTimclock = 2 * HAL_RCC_GetPCLK1Freq();
  }

  /* Compute the prescaler value to have TIM13 counter clock equal to 100 KHz */
  uwPrescalerValue = (uint32_t)((uwTimclock / 100000U) - 1U);

  /* Initialize TIM13 */
  Timer13Handle.Instance = TIM13;

  Timer13Handle.Init.Period = 1000U;
  Timer13Handle.Init.Prescaler = uwPrescalerValue;
  Timer13Handle.Init.ClockDivision = 0;
  Timer13Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
  
  HAL_TIM_OC_Init(&Timer13Handle);

  /*##-2- Configure the Output Compare channel #########################################*/
  sConfig.OCMode = TIM_OCMODE_PWM1;
  sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
  //sConfig.Pulse = ;
  HAL_TIM_OC_ConfigChannel(&Timer13Handle, &sConfig, TIM_CHANNEL_1);
  

  /* Initialize HeatControl object */
  HeatControlCmd.TransmitterID = TSP200_TRANSMITTER_ID;
  HeatControlCmd.DeviceID = 11;
  HeatControlCmd.GroupBit = FALSE;
  
}

// ============================================================
static volatile bool RadioTxBusy = FALSE;
static volatile uint32_t WfIndex = 0;
static volatile uint32_t WfRetransmit = 0; // Number of times a message shall be sent
static uint16_t Waveform[256]; 

// ============ TSP 200 Protocol ============
const uint16_t TSP200_StartPattern[] = { 25, 250 };
const uint16_t TSP200_PausePattern[] = { 25, 1000 };
const uint16_t TSP200_LineCodeZero[] = { 25, 25, 25, 125 };
const uint16_t TSP200_LineCodeOne[]  = { 25, 125, 25, 25 };


static void TSP200_WriteLineCode(bool BitSet, uint32_t *ArrIndex)
{
  if (BitSet) 
  {
    Waveform[(*ArrIndex)++] = TSP200_LineCodeOne[0];
    Waveform[(*ArrIndex)++] = TSP200_LineCodeOne[1];
    Waveform[(*ArrIndex)++] = TSP200_LineCodeOne[2];
    Waveform[(*ArrIndex)++] = TSP200_LineCodeOne[3];
  }
  else 
  {
    Waveform[(*ArrIndex)++] = TSP200_LineCodeZero[0];
    Waveform[(*ArrIndex)++] = TSP200_LineCodeZero[1];
    Waveform[(*ArrIndex)++] = TSP200_LineCodeZero[2];
    Waveform[(*ArrIndex)++] = TSP200_LineCodeZero[3];
  }
}


static void TSP200_MakeWaveform(const TSP200_CmdStruct *Cmd)
{
  uint32_t ArrIndex = 0;
  int32_t  BitIndex = 0;
  bool     BitSet = FALSE;

  /* Clear Array */
  (void)memset(Waveform, 0, sizeof(Waveform));
  WfIndex = 0;

  /* Start Pattern */
  Waveform[ArrIndex++] = TSP200_StartPattern[0];
  Waveform[ArrIndex++] = TSP200_StartPattern[1];

  /* TransmitterID, 26 bits */
  for (BitIndex = 25; BitIndex >= 0; BitIndex--)
  {
    BitSet = Util_BitRead(Cmd->TransmitterID, BitIndex);
    TSP200_WriteLineCode(BitSet, &ArrIndex);
  }

  /* Group Bit */
  TSP200_WriteLineCode(Cmd->GroupBit, &ArrIndex);

  /* Command Bit */
  TSP200_WriteLineCode(Cmd->CmdBit, &ArrIndex);

  /* DeviceID, 4 bits */
  for (BitIndex = 3; BitIndex >= 0; BitIndex--)
  {
    BitSet = Util_BitRead(Cmd->DeviceID, BitIndex);
    TSP200_WriteLineCode(BitSet, &ArrIndex);
  }

  /* Pause Pattern */
  Waveform[ArrIndex++] = TSP200_PausePattern[0];
  Waveform[ArrIndex++] = TSP200_PausePattern[1];

  /* Send message 3 times after each other */
  WfRetransmit = 3; 
}

// ============ TSS 320 Protocol ============
const uint16_t TSS320_LineCodeZero[] = { 140, 100 };
const uint16_t TSS320_LineCodeOne[]  = {  40, 100 };

static void TSS320_WriteLineCode(bool BitSet, uint32_t *ArrIndex)
{
  if (BitSet)
  {
    Waveform[(*ArrIndex)++] = TSS320_LineCodeOne[0];
    Waveform[(*ArrIndex)++] = TSS320_LineCodeOne[1];
  }
  else
  {
    Waveform[(*ArrIndex)++] = TSS320_LineCodeZero[0];
    Waveform[(*ArrIndex)++] = TSS320_LineCodeZero[1];
  }
}

static void TSS320_MakeWaveform(const TSS320_MsgStruct *Msg)
{
  uint32_t ArrIndex = 0;
  int32_t  BitIndex = 0;
  bool     BitSet = FALSE;
  uint32_t Message = 0;
  uint8_t  Crc = 0;
  uint8_t  ByteArr[4];

  /* Clear Array */
  (void)memset(Waveform, 0, sizeof(Waveform));
  WfIndex = 0;

  /* Preamble, Send 12 Ones */
  for (BitIndex = 11; BitIndex >= 0; BitIndex--)
  {
    TSS320_WriteLineCode(1, &ArrIndex);
  }
  
  // Put all bits in the 32 bit Message, start with Most significant bits.
  /* Device ID, 4 bits */
  Message |= (uint32_t)Msg->DeviceID << 28;

  /* Battery bit */
  Message |= (uint32_t)Msg->Battery << 27;

  /* Channel, 3 bits */
  Message |= (uint32_t)Msg->Channel << 24;

  /* Serial number, 4 bits */
  Message |= (uint32_t)Msg->SerialNum << 20;

  /* Temperature Sign bit */
  Util_BitWrite(Message, 19, Msg->Temperature < 0);

  /* Temperature, remaining 11 bits (remove sign bit) */
  Message |= (uint32_t)(abs(Msg->Temperature) & 0x7FF) << 8;
  //Message |= (uint32_t)(RoomTempSnsr.ADCVal >> 4 ) << 8;

  /* Humidity, 8 least significant bits */
  Message |= (uint32_t)Msg->Humidity;

  // Compute Crc, the data is contained in the 32 bit variable but function expect array of uint8 so copy data to the array.
  ByteArr[0] = Message >> 24;
  ByteArr[1] = Message >> 16;
  ByteArr[2] = Message >> 8;
  ByteArr[3] = Message;
  Crc = Crc_CalcCrc8(ByteArr, 4);

  /* Write Line codes for message (32 bits) and then for CRC (8 bits) */
  for (BitIndex = 31; BitIndex >= 0; BitIndex--)
  {
    BitSet = Util_BitRead(Message, BitIndex);
    TSS320_WriteLineCode(BitSet, &ArrIndex);
  }

  for (BitIndex = 7; BitIndex >= 0; BitIndex--)
  {
    BitSet = Util_BitRead(Crc, BitIndex);
    TSS320_WriteLineCode(BitSet, &ArrIndex);
  }

  /* Send message 1 time */
  WfRetransmit = 1;
}

// ====================================================
void RadioTransmit_TxStart(void)
{
  RadioTxBusy = TRUE;

  /* Start Radio Transmission, has to be done before calling HAL_TIM_Base_Start_IT, dont understand why though. */

  /* Start TIMER 13 in interrupt mode */
  Pwm_SetDuty(TIM13, TIM_CHANNEL_1, Waveform[0]);
  Pwm_SetPeriod(&Timer13Handle, Waveform[0] + Waveform[1]);

  HAL_TIM_OC_Start(&Timer13Handle, TIM_CHANNEL_1);
  HAL_TIM_Base_Start_IT(&Timer13Handle);
}

// Clients send Cmd requests to TxMgr who will build a waveform and send then start radio transmission
// by activating the Timer interrupt.
// The incoming client pending requests shall remain HIGH until TxMgr sends back an acknowledgement signal. 
// Client functions shall also execute in 100ms loop yo make sure they see the acknowledgment signal. 
void RadioTransmit_TxMgr(const TSP200_CmdStruct *HeatCtrl, const TSS320_MsgStruct *TempLog)
{
  /* Always reset all Ack signals here so that they are only Set for one tick. */
  RadioTransmit_TxMgr_Y.HeatControlAck = FALSE;
  RadioTransmit_TxMgr_Y.TempLoggerAck  = FALSE;


  if (!RadioTxBusy) // Only start new transmission if previous has finished
  {
    if (HeatCtrl->PendingRequest)
    {
      TSP200_MakeWaveform(HeatCtrl);
      RadioTransmit_TxStart();
      RadioTransmit_TxMgr_Y.HeatControlAck = TRUE;
    }
    else if(TempLog->PendingRequest)
    {
      TSS320_MakeWaveform(TempLog);
      RadioTransmit_TxStart();
      RadioTransmit_TxMgr_Y.TempLoggerAck = TRUE;
    }
    //... handle more client requests here
    
  }
}

void HeatController(bool TxMgrAck) 
{
  static Util_SRLatch SRLatch_Thermostat   = {FALSE, FALSE};
  static Util_SRLatch SRLatch_RadioRequest = {FALSE, FALSE};
  //static Util_Ramp    Ramp_Temperature = { 1, 2, 200 };

  bool Set, Reset;

  Set   = RoomTempSnsr.Temperature < 300;
  Reset = RoomTempSnsr.Temperature > 400;
  HeatControlCmd.CmdBit = Util_SetSRLatchState(&SRLatch_Thermostat, Set, Reset);

  Set = SRLatch_Thermostat.Toggled;
  Reset = TxMgrAck;
  HeatControlCmd.PendingRequest = Util_SetSRLatchState(&SRLatch_RadioRequest, Set, Reset);
}

void TemperatureLogger(bool TxMgrAck)
{
  static Util_SRLatch SRLatch_RadioRequest = { FALSE, FALSE };
  
  bool Set = FALSE;
  bool Reset;
  static uint32_t counter = 0;

  counter++;

  if (counter >= 100)   // Send message every 10 seconds
  { 
    counter = 0;
    Set = TRUE;
    
    TemperatureMsg.Temperature = RoomTempSnsr.Temperature;   // 500;
    TemperatureMsg.Humidity    = RoomTempSnsr.Status;        // report status in Humidity field
  }
 
  Reset = TxMgrAck;
  TemperatureMsg.PendingRequest = Util_SetSRLatchState(&SRLatch_RadioRequest, Set, Reset);
}

// First run client functions and then the TxMgr which serves the requests from the clients.
void RadioTransmit_100ms(void)
{
  HeatController(RadioTransmit_TxMgr_Y.HeatControlAck);

  TemperatureLogger(RadioTransmit_TxMgr_Y.TempLoggerAck);

  RadioTransmit_TxMgr(&HeatControlCmd, &TemperatureMsg);
}


/**
* @brief  Period elapsed callback in non blocking mode
* @note   This function is called  when TIM13 interrupt took place, inside
* HAL_TIM_IRQHandler(). 
  Note: Computation time can be reduced by NOT going via HAL IRQ function
*/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{  
  WfIndex += 2;

  if (Waveform[WfIndex] == '\0')   // End of waveform when "null" is found.
  {
    WfIndex = 0;
    WfRetransmit--;                 // Number of times to send message
    
    if (WfRetransmit <= 0) 
    {        /* Message finished. Stop Radio Transmission and disable Interrupt. */
      RadioTxBusy = FALSE;
      HAL_TIM_OC_Stop(&Timer13Handle, TIM_CHANNEL_1);
      HAL_TIM_Base_Stop_IT(&Timer13Handle);
      HAL_GPIO_WritePin(GPIOA, RADIO_TX_PIN, GPIO_PIN_RESET);
    }
  }
  Pwm_SetDuty(TIM13, TIM_CHANNEL_1, Waveform[WfIndex]);
  Pwm_SetPeriod(&Timer13Handle, Waveform[WfIndex] + Waveform[WfIndex + 1]);
}

