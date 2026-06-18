#include "bsp_math.h"
#include "bsp_autoconf.h"
#include <math.h>

#if CONFIG_BLDC_HAS_HW_ACCEL
void hw_accel_sin_f(float radians, float *out);
void hw_accel_log_f(float x, float *out);
void hw_accel_iir_lowpass_f(float prev, float sample, float alpha, float *out);
uint32_t hw_accel_rand32(void);
void hw_accel_init(void);
#endif

void bsp_hw_accel_init(void)
{
#if CONFIG_BLDC_HAS_HW_ACCEL
    hw_accel_init();
#endif
}

float bsp_sin_f(float radians)
{
#if CONFIG_BLDC_HAS_HW_ACCEL
    float out;
    hw_accel_sin_f(radians, &out);
    return out;
#else
    return sinf(radians);
#endif
}

float bsp_log_f(float x)
{
    if (x <= 0.0f) {
        return -INFINITY;
    }

#if CONFIG_BLDC_HAS_HW_ACCEL
    float out;
    hw_accel_log_f(x, &out);
    return out;
#else
    return logf(x);
#endif
}

float bsp_iir_lowpass_f(float prev, float sample, float alpha)
{
#if CONFIG_BLDC_HAS_HW_ACCEL
    float out;
    hw_accel_iir_lowpass_f(prev, sample, alpha, &out);
    return out;
#else
    return (alpha * sample) + ((1.0f - alpha) * prev);
#endif
}


#if !CONFIG_BLDC_HAS_HW_ACCEL
static uint32_t xorshift32_state = 0x12345678U;
static uint32_t soft_rand32(void)
{
    uint32_t x = xorshift32_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    xorshift32_state = x;
    return x;
}
#endif

uint32_t bsp_rand32(void)
{
#if CONFIG_BLDC_HAS_HW_ACCEL
    return hw_accel_rand32();
#else
    return soft_rand32();
#endif
}