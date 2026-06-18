
#include <stdint.h>
#include <string.h>
#include "cmsis_os.h"
#include "bldc.h"
#include "main.h"
#include "dronecan.c"
#include "usb_device.h"
#include <math.h>
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "nanocbor/nanocbor.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern USBD_HandleTypeDef hUsbDeviceFS;



extern BLDC_Handle_t bldc_h;
static bldc_telemetry_t telem_data;

static bldc_settings_t settings_data;


static uint16_t adc_dma_buf[ADC_CHANNEL_COUNT];
static volatile uint8_t adc_dma_ready = 0;
int bldc_adc_dma_start(void)
{
		adc_dma_ready = 0;
    // Start ADC in DMA mode 
    return HAL_ADC_Start_DMA(bldc_h.hadc,
                          (uint32_t*)adc_dma_buf,
                          ADC_CHANNEL_COUNT);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc == bldc_h.hadc)
    {
        adc_dma_ready = 1;
    }
}

#if !BLDC_TELEM_USE_DEMO
static int bldc_telem_adc_dma_read(uint16_t *out_buf)
{
		if (!adc_dma_ready)
				return 0;

		memcpy(out_buf, adc_dma_buf, sizeof(uint16_t) * ADC_CHANNEL_COUNT);
		adc_dma_ready = 0;
		return 1;
}

void bldc_telem_update(void)
{
    uint16_t adc[ADC_CHANNEL_COUNT];

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
        const float inv_t = (1.0f / t0_k) + (1.0f / THERMISTOR_BETA) * bsp_log_f(ratio);
        const float t_k = 1.0f / inv_t;
        const float temp_c_new = t_k - 273.15f;

        if (telem_data.temp_c == 0.0f) {
          telem_data.temp_c = temp_c_new;
        } else {
          telem_data.temp_c = bsp_iir_lowpass_f(telem_data.temp_c, temp_c_new, IIR_FILTER_ALPHA);
        }
      }
    }

    const float period = (float)(__HAL_TIM_GET_AUTORELOAD(bldc_h.htim_high) + 1U);
    if (period > 1.0f)
    {
      // duty = CCR / (ARR + 1)
      float duty_a = (float)__HAL_TIM_GET_COMPARE(bldc_h.htim_high, bldc_h.chA) / period;
      float duty_b = (float)__HAL_TIM_GET_COMPARE(bldc_h.htim_high, bldc_h.chB) / period;
      float duty_c = (float)__HAL_TIM_GET_COMPARE(bldc_h.htim_high, bldc_h.chC) / period;

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
#else
static float fake_rand_f(float lo, float hi)
{
    return lo + (hi - lo) * ((float)bsp_rand32() / (float)UINT32_MAX);
}

void bldc_telem_fake(void)
{
    telem_data.rpm_actual = fake_rand_f(800.0f, 3200.0f);
    telem_data.rpm_target = fake_rand_f(1000.0f, 3000.0f);

    telem_data.current_phase_a = fake_rand_f(-4.0f, 4.0f);
    telem_data.current_phase_b = fake_rand_f(-4.0f, 4.0f);
    telem_data.current_phase_c = fake_rand_f(-4.0f, 4.0f);

    telem_data.voltage_phase_a = fake_rand_f(0.0f, 24.0f);
    telem_data.voltage_phase_b = fake_rand_f(0.0f, 24.0f);
    telem_data.voltage_phase_c = fake_rand_f(0.0f, 24.0f);

    telem_data.i_d = fake_rand_f(-2.0f, 2.0f);
    telem_data.i_q = fake_rand_f(0.0f, 5.0f);

    telem_data.angle_mechanical = fake_rand_f(0.0f, 360.0f);
    telem_data.angle_electrical = fake_rand_f(0.0f, 360.0f);

    telem_data.battery_voltage = fake_rand_f(42.0f, 50.4f);
    telem_data.battery_current = fake_rand_f(0.0f, 15.0f);

    telem_data.energy_used_wh = fake_rand_f(0.0f, 50.0f);
    telem_data.energy_rem_wh = fake_rand_f(50.0f, 100.0f);

    telem_data.bemf_strength = (uint8_t)(bsp_rand32() & 0xFFU);
    telem_data.obs_confidence = (uint8_t)(bsp_rand32() % 101U);
    telem_data.pll_lock_status = (uint8_t)(bsp_rand32() & 1U);
    telem_data.angle_error_deg = (uint8_t)(bsp_rand32() % 31U);
    telem_data.temp_c = fake_rand_f(25.0f, 85.0f);
    telem_data.timestamp_ms = millis32();
}
#endif


void bldc_telem_init(void) {
#if !BLDC_TELEM_USE_DEMO
  bsp_usb_init();
	bldc_adc_dma_start();
#endif
	telem_data.temp_c = 0.0f; 
}

void bldc_telem_fetch(usb_msg_t *msg) {
#if BLDC_TELEM_USE_DEMO
    bldc_telem_fake();
#else
	bldc_telem_update();
#endif
	if (msg != NULL) {
		msg->type = USB_MSG_TELEMETRY;
		memcpy(&msg->data.telemetry, &telem_data, sizeof(bldc_telemetry_t));
	}
}

bldc_settings_t* bldc_get_settings(void) {
    return &settings_data;
}

/* ---------------- USB SEND (robust) ---------------- */

static uint8_t usb_send_blocking(uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0)
        return USBD_FAIL;

    for (uint32_t retry = 0; retry < 50; retry++)
    {
        if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED ||
            hUsbDeviceFS.pClassData == NULL)
        {
            osDelay(10);
            continue;
        }

        uint8_t status = CDC_Transmit_FS(data, len);

        if (status == USBD_OK)
            return USBD_OK;

        if (status != USBD_BUSY)
            return status;

        osDelay(2);
    }

    return USBD_BUSY;
}

