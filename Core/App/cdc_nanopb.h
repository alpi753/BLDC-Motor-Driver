#ifndef CDC_NANOPB_H
#define CDC_NANOPB_H

#include <stddef.h>
#include <stdint.h>

typedef uint8_t (*CdcNanopbTransmitFn)(uint8_t *buffer, uint16_t length);
typedef uint16_t (*CdcNanopbNtcPcbReadFn)(void);

void CdcNanopb_Init(CdcNanopbTransmitFn transmit, CdcNanopbNtcPcbReadFn read_ntc_pcb);
void CdcNanopb_Task(uint32_t now_ms);
void CdcNanopb_OnReceive(const uint8_t *buffer, uint32_t length);
void CdcNanopb_OnTransmitComplete(void);
uint8_t *CdcNanopb_RxBuffer(void);

#endif
