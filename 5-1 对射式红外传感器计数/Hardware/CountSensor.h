#ifndef __COUNTSENSOR_H
#define __COUNTSENSOR_H

void CountSensor_Init(void);
uint16_t CountSensor_Get(void);
uint8_t CountSensor_GetFlag(void);
void CountSensor_ClearFlag(void);
void CountSensor_Add(void);  // 新增这一行

#endif