/* ---------------- TELEMETRY ENCODE ---------------- */

static int usb_telem_encode(nanocbor_encoder_t* enc, usb_msg_t msg)
{
    nanocbor_fmt_array(enc, 2);

    nanocbor_fmt_uint(enc, msg.type);

    nanocbor_fmt_map(enc, 22);

    nanocbor_put_tstr(enc, "rpm");
    nanocbor_fmt_float(enc, msg.data.telemetry.rpm_actual);

    nanocbor_put_tstr(enc, "rpm_t");
    nanocbor_fmt_float(enc, msg.data.telemetry.rpm_target);

    nanocbor_put_tstr(enc, "i_a");
    nanocbor_fmt_float(enc, msg.data.telemetry.current_phase_a);

    nanocbor_put_tstr(enc, "i_b");
    nanocbor_fmt_float(enc, msg.data.telemetry.current_phase_b);

    nanocbor_put_tstr(enc, "i_c");
    nanocbor_fmt_float(enc, msg.data.telemetry.current_phase_c);

    nanocbor_put_tstr(enc, "v_a");
    nanocbor_fmt_float(enc, msg.data.telemetry.voltage_phase_a);

    nanocbor_put_tstr(enc, "v_b");
    nanocbor_fmt_float(enc, msg.data.telemetry.voltage_phase_b);

    nanocbor_put_tstr(enc, "v_c");
    nanocbor_fmt_float(enc, msg.data.telemetry.voltage_phase_c);

    nanocbor_put_tstr(enc, "i_d");
    nanocbor_fmt_float(enc, msg.data.telemetry.i_d);

    nanocbor_put_tstr(enc, "i_q");
    nanocbor_fmt_float(enc, msg.data.telemetry.i_q);

    nanocbor_put_tstr(enc, "ang_m");
    nanocbor_fmt_float(enc, msg.data.telemetry.angle_mechanical);

    nanocbor_put_tstr(enc, "ang_e");
    nanocbor_fmt_float(enc, msg.data.telemetry.angle_electrical);

    nanocbor_put_tstr(enc, "ts");
    nanocbor_fmt_uint(enc, msg.data.telemetry.timestamp_ms);

    nanocbor_put_tstr(enc, "v_bat");
    nanocbor_fmt_float(enc, msg.data.telemetry.battery_voltage);

    nanocbor_put_tstr(enc, "i_bat");
    nanocbor_fmt_float(enc, msg.data.telemetry.battery_current);

    nanocbor_put_tstr(enc, "e_used");
    nanocbor_fmt_float(enc, msg.data.telemetry.energy_used_wh);

    nanocbor_put_tstr(enc, "e_rem");
    nanocbor_fmt_float(enc, msg.data.telemetry.energy_rem_wh);

    nanocbor_put_tstr(enc, "bemf");
    nanocbor_fmt_uint(enc, msg.data.telemetry.bemf_strength);

    nanocbor_put_tstr(enc, "obs");
    nanocbor_fmt_uint(enc, msg.data.telemetry.obs_confidence);

    nanocbor_put_tstr(enc, "pll");
    nanocbor_fmt_uint(enc, msg.data.telemetry.pll_lock_status);

    nanocbor_put_tstr(enc, "ang_err");
    nanocbor_fmt_uint(enc, msg.data.telemetry.angle_error_deg);

		nanocbor_put_tstr(enc, "temp");
		nanocbor_fmt_float(enc, msg.data.telemetry.temp_c);

    return nanocbor_encoded_len(enc);
}

/* ---------------- SETTINGS ENCODE ---------------- */

