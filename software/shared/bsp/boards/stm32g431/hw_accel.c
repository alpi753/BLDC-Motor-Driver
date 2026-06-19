#include "bsp.h"

#if CONFIG_BLDC_HAS_HW_ACCEL

#include <math.h>
#include <stdbool.h>

extern CORDIC_HandleTypeDef hcordic;
extern FMAC_HandleTypeDef hfmac;
extern RNG_HandleTypeDef hrng;

#define CORDIC_Q31F        2147483648.0f
#define FMAC_TEMP_SCALE    256.0f
#define FMAC_IIR_ALPHA     IIR_FILTER_ALPHA

#define FMAC_COEFF_B_SIZE  1U
#define FMAC_COEFF_A_SIZE  1U
#define FMAC_INPUT_HEADROOM 1U
#define FMAC_OUTPUT_HEADROOM 1U

static int16_t fmac_coeff_b[FMAC_COEFF_B_SIZE];
static int16_t fmac_coeff_a[FMAC_COEFF_A_SIZE];
static bool fmac_iir_ready;

static int32_t cordic_float_to_q31(float value)
{
    if (value > 0.999999f) {
        value = 0.999999f;
    } else if (value < -1.0f) {
        value = -1.0f;
    }
    return (int32_t)(value * CORDIC_Q31F);
}

static float cordic_q31_to_float(int32_t value)
{
    return (float)value / CORDIC_Q31F;
}

static int16_t fmac_float_to_q15_scaled(float value)
{
    float normalized = value / FMAC_TEMP_SCALE;

    if (normalized > 0.999f) {
        normalized = 0.999f;
    } else if (normalized < -1.0f) {
        normalized = -1.0f;
    }

    return (int16_t)(normalized * 32767.0f);
}

static float fmac_q15_to_float_scaled(int16_t value)
{
    return ((float)value / 32767.0f) * FMAC_TEMP_SCALE;
}

