#include <stdint.h>
#include <string.h>
#include "cmsis_os2.h"
#include "bldc.h"
#include "usb_device.h"
#include "main.h"
#include <math.h>

extern osMessageQueueId_t usbQueueHandle;
extern BLDC_Handle_t bldc_h;
static bldc_telemetry_t telem_data;

static bldc_settings_t settings_data;


#define ADC_DMA_CHANNELS 5

static uint16_t adc_dma_buf[ADC_DMA_CHANNELS];
static volatile uint8_t adc_dma_ready = 0;
int bldc_adc_dma_start(void)
{
		adc_dma_ready = 0;
    // Start ADC in DMA mode 
    return HAL_ADC_Start_DMA(&bldc_h.hadc,
                          (uint32_t*)adc_dma_buf,
                          ADC_DMA_CHANNELS);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc == &bldc_h.hadc)
    {
        adc_dma_ready = 1;
    }
}

#ifndef BLDC_TELEM_USE_DEMO
static int bldc_telem_adc_dma_read(uint16_t *out_buf)
{
		if (!adc_dma_ready)
				return 0;

		memcpy(out_buf, adc_dma_buf, sizeof(uint16_t) * ADC_DMA_CHANNELS);
		adc_dma_ready = 0;
		return 1;
}

static void bldc_telem_update(void)
{
    uint16_t adc[ADC_DMA_CHANNELS];

    /* bldc_telem_adc_dma_read returns 1 on success, 0 on failure */
    if (!bldc_telem_adc_dma_read(adc)) return;


    telem_data.current_phase_a = ADC_TO_CURR(adc[0]);
    telem_data.current_phase_b = ADC_TO_CURR(adc[1]);
    telem_data.current_phase_c = ADC_TO_CURR(adc[2]);

    telem_data.battery_voltage = ADC_TO_VOLT(adc[3]) * BUS_VOLTAGE_DIVIDER_RATIO;
    telem_data.battery_current =
        (fabsf(telem_data.current_phase_a) +
         fabsf(telem_data.current_phase_b) +
         fabsf(telem_data.current_phase_c)) / 3.0f;

    /* Temperature (NTC thermistor conversion using pull-up divider and Beta equation) */
		// TODO: should be calibrated with thermistor curve, and scaled nonlinearly if needed.
    {
      const float v = ADC_TO_VOLT(adc[4]);
      const float vref = ADC_REF_VOLT;
      const float min_v = 0.005f;

      if (v > min_v && v < ADC_REF_VOLT) {
        float r_ntc = THERMISTOR_PULLUP * (v / (vref - v));
        float ratio = r_ntc / THERMISTOR_R25;
				CLAMP(ratio, 1e-6f, 1e6f);

        const float t0_k = 298.15f; // 25C in Kelvin
        const float inv_t = (1.0f / t0_k) + (1.0f / THERMISTOR_BETA) * logf(ratio);
        const float t_k = 1.0f / inv_t;
        const float temp_c_new = t_k - 273.15f;

        if (telem_data.temp_c == 0.0f) {
          telem_data.temp_c = temp_c_new;
        } else {
          IIR_FILTER(telem_data.temp_c, temp_c_new);
        }
      }
    }

    const float period = ((float)(__HAL_TIM_GET_AUTORELOAD(bldc_h.htim) + 1.0f) + 1); // period = arr + 1, because STM32 timers are inclusive-counting devices.
    if (period > 1.0f)
    {
      // duty = CCR / (ARR + 1)
      float duty_a = (float)__HAL_TIM_GET_COMPARE(bldc_h.htim, TIM_CHANNEL_1) / period;
      float duty_b = (float)__HAL_TIM_GET_COMPARE(bldc_h.htim, TIM_CHANNEL_2) / period;
      float duty_c = (float)__HAL_TIM_GET_COMPARE(bldc_h.htim, TIM_CHANNEL_3) / period;

      duty_a = fminf(fmaxf(duty_a, 0.0f), 1.0f);
      duty_b = fminf(fmaxf(duty_b, 0.0f), 1.0f);
      duty_c = fminf(fmaxf(duty_c, 0.0f), 1.0f);

      float vbus = telem_data.battery_voltage;
      // TODO: for better observer accuracy and removal of common mode noise, must be offset by the actual bus voltage.
      telem_data.voltage_phase_a = duty_a * vbus;
      telem_data.voltage_phase_b = duty_b * vbus;
      telem_data.voltage_phase_c = duty_c * vbus;
    }

    {
        uint32_t last_ms = telem_data.timestamp_ms;
        uint32_t now_ms = millis32();
        float dt_h = (last_ms == 0U) ? 0.0f : ((float)(now_ms - last_ms) / 3600000.0f);
        telem_data.energy_used_wh += telem_data.battery_voltage * telem_data.battery_current * dt_h;
        telem_data.energy_rem_wh = 0.0f;
    }

    telem_data.timestamp_ms = millis32();
    /* The remaining fields require rotor position/speed observers not wired yet. */
    telem_data.rpm_actual = 0.0f;
    telem_data.rpm_target = 0.0f;
    telem_data.i_d = 0.0f;
    telem_data.i_q = 0.0f;
    telem_data.angle_mechanical = 0.0f;
    telem_data.angle_electrical = 0.0f;

    telem_data.bemf_strength = 0U;
    telem_data.obs_confidence = 100U;
    telem_data.pll_lock_status = 0U;
    telem_data.angle_error_deg = 0U;
}
#endif



void bldc_telem_init(void) {
    // Initialize transport used by telemetry publisher
  MX_USB_DEVICE_Init();
	bldc_adc_dma_start();
	telem_data.temp_c = 0.0f; 
}

static usb_msg_t msg;
void bldc_telem_pub(void) {
#ifndef BLDC_TELEM_USE_DEMO
    bldc_telem_update();
#else
    gen_demo_telemetry(&telem_data);
#endif
		msg.type = USB_MSG_TELEMETRY;
		memcpy(&msg.data.telemetry, &telem_data, sizeof(bldc_telemetry_t));

		// Send to USB queue
		if (usbQueueHandle != NULL) 
				osMessageQueuePut(usbQueueHandle, &msg, 0, 10);
}

// Global exposed getter for the settings structure for updates
bldc_settings_t* bldc_get_settings(void) {
    return &settings_data;
}

void TelemThread(void *argument) {
		for (;;) {
				bldc_telem_pub();
        osDelay(10); // 10ms before CAN publish
				bldc_dronecan_pub();
				osDelay(90); // 100ms delay for 10Hz update rate
		}

}