static int settings_encode(nanocbor_encoder_t* enc, usb_msg_t msg)
{
    nanocbor_fmt_array(enc, 2);
    nanocbor_fmt_uint(enc, USB_MSG_SETTINGS);

    nanocbor_fmt_map(enc, 22);

    nanocbor_put_tstr(enc, "pp"); nanocbor_fmt_float(enc, msg.data.settings.pole_pairs);
    nanocbor_put_tstr(enc, "kv"); nanocbor_fmt_float(enc, msg.data.settings.motor_kv);
    nanocbor_put_tstr(enc, "rs"); nanocbor_fmt_float(enc, msg.data.settings.phase_resistance);
    nanocbor_put_tstr(enc, "ls"); nanocbor_fmt_float(enc, msg.data.settings.phase_inductance);

    nanocbor_put_tstr(enc, "i_kp"); nanocbor_fmt_float(enc, msg.data.settings.current_kp);
    nanocbor_put_tstr(enc, "i_ki"); nanocbor_fmt_float(enc, msg.data.settings.current_ki);
    nanocbor_put_tstr(enc, "s_kp"); nanocbor_fmt_float(enc, msg.data.settings.speed_kp);
    nanocbor_put_tstr(enc, "s_ki"); nanocbor_fmt_float(enc, msg.data.settings.speed_ki);
    nanocbor_put_tstr(enc, "idt"); nanocbor_fmt_float(enc, msg.data.settings.i_d_target);

    nanocbor_put_tstr(enc, "p_kp"); nanocbor_fmt_float(enc, msg.data.settings.pll_kp);
    nanocbor_put_tstr(enc, "p_ki"); nanocbor_fmt_float(enc, msg.data.settings.pll_ki);
    nanocbor_put_tstr(enc, "bemf"); nanocbor_fmt_float(enc, msg.data.settings.bemf_filter_cutoff_hz);
    nanocbor_put_tstr(enc, "obs"); nanocbor_fmt_float(enc, msg.data.settings.observer_gain);
    nanocbor_put_tstr(enc, "min_cl"); nanocbor_fmt_float(enc, msg.data.settings.min_rpm_closed_loop);
    nanocbor_put_tstr(enc, "max_ol"); nanocbor_fmt_float(enc, msg.data.settings.max_rpm_open_loop);

    nanocbor_put_tstr(enc, "ramp"); nanocbor_fmt_float(enc, msg.data.settings.startup_ramp_time_ms);
    nanocbor_put_tstr(enc, "align"); nanocbor_fmt_float(enc, msg.data.settings.alignment_current);
    nanocbor_put_tstr(enc, "smode"); nanocbor_fmt_uint(enc, msg.data.settings.startup_mode);

    nanocbor_put_tstr(enc, "l_i"); nanocbor_fmt_float(enc, msg.data.settings.max_phase_current);
    nanocbor_put_tstr(enc, "l_v"); nanocbor_fmt_float(enc, msg.data.settings.max_bus_voltage);
    nanocbor_put_tstr(enc, "l_t"); nanocbor_fmt_float(enc, msg.data.settings.max_temperature);
    nanocbor_put_tstr(enc, "l_cd"); nanocbor_fmt_float(enc, msg.data.settings.current_derating_start);

    return nanocbor_encoded_len(enc);
}

