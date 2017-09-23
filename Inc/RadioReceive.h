/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RADIO_RECEIVE_H
#define __RADIO_RECEIVE_H

#include "ProjectDefs.h"

#define RX_BUF_SIZE       256
#define RX_MIN_SILENCE  1000

#define TSS320_LOW_PULSE_LOWER_LIM  100
#define TSS320_LOW_PULSE_UPPER_LIM  110

#define TSS320_HIGH_ZERO_PULSE_LOWER_LIM  134
#define TSS320_HIGH_ZERO_PULSE_UPPER_LIM  144

#define TSS320_HIGH_ONE_PULSE_LOWER_LIM  37
#define TSS320_HIGH_ONE_PULSE_UPPER_LIM  47


extern void RadioReceive_Init(void);
extern void RadioReceieve_100ms(void);

#endif // __RADIO_RECEIVE_H
