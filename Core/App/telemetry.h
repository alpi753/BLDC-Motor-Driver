#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>

void Telemetry_Init(void);
void Telemetry_Task(uint32_t now_ms);
uint8_t Telemetry_OnReceive(uint8_t *buffer, uint32_t length);
void Telemetry_OnTransmitComplete(void);

#endif
