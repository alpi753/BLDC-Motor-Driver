#include "main.h"
#include "bldc.h"
#include "math.h"
void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint64_t stm32_cycles64(void)
{
    static uint64_t high = 0;
    static uint32_t last = 0;

    uint32_t now = DWT->CYCCNT;

    if (now < last)
        high += 0x100000000ULL;

    last = now;

    return high | now;
}

// perhaps in the future we may explore the option of using 32-bit timer + overflow accumulation 
// for getting microsecond timestamps, but for now DWT cycle counter with 64-bit accumulation is 
// simpler to implement and has no risk of overflow in the long term (hundreds of years at 72 MHz)
uint64_t micros64(void)
{
    // Use HAL_RCC_GetHCLKFreq() to get the actual current hardware clock frequency.
    // This is safer than SystemCoreClock if the PLL has changed without updating the global variable.
    return stm32_cycles64() / (HAL_RCC_GetHCLKFreq() / 1000000ULL);
}

uint32_t millis32(void)
{
    return (uint32_t)(micros64() / 1000ULL);
}

// derive unique 128-bit ID from STM32 factory UID (96-bit) with deterministic padding
void get_device_id(uint8_t id[16])
{
    uint32_t *uid = (uint32_t *)UID_BASE;

    // STM32 factory UID is 96-bit (3 words)
    uint32_t uid0 = uid[0];
    uint32_t uid1 = uid[1];
    uint32_t uid2 = uid[2];

    // Pack into first 12 bytes (little-endian)
    id[0]  = (uint8_t)(uid0);
    id[1]  = (uint8_t)(uid0 >> 8);
    id[2]  = (uint8_t)(uid0 >> 16);
    id[3]  = (uint8_t)(uid0 >> 24);

    id[4]  = (uint8_t)(uid1);
    id[5]  = (uint8_t)(uid1 >> 8);
    id[6]  = (uint8_t)(uid1 >> 16);
    id[7]  = (uint8_t)(uid1 >> 24);

    id[8]  = (uint8_t)(uid2);
    id[9]  = (uint8_t)(uid2 >> 8);
    id[10] = (uint8_t)(uid2 >> 16);
    id[11] = (uint8_t)(uid2 >> 24);

    // Expand to 128-bit (deterministic padding)
    id[12] = 0xA5;
    id[13] = 0x5A;
    id[14] = 0xC3;
    id[15] = 0x3C;
}

uint32_t rand32(void)
{
    return bsp_rand32();
}

