#include <string.h>

#include "bsp.h"
#include "bldc.h"
#include "main.h"

extern TIM_HandleTypeDef htim1;
extern ADC_HandleTypeDef hadc1;

static BLDC_Handle_t motor_handle;

#if CONFIG_BLDC_PWM_TIMER_HIGH != 1
#error "stm32f411 board.c: CONFIG_BLDC_PWM_TIMER_HIGH must be 1 (htim1)"
#endif

#if !BLDC_TELEM_USE_DEMO
static uint16_t adc_dma_buf[ADC_CHANNEL_COUNT];
static volatile uint32_t adc_dma_seq;
#endif

void bsp_board_init(void)
{
#if BLDC_PHASE1_LOW_USE_GPIO
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = BLDC_PHASE1_LOW_GPIO_Pin;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BLDC_PHASE1_LOW_GPIO_Port, &gpio);
    HAL_GPIO_WritePin(BLDC_PHASE1_LOW_GPIO_Port, BLDC_PHASE1_LOW_GPIO_Pin, GPIO_PIN_RESET);
#endif

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
    extern void MX_USB_DEVICE_Init(void);
    MX_USB_DEVICE_Init();
#endif
}

#if !BLDC_TELEM_USE_DEMO
int bsp_telem_adc_init(void)
{
    adc_dma_seq = 0U;
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buf, ADC_CHANNEL_COUNT) != HAL_OK) {
        return -1;
    }
    return 0;
}

void bsp_telem_adc_conv_cplt(ADC_HandleTypeDef *hadc)
{
    if (hadc == &hadc1) {
        adc_dma_seq++;
    }
}

int bsp_telem_adc_snapshot(uint16_t *samples, unsigned count)
{
    uint32_t seq_before;
    uint32_t seq_after;

    if (samples == NULL || count != ADC_CHANNEL_COUNT) {
        return 0;
    }

    do {
        seq_before = adc_dma_seq;
        memcpy(samples, adc_dma_buf, sizeof(uint16_t) * ADC_CHANNEL_COUNT);
        seq_after = adc_dma_seq;
    } while (seq_before != seq_after);

    return 1;
}
#endif

#if CONFIG_FOC_ENABLE && !BLDC_TELEM_USE_DEMO
int bsp_foc_sample_sensors(float *ia, float *ib, float *ic, float *vbus)
{
    uint16_t adc[ADC_CHANNEL_COUNT];

    if (ia == NULL || ib == NULL || ic == NULL || vbus == NULL) {
        return 0;
    }

    if (!bsp_telem_adc_snapshot(adc, ADC_CHANNEL_COUNT)) {
        return 0;
    }

    *ia = ADC_TO_CURR(adc[ADC_IDX_PHASE_A]);
    *ib = ADC_TO_CURR(adc[ADC_IDX_PHASE_B]);
    *ic = ADC_TO_CURR(adc[ADC_IDX_PHASE_C]);
#if ADC_HAS_VBUS_ADC
    *vbus = ADC_TO_VOLT(adc[ADC_IDX_VBUS]) * BUS_VOLTAGE_DIVIDER_RATIO;
#else
    *vbus = 0.0f;
#endif
    return 1;
}
#endif