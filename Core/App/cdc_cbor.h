#ifndef CDC_CBOR_H
#define CDC_CBOR_H

#include <stddef.h>
#include <stdint.h>

typedef uint8_t (*CdcCborTransmitFn)(uint8_t *buffer, uint16_t length);

void CdcCbor_Init(CdcCborTransmitFn transmit);
void CdcCbor_Task(uint32_t now_ms);
void CdcCbor_OnReceive(const uint8_t *buffer, uint32_t length);
void CdcCbor_OnTransmitComplete(void);
uint8_t *CdcCbor_RxBuffer(void);

#endif
