/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RADIO_TRANSMIT_H
#define __RADIO_TRANSMIT_H

#include "ProjectDefs.h"


#define RADIO_TX_PIN    GPIO_PIN_6                 // Port A, pin 6 is used for Radio transmitter
#define TSP200_TRANSMITTER_ID  ((uint32_t)0b11011011100011101111110110)

typedef struct {
  uint32_t TransmitterID;
  bool     GroupBit;
  bool     CmdBit;
  uint8_t  DeviceID;
  bool     PendingRequest;
} TSP200_CmdStruct;

typedef struct {
  int16_t Temperature;
  uint8_t Humidity;
  uint8_t DeviceID;
  bool    Battery;
  uint8_t Channel;
  uint8_t SerialNum;
  bool    PendingRequest;
} TSS320_MsgStruct;

/* External outputs (root outports fed by signals with auto storage) */
typedef struct {
  bool HeatControlAck;        // Acknowledgement signal to Heat Control client
  bool TempLoggerAck;
} ExtY_RadioTransmit_TxMgr_T;

extern ExtY_RadioTransmit_TxMgr_T RadioTransmit_TxMgr_Y;

extern TIM_HandleTypeDef        Timer13Handle;


extern void RadioTransmit_Init(void);
extern void RadioTransmit_100ms(void);

#endif // __RADIO_TRANSMIT_H
