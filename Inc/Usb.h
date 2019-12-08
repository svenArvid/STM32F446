#ifndef __USB_H
#define __USB_H

#include "ProjectDefs.h"

#include "ff_gen_drv.h"       // FatFs includes component
#include "usbh_diskio_dma.h"

void Usb_Init(void);
void Usb_500ms(void);

#endif // __USB_H