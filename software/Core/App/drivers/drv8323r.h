#ifndef DRV8323R_H
#define DRV8323R_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
  Drv8323rOvercurrentDisabled = 0,
  Drv8323rOvercurrentLimit = 1,
  Drv8323rOvercurrentLatchShutdown = 2,
} Drv8323rOvercurrentMode;

typedef enum
{
  Drv8323rCurrentSenseGain5VPerV = 5,
  Drv8323rCurrentSenseGain10VPerV = 10,
  Drv8323rCurrentSenseGain20VPerV = 20,
  Drv8323rCurrentSenseGain40VPerV = 40,
} Drv8323rCurrentSenseGain;

bool Drv8323r_Init(void);
uint16_t Drv8323r_ReadRegister(uint8_t address);
void Drv8323r_WriteRegister(uint8_t address, uint16_t value);
void Drv8323r_SetOvercurrentAdjustment(uint8_t value);
void Drv8323r_SetOvercurrentMode(Drv8323rOvercurrentMode mode);
void Drv8323r_SetCurrentSenseGain(Drv8323rCurrentSenseGain gain);
void Drv8323r_SetDcCalibration(bool enabled);
uint32_t Drv8323r_ReadFaults(void);
const char *Drv8323r_FaultsToString(uint32_t faults);
void Drv8323r_ClearFaults(void);

#endif
