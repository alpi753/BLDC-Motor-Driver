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
static void g431_trigger_injected(void)
{
    /* CubeMX routes ADC1 injected trigger to EXTI line 15 (PC15). */
    EXTI->SWIER1 = (1UL << 15);
}

static int g431_read_phase_currents(uint16_t *phase_a, uint16_t *phase_b, uint16_t *phase_c)
{
    g431_trigger_injected();
    if (HAL_ADCEx_InjectedPollForConversion(&hadc1, 2) != HAL_OK) {
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
    if (HAL_ADCEx_InjectedStart(&hadc1) != HAL_OK) {
        return -1;
    }
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