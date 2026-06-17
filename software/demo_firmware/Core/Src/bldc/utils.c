#include "main.h"
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

static uint32_t lfsr = 0x12345678;

static uint32_t xorshift32(void)
{
    uint32_t x = lfsr;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    lfsr = x;
    return x;
}

uint32_t rand32(void)
{
#if HAL_RNG_MODULE_ENABLED
    uint32_t val;

    if (HAL_RNG_GenerateRandomNumber(&hrng, &val) == HAL_OK)
    {
        return val;
    }
    // fallback if RNG fails at runtime
#endif

    return xorshift32();
}

#ifdef BLDC_TELEM_USE_DEMO
void gen_demo_telemetry(bldc_telemetry_t* telem_data)
{
    static float t = 0.0f;
    static float angle = 0.0f;
    static float energy_used = 0.0f;

    t += 0.01f;

    telem_data->rpm_actual = 1500.0f + 400.0f * sinf(t) + 120.0f * sinf(2.7f * t);
    telem_data->rpm_target = 1600.0f;

    {
        float i_amp = 0.8f + 0.3f * sinf(0.8f * t);
        telem_data->current_phase_a = 2.0f + i_amp * sinf(t * 1.5f);
        telem_data->current_phase_b = 2.0f + i_amp * sinf(t * 1.5f + 2.094f);
        telem_data->current_phase_c = 2.0f + i_amp * sinf(t * 1.5f + 4.188f);
    }

    {
        float v_amp = 0.5f + 0.1f * sinf(0.3f * t);
        telem_data->voltage_phase_a = 24.0f + v_amp * sinf(t * 0.3f);
        telem_data->voltage_phase_b = 24.0f + v_amp * sinf(t * 0.3f + 2.094f);
        telem_data->voltage_phase_c = 24.0f + v_amp * sinf(t * 0.3f + 4.188f);
    }

    telem_data->i_d = 0.3f * sinf(0.7f * t);
    telem_data->i_q = 1.2f + 0.5f * sinf(0.9f * t);

    angle += telem_data->rpm_actual * 0.001f;
    if (angle > 360.0f)
        angle -= 360.0f;

    telem_data->angle_mechanical = angle;
    telem_data->angle_electrical = fmodf(angle * 7.0f, 360.0f);

    telem_data->battery_voltage = 48.0f - 0.005f * t;
    telem_data->battery_current = 8.0f + 2.0f * sinf(0.5f * t);

    energy_used += (telem_data->battery_voltage * telem_data->battery_current) * 0.00001f;
    telem_data->energy_used_wh = energy_used;
    telem_data->energy_rem_wh = 100.0f - energy_used;

    telem_data->bemf_strength = (uint8_t)(fminf(255.0f, fabsf(0.01f * telem_data->rpm_actual * sinf(1.5f * t))));
    telem_data->obs_confidence = (uint8_t)((0.7f + 0.3f * sinf(0.4f * t)) * 100.0f);
    telem_data->pll_lock_status = (fabsf(sinf(0.8f * t)) > 0.5f) ? 1U : 0U;
    telem_data->angle_error_deg = (uint8_t)(fabsf(5.0f * sinf(1.1f * t)));
    telem_data->timestamp_ms = millis32();
}
#endif
