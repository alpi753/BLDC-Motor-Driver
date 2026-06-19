#include "bsp.h"
#include "main.h"

extern TIM_HandleTypeDef htim1;
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

static BLDC_Handle_t motor_handle;

#if CONFIG_BLDC_PWM_TIMER_HIGH != 1
#error "stm32g431 board.c: CONFIG_BLDC_PWM_TIMER_HIGH must be 1 (htim1)"
#endif

#if !BLDC_TELEM_USE_DEMO
static int g431_read_phase_currents(uint16_t *phase_a, uint16_t *phase_b, uint16_t *phase_c)
{
    if (HAL_ADCEx_InjectedPollForConversion(&hadc1, 1) != HAL_OK) {
        return 0;
    }

    *phase_a = (uint16_t)HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
    *phase_b = (uint16_t)HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_2);
    *phase_c = (uint16_t)HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_3);
    return 1;
}

static int g431_read_slow_sensors(uint16_t slow[3])
{
    if (HAL_ADC_Start(&hadc2) != HAL_OK) {
        return 0;
    }

    for (unsigned i = 0U; i < 3U; i++) {
        if (HAL_ADC_PollForConversion(&hadc2, 2) != HAL_OK) {
            (void)HAL_ADC_Stop(&hadc2);
            return 0;
        }
        slow[i] = (uint16_t)HAL_ADC_GetValue(&hadc2);
    }

    (void)HAL_ADC_Stop(&hadc2);
    return 1;
}
#endif

#if CONFIG_FOC_ENABLE && !BLDC_TELEM_USE_DEMO

static uint8_t g431_foc_adc_hw_active;

typedef struct {
    volatile uint16_t phase_a;
    volatile uint16_t phase_b;
    volatile uint16_t phase_c;
    volatile uint8_t ready;
} g431_foc_adc_cache_t;

static g431_foc_adc_cache_t g431_foc_adc_cache;
static volatile float g431_foc_vbus_v;
static uint32_t g431_foc_adc_decim;
static uint32_t g431_foc_adc_decim_count;
static uint32_t g431_foc_vbus_divider;
static void (*g431_foc_loop_notify_isr)(void);

static uint32_t g431_tim1_clock_hz(void)
{
    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();
    uint32_t ppre2 = (RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos;

    if (ppre2 >= 4U) {
        return pclk2 * 2U;
    }

    return pclk2;
}

static uint32_t g431_tim1_pwm_hz(void)
{
    const uint32_t timclk = g431_tim1_clock_hz();
    const uint32_t psc = (uint32_t)htim1.Instance->PSC + 1U;
    const uint32_t arr = (uint32_t)__HAL_TIM_GET_AUTORELOAD(&htim1) + 1U;

    if (psc == 0U || arr == 0U || timclk == 0U) {
        return 0U;
    }

    return timclk / (psc * arr);
}

static void g431_foc_refresh_vbus_cache(void)
{
    uint16_t slow[3];

    if (!g431_read_slow_sensors(slow)) {
        return;
    }

    g431_foc_vbus_v =
        ADC_TO_VOLT(slow[ADC_SLOW_RANK_VBUS]) * BUS_VOLTAGE_DIVIDER_RATIO;
}

int bsp_foc_loop_uses_hw_adc_trigger(void)
{
    return 1;
}

void bsp_foc_loop_hw_init(void (*notify_from_isr)(void))
{
    const uint32_t pwm_hz = g431_tim1_pwm_hz();
    uint32_t decim = 1U;

    g431_foc_loop_notify_isr = notify_from_isr;
    g431_foc_adc_cache.ready = 0U;
    g431_foc_adc_decim_count = 0U;
    g431_foc_vbus_divider = 0U;
    g431_foc_refresh_vbus_cache();

    if (pwm_hz > 0U && (uint32_t)CONFIG_FOC_LOOP_HZ > 0U) {
        decim = (pwm_hz + ((uint32_t)CONFIG_FOC_LOOP_HZ / 2U)) /
                (uint32_t)CONFIG_FOC_LOOP_HZ;
        if (decim == 0U) {
            decim = 1U;
        }
    }

    g431_foc_adc_decim = decim;

    (void)HAL_ADCEx_InjectedStop_IT(&hadc1);
    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4) != HAL_OK ||
        HAL_ADCEx_InjectedStart_IT(&hadc1) != HAL_OK) {
        Error_Handler();
    }

    HAL_NVIC_SetPriority(ADC1_2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
    g431_foc_adc_hw_active = 1U;
}

void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance != ADC1 || !g431_foc_adc_hw_active) {
        return;
    }

    g431_foc_adc_cache.phase_a =
        (uint16_t)HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
    g431_foc_adc_cache.phase_b =
        (uint16_t)HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_2);
    g431_foc_adc_cache.phase_c =
        (uint16_t)HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_3);
    g431_foc_adc_cache.ready = 1U;

    g431_foc_adc_decim_count++;
    if ((g431_foc_adc_decim_count % g431_foc_adc_decim) != 0U) {
        return;
    }

    if (g431_foc_loop_notify_isr != NULL) {
        g431_foc_loop_notify_isr();
    }
}

