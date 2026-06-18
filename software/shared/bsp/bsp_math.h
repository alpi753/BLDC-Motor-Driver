#ifndef BLDC_BSP_MATH_H
#define BLDC_BSP_MATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void bsp_hw_accel_init(void);

float bsp_sin_f(float radians);
float bsp_log_f(float x);
float bsp_iir_lowpass_f(float prev, float sample, float alpha);
uint32_t bsp_rand32(void);

#ifdef __cplusplus
}
#endif

#endif /* BLDC_BSP_MATH_H */