static int settings_decode(nanocbor_value_t* map, bldc_settings_t *settings) {
      const uint8_t *str;
      size_t str_len;
      if (nanocbor_get_tstr(map, &str, &str_len) < 0) {
          return 0; // Failed to get key
      }
			#define MATCH_STR(k) (str_len == sizeof(k)-1 && strncmp((const char*)str, k, str_len) == 0)
      float fval;
      uint32_t uval;
      if (MATCH_STR("pp")) { if (nanocbor_get_float(map, &fval) >= 0) settings->pole_pairs = fval; }
      else if (MATCH_STR("kv")) { if (nanocbor_get_float(map, &fval) >= 0) settings->motor_kv = fval; }
      else if (MATCH_STR("rs")) { if (nanocbor_get_float(map, &fval) >= 0) settings->phase_resistance = fval; }
      else if (MATCH_STR("ls")) { if (nanocbor_get_float(map, &fval) >= 0) settings->phase_inductance = fval; }
      else if (MATCH_STR("i_kp")) { if (nanocbor_get_float(map, &fval) >= 0) settings->current_kp = fval; }
      else if (MATCH_STR("i_ki")) { if (nanocbor_get_float(map, &fval) >= 0) settings->current_ki = fval; }
      else if (MATCH_STR("s_kp")) { if (nanocbor_get_float(map, &fval) >= 0) settings->speed_kp = fval; }
      else if (MATCH_STR("s_ki")) { if (nanocbor_get_float(map, &fval) >= 0) settings->speed_ki = fval; }
      else if (MATCH_STR("idt")) { if (nanocbor_get_float(map, &fval) >= 0) settings->i_d_target = fval; }
      else if (MATCH_STR("p_kp")) { if (nanocbor_get_float(map, &fval) >= 0) settings->pll_kp = fval; }
      else if (MATCH_STR("p_ki")) { if (nanocbor_get_float(map, &fval) >= 0) settings->pll_ki = fval; }
      else if (MATCH_STR("bemf")) { if (nanocbor_get_float(map, &fval) >= 0) settings->bemf_filter_cutoff_hz = fval; }
      else if (MATCH_STR("obs")) { if (nanocbor_get_float(map, &fval) >= 0) settings->observer_gain = fval; }
      else if (MATCH_STR("min_cl")) { if (nanocbor_get_float(map, &fval) >= 0) settings->min_rpm_closed_loop = fval; }
      else if (MATCH_STR("max_ol")) { if (nanocbor_get_float(map, &fval) >= 0) settings->max_rpm_open_loop = fval; }
      else if (MATCH_STR("ramp")) { if (nanocbor_get_float(map, &fval) >= 0) settings->startup_ramp_time_ms = fval; }
      else if (MATCH_STR("align")) { if (nanocbor_get_float(map, &fval) >= 0) settings->alignment_current = fval; }
      else if (MATCH_STR("smode")) { if (nanocbor_get_uint32(map, &uval) >= 0) settings->startup_mode = (uint8_t)uval; }
      else if (MATCH_STR("l_i")) { if (nanocbor_get_float(map, &fval) >= 0) settings->max_phase_current = fval; }
      else if (MATCH_STR("l_v")) { if (nanocbor_get_float(map, &fval) >= 0) settings->max_bus_voltage = fval; }
      else if (MATCH_STR("l_t")) { if (nanocbor_get_float(map, &fval) >= 0) settings->max_temperature = fval; }
      else if (MATCH_STR("l_cd")) { if (nanocbor_get_float(map, &fval) >= 0) settings->current_derating_start = fval; }
      else { nanocbor_skip(map);} // unknown key 
			return 1; // success
}

/* ---------------- DEBUG ENCODE ---------------- */

static int usb_debug_encode(nanocbor_encoder_t* enc, usb_msg_t msg)
{
    nanocbor_fmt_array(enc, 2);
    nanocbor_fmt_uint(enc, USB_MSG_DEBUG_STR);

    nanocbor_put_tstr(enc, msg.data.debug_str);

    return nanocbor_encoded_len(enc);
}

void usb_msg_tx(usb_msg_t* msg, uint8_t* buf, uint16_t buf_size)
{
    if (msg == NULL || buf == NULL || buf_size == 0)
        return;

    nanocbor_encoder_t enc;
    nanocbor_encoder_init(&enc, buf, buf_size);

    int len = 0;

    switch (msg->type)
    {
        case USB_MSG_TELEMETRY:
            len = usb_telem_encode(&enc, *msg);
            break;

        case USB_MSG_SETTINGS:
            len = settings_encode(&enc, *msg);
            break;

        case USB_MSG_DEBUG_STR:
            len = usb_debug_encode(&enc, *msg);
            break;

        case USB_MSG_ERROR:
            nanocbor_fmt_array(&enc, 2);
            nanocbor_fmt_uint(&enc, USB_MSG_ERROR);
            nanocbor_fmt_uint(&enc, msg->data.error_code);
            len = nanocbor_encoded_len(&enc);
            break;

        default:
            len = 0;
            break;
    }

    if (len > 0)
        usb_send_blocking(buf, (uint16_t)len);
}


void usb_msg_rx(uint8_t *buf, uint32_t *len) {
    if (buf == NULL || len == NULL || *len == 0) {
        return;
    }

    nanocbor_value_t dec;
    nanocbor_decoder_init(&dec, buf, *len);

    nanocbor_value_t array;
    if (nanocbor_enter_array(&dec, &array) < 0) return; // Not an array

    uint32_t msg_type;
    if (nanocbor_get_uint32(&array, &msg_type) < 0) return; // Failed to get message type

    if (msg_type == USB_MSG_SETTINGS) {
        nanocbor_value_t map;
        if (nanocbor_enter_map(&array, &map) >= 0) {
            bldc_settings_t *settings = bldc_get_settings();
            
            while (!nanocbor_at_end(&map)) {
							if (!settings_decode(&map, settings)) {
								break;
							}
            }
            #undef MATCH_STR
        }
    }
}


void TelemThread(void *argument) {
		usb_msg_t msg;
		uint8_t buf[1024];
		(void)argument;

		for (;;) {
				bldc_telem_fetch(&msg);
				usb_msg_tx(&msg, buf, sizeof(buf));
				osDelay(10);
				bldc_dronecan_pub();
				osDelay(90);
				osDelay(1000-100); // total 1s loop time
		}
}