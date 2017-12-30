#ifndef __EXPORTED_SIGNALS_H
#define __EXPORTED_SIGNALS_H

#include "ProjectDefs.h"


/* Updates the exported signals array by copying in the values of the signals to be exported */
void ExportedSignals_Update(void);

/* Read the signal at given index */
uint16_t ExportedSignals_Read(uint16_t indx);

#endif // __EXPORTED_SIGNALS_H