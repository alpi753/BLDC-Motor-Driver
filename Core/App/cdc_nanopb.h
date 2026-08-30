#ifndef CDC_NANOPB_H
#define CDC_NANOPB_H

#include <stddef.h>
#include <stdint.h>

typedef uint8_t (*CdcNanopbTransmitFn)(uint8_t *buffer, uint16_t length);
typedef struct
{
  uint16_t ntc_pcb;
  uint16_t curr_a;
  uint16_t curr_b;
  uint16_t curr_c;
  uint16_t volt_a;
  uint16_t volt_b;
  uint16_t volt_c;
  uint16_t vbus;
} CdcNanopbAdcSamples;

typedef void (*CdcNanopbReadAdcSamplesFn)(CdcNanopbAdcSamples *samples);

void CdcNanopb_Init(CdcNanopbTransmitFn transmit, CdcNanopbReadAdcSamplesFn read_adc_samples);
void CdcNanopb_Task(uint32_t now_ms);
void CdcNanopb_OnReceive(const uint8_t *buffer, uint32_t length);
void CdcNanopb_OnTransmitComplete(void);
uint8_t *CdcNanopb_RxBuffer(void);

#endif
