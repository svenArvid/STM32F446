#ifndef __RTC_H
#define __RTC_H

#include "ProjectDefs.h"

void RTC_Init(void);
void RTC_CalendarShow(char *showtime, char *showdate);
uint32_t RTC_GetTimeStamp(void);

#endif // __RTC_H