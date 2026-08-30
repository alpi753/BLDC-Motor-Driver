#ifndef CDC_USB_BRIDGE_H
#define CDC_USB_BRIDGE_H

#include <stdint.h>

void AppCdc_Init(void);
void AppCdc_Task(uint32_t now_ms);
uint8_t AppCdc_OnReceive(uint8_t *buffer, uint32_t length);
void AppCdc_OnTransmitComplete(void);
void AppCdc_StateIndicator(uint32_t now_ms);
#endif