void ADC1_2_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&hadc1);
}

int bsp_foc_sample_sensors(float *ia, float *ib, float *ic, float *vbus)
{
    if (ia == NULL || ib == NULL || ic == NULL || vbus == NULL) {
        return 0;
    }

    if (!g431_foc_adc_cache.ready) {
        return 0;
    }

    g431_foc_vbus_divider++;
    if (g431_foc_vbus_divider >= (uint32_t)CONFIG_FOC_LOOP_HZ) {
        g431_foc_vbus_divider = 0U;
        g431_foc_refresh_vbus_cache();
    }

    *ia = ADC_TO_CURR(g431_foc_adc_cache.phase_a);
    *ib = ADC_TO_CURR(g431_foc_adc_cache.phase_b);
    *ic = ADC_TO_CURR(g431_foc_adc_cache.phase_c);
#if ADC_HAS_VBUS_ADC
    *vbus = g431_foc_vbus_v;
#else
    *vbus = 0.0f;
#endif
    return 1;
}

#elif CONFIG_FOC_ENABLE

int bsp_foc_loop_uses_hw_adc_trigger(void)
{
    return 0;
}

void bsp_foc_loop_hw_init(void (*notify_from_isr)(void))
{
    (void)notify_from_isr;
}

#endif /* CONFIG_FOC_ENABLE && !BLDC_TELEM_USE_DEMO */

void bsp_board_init(void)
{
    motor_handle = (BLDC_Handle_t){
        .htim_high = &htim1,
        .htim_low  = NULL,
        .htim_aux  = NULL,
        .chA       = BLDC_PHASE1_PWM_CH,
        .chB       = BLDC_PHASE2_PWM_CH,
        .chC       = BLDC_PHASE3_PWM_CH,
        .aux_chA   = 0U,
        .aux_chB   = 0U,
        .aux_chC   = 0U,
        .hadc      = &hadc1,
    };
}

BLDC_Handle_t *bsp_board_get_motor_handle(void)
{
    return &motor_handle;
}

void bsp_usb_init(void)
{
#if CONFIG_BLDC_HAS_USB_TELEM
    extern void MX_USB_Device_Init(void);
    MX_USB_Device_Init();
#endif
}

#if !BLDC_TELEM_USE_DEMO
int bsp_telem_adc_init(void)
{
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK) {
        return -1;
    }
    if (HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED) != HAL_OK) {
        return -1;
    }
    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4) != HAL_OK) {
        return -1;
    }
#if CONFIG_FOC_ENABLE
    if (HAL_ADCEx_InjectedStart_IT(&hadc1) != HAL_OK) {
        return -1;
    }
    HAL_NVIC_SetPriority(ADC1_2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
#else
    if (HAL_ADCEx_InjectedStart(&hadc1) != HAL_OK) {
        return -1;
    }
#endif
    return 0;
}

void bsp_telem_adc_conv_cplt(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
}

int bsp_telem_adc_snapshot(uint16_t *samples, unsigned count)
{
    uint16_t slow[3];

    if (samples == NULL || count != ADC_CHANNEL_COUNT) {
        return 0;
    }

#if CONFIG_FOC_ENABLE
    if (g431_foc_adc_hw_active && g431_foc_adc_cache.ready) {
        samples[ADC_IDX_PHASE_A] = g431_foc_adc_cache.phase_a;
        samples[ADC_IDX_PHASE_B] = g431_foc_adc_cache.phase_b;
        samples[ADC_IDX_PHASE_C] = g431_foc_adc_cache.phase_c;
    } else
#endif
    if (!g431_read_phase_currents(&samples[ADC_IDX_PHASE_A],
                                  &samples[ADC_IDX_PHASE_B],
                                  &samples[ADC_IDX_PHASE_C])) {
        return 0;
    }

    if (!g431_read_slow_sensors(slow)) {
        return 0;
    }

    samples[ADC_IDX_VBUS] = slow[ADC_SLOW_RANK_VBUS];
    samples[ADC_IDX_TEMP] = slow[ADC_SLOW_RANK_NTC_FET];
    return 1;
}
#endif

#if CONFIG_FOC_ENABLE && BLDC_TELEM_USE_DEMO

int bsp_foc_loop_uses_hw_adc_trigger(void)
{
    return 0;
}

void bsp_foc_loop_hw_init(void (*notify_from_isr)(void))
{
    (void)notify_from_isr;
}

int bsp_foc_sample_sensors(float *ia, float *ib, float *ic, float *vbus)
{
    (void)ia;
    (void)ib;
    (void)ic;
    (void)vbus;
    return 0;
}

#endif