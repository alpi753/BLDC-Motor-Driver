#include "bldc.h"
#include "cmsis_os.h"
#include "main.h"
#include "usb_device.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "nanocbor/nanocbor.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern osMessageQueueId_t usbQueueHandle;
extern USBD_HandleTypeDef hUsbDeviceFS;

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

void usb_msg_tx(usb_msg_t* msg, uint8_t* buf, uint16_t buf_size) {
        if (osMessageQueueGet(usbQueueHandle, msg, NULL, 50) == osOK)
        {
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
            {
                usb_send_blocking(buf, (uint16_t)len);
            }
        }
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

/* ---------------- MAIN THREAD ---------------- */

void UsbThread(void *argument)
{
    usb_msg_t msg;
		uint8_t buf[1024];

    while (1)
    {
        osDelay(10);
				usb_msg_tx(&msg, buf, sizeof(buf));
    }
}