static HAL_StatusTypeDef cordic_calculate(uint32_t function, int32_t in, int32_t *out)
{
    const CORDIC_ConfigTypeDef config = {
        .Function   = function,
        .Scale      = CORDIC_SCALE_0,
        .InSize     = CORDIC_INSIZE_32BITS,
        .OutSize    = CORDIC_OUTSIZE_32BITS,
        .NbWrite    = CORDIC_NBWRITE_1,
        .NbRead     = CORDIC_NBREAD_1,
        .Precision  = CORDIC_PRECISION_6CYCLES,
    };

    if (HAL_CORDIC_Configure(&hcordic, &config) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_CORDIC_Calculate(&hcordic, &in, out, 1, HAL_MAX_DELAY);
}

static float cordic_sin(float radians)
{
    while (radians > (float)M_PI) {
        radians -= 2.0f * (float)M_PI;
    }
    while (radians < -(float)M_PI) {
        radians += 2.0f * (float)M_PI;
    }

    const int32_t in = cordic_float_to_q31(radians / (float)M_PI);
    int32_t out = 0;

    if (cordic_calculate(CORDIC_FUNCTION_SINE, in, &out) != HAL_OK) {
        return sinf(radians);
    }

    return cordic_q31_to_float(out);
}

static float cordic_log_positive(float x)
{
    if (x < 1.0f) {
        const int32_t in = cordic_float_to_q31(x);
        int32_t out = 0;

        if (cordic_calculate(CORDIC_FUNCTION_NATURALLOG, in, &out) != HAL_OK) {
            return logf(x);
        }

        return cordic_q31_to_float(out);
    }

    return -cordic_log_positive(1.0f / x);
}

static void fmac_iir_configure(float alpha)
{
    FMAC_FilterConfigTypeDef config = {0};

    fmac_coeff_b[0] = (int16_t)(alpha * 32767.0f);
    fmac_coeff_a[0] = (int16_t)((alpha - 1.0f) * 32767.0f);

    config.InputBaseAddress  = FMAC_COEFF_B_SIZE + FMAC_COEFF_A_SIZE;
    config.InputBufferSize   = FMAC_COEFF_B_SIZE + FMAC_INPUT_HEADROOM;
    config.InputThreshold    = FMAC_THRESHOLD_1;
    config.CoeffBaseAddress  = 0U;
    config.CoeffBufferSize   = FMAC_COEFF_B_SIZE + FMAC_COEFF_A_SIZE;
    config.OutputBaseAddress = config.InputBaseAddress + config.InputBufferSize;
    config.OutputBufferSize  = FMAC_COEFF_A_SIZE + FMAC_OUTPUT_HEADROOM;
    config.OutputThreshold   = FMAC_THRESHOLD_1;
    config.pCoeffA           = fmac_coeff_a;
    config.CoeffASize        = FMAC_COEFF_A_SIZE;
    config.pCoeffB           = fmac_coeff_b;
    config.CoeffBSize        = FMAC_COEFF_B_SIZE;
    config.Filter            = FMAC_FUNC_IIR_DIRECT_FORM_1;
    config.InputAccess       = FMAC_BUFFER_ACCESS_POLLING;
    config.OutputAccess      = FMAC_BUFFER_ACCESS_POLLING;
    config.Clip              = FMAC_CLIP_ENABLED;
    config.P                 = FMAC_COEFF_B_SIZE;
    config.Q                 = FMAC_COEFF_A_SIZE;
    config.R                 = 0U;

    if (HAL_FMAC_FilterConfig(&hfmac, &config) != HAL_OK) {
        fmac_iir_ready = false;
        return;
    }

    fmac_iir_ready = true;
}

static float fmac_iir_soft(float prev, float sample)
{
    return (FMAC_IIR_ALPHA * sample) + ((1.0f - FMAC_IIR_ALPHA) * prev);
}

static float fmac_iir_step(float prev, float sample)
{
    int16_t input_q15 = fmac_float_to_q15_scaled(sample);
    int16_t y_state_q15 = fmac_float_to_q15_scaled(prev);
    int16_t output_q15 = 0;
    uint16_t output_count = 1U;

    if (HAL_FMAC_FilterStop(&hfmac) != HAL_OK) {
        return fmac_iir_soft(prev, sample);
    }

    if (HAL_FMAC_FilterPreload(&hfmac, &input_q15, 1U, &y_state_q15, 1U) != HAL_OK) {
        return fmac_iir_soft(prev, sample);
    }

    if (HAL_FMAC_FilterStart(&hfmac, &output_q15, &output_count) != HAL_OK) {
        return fmac_iir_soft(prev, sample);
    }

    if (HAL_FMAC_PollFilterData(&hfmac, 10U) != HAL_OK) {
        return fmac_iir_soft(prev, sample);
    }

    return fmac_q15_to_float_scaled(output_q15);
}

void hw_accel_init(void)
{
    fmac_iir_configure(FMAC_IIR_ALPHA);
}

void hw_accel_sin_f(float radians, float *out)
{
    *out = cordic_sin(radians);
}

void hw_accel_log_f(float x, float *out)
{
    *out = cordic_log_positive(x);
}

void hw_accel_iir_lowpass_f(float prev, float sample, float alpha, float *out)
{
    (void)alpha;

    if (!fmac_iir_ready) {
        *out = (FMAC_IIR_ALPHA * sample) + ((1.0f - FMAC_IIR_ALPHA) * prev);
        return;
    }

    *out = fmac_iir_step(prev, sample);
}

uint32_t hw_accel_rand32(void)
{
    uint32_t value = 0U;

    if (HAL_RNG_GenerateRandomNumber(&hrng, &value) == HAL_OK) {
        return value;
    }

    return HAL_GetTick() ^ (uint32_t)micros64();
}

#endif /* CONFIG_BLDC_HAS_HW_ACCEL */