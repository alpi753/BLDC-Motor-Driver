#ifndef __BLDC_H
#define __BLDC_H

#include <stdint.h>
#include "bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Portable math / filter helpers (board constants come from board.h)        */
/* -------------------------------------------------------------------------- */
#define ADC_TO_VOLT(x)            (((float)(x) / ADC_MAX_COUNT) * ADC_REF_VOLT)
#define ADC_TO_CURR(x)            ((ADC_TO_VOLT(x) - PHASE_CURRENT_ZERO_V) / PHASE_CURRENT_V_PER_A)

#define IIR_FILTER_ALPHA          0.1f
#define IIR_FILTER(prev, curr)    prev = ((IIR_FILTER_ALPHA * (curr)) + ((1.0f - IIR_FILTER_ALPHA) * (prev)))

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif
#define CLAMP(x, min_val, max_val) ((x) = MIN(MAX((x), (min_val)), (max_val)))

/* -------------------------------------------------------------------------- */
/* Type definitions                                                             */
/* -------------------------------------------------------------------------- */

typedef enum {
    PHASE_FLOAT = 0,
    PHASE_PWM_HIGH,
    PHASE_PWM_LOW
} PhaseState;

typedef struct {
    float pole_pairs;
    float motor_kv;
    float phase_resistance;
    float phase_inductance;
    float current_kp;
    float current_ki;
    float speed_kp;
    float speed_ki;
    float i_d_target;
    float pll_kp;
    float pll_ki;
    float bemf_filter_cutoff_hz;
    float observer_gain;
    float min_rpm_closed_loop;
    float max_rpm_open_loop;
    float startup_ramp_time_ms;
    float alignment_current;
    uint8_t startup_mode;
    float max_phase_current;
    float max_bus_voltage;
    float max_temperature;
    float current_derating_start;
} bldc_settings_t;

typedef struct {
    float rpm_actual;
    float rpm_target;
    float current_phase_a;
    float current_phase_b;
    float current_phase_c;
    float voltage_phase_a;
    float voltage_phase_b;
    float voltage_phase_c;
    float i_d;
    float i_q;
    float angle_mechanical;
    float angle_electrical;
    uint64_t timestamp_ms;
    float battery_voltage;
    float battery_current;
    float energy_used_wh;
    float energy_rem_wh;
    uint8_t bemf_strength;
    uint8_t obs_confidence;
    uint8_t pll_lock_status;
    uint8_t angle_error_deg;
    float temp_c;
} bldc_telemetry_t;

typedef enum {
    USB_MSG_TELEMETRY,
    USB_MSG_SETTINGS,
    USB_MSG_DEBUG_STR,
    USB_MSG_ERROR
} usb_msg_type_t;

typedef struct {
    usb_msg_type_t type;
    union {
        bldc_telemetry_t telemetry;
        bldc_settings_t settings;
        char debug_str[64];
        uint32_t error_code;
    } data;
} usb_msg_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                   */
/* -------------------------------------------------------------------------- */

void dwt_init(void);
uint32_t millis32(void);
uint64_t micros64(void);
void get_device_id(uint8_t id[16]);
uint32_t rand32(void);

void bldc_comm_init(BLDC_Handle_t *motor);
void bldc_comm_set_duty(uint16_t duty);
void bldc_comm_commutate(uint8_t step);
void bldc_comm_enable(void);
void bldc_comm_disable(void);

#if CONFIG_BLDC_HAS_DRONECAN
void bldc_dronecan_init(void);
void bldc_dronecan_update(void);
void bldc_dronecan_pub(void);
#endif

#if CONFIG_BLDC_HAS_DRV8323
void bldc_drv8323r_init(void);
uint16_t bldc_drv8323r_read_reg(uint8_t reg);
void bldc_drv8323r_write_reg(uint8_t reg, uint16_t data);
uint32_t bldc_drv8323r_read_faults(void);
void bldc_drv8323r_reset_faults(void);
void bldc_drv8323r_set_oc_adj(int val);
void bldc_drv8323r_set_oc_mode(int mode);
void bldc_drv8323r_set_current_amp_gain(int gain);
void bldc_drv8323r_dccal_on(void);
void bldc_drv8323r_dccal_off(void);
char *bldc_drv8323r_faults_to_string(uint32_t faults);
#endif

void bldc_telem_init(void);
void bldc_telem_pub(void);
bldc_settings_t *bldc_get_settings(void);

#if CONFIG_BLDC_HAS_USB_TELEM
void usb_msg_tx(usb_msg_t *msg, uint8_t *buf, uint16_t buf_size);
void usb_msg_rx(uint8_t *Buf, uint32_t *Len);
#endif

#if BLDC_TELEM_USE_DEMO
void gen_demo_telemetry(bldc_telemetry_t *telem_data);
#endif

extern BLDC_Handle_t bldc_h;

#ifdef __cplusplus
}
#endif

#endif /* __BLDC_H */