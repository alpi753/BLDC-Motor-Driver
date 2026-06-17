#ifndef __BLDC_H
#define __BLDC_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Hardware Configuration                                                       */
/* -------------------------------------------------------------------------- */
// Uncomment this if using advanced timers with complementary outputs (e.g., TIM1, TIM8)
// #define BLDC_COMPLEMENTARY_DRIVE
// #define BLDC_TELEM_USE_DEMO
#define ADC_MAX_COUNT             4095.0f
#define ADC_REF_VOLT              3.3f
#define PHASE_CURRENT_ZERO_V      1.65f
#define PHASE_CURRENT_V_PER_A     0.100f
#define BUS_VOLTAGE_DIVIDER_RATIO 11.0f // = (R_h + R_l) / R_l 
#define THERMISTOR_PULLUP        10000.0f
#define THERMISTOR_R25           10000.0f
#define THERMISTOR_BETA          3950.0f
#define ADC_CHANNEL_COUNT         5U
#define BATTERY_CAPACITY_WH       100.0f

/* -------------------------------------------------------------------------- */
/* mappings 						                                                      */
/* -------------------------------------------------------------------------- */
#define PHASE_1_CH TIM_CHANNEL_1
#define CH1E       TIM_CCER_CC1E
#define CH1NE      TIM_CCER_CC1NE

#define PHASE_2_CH TIM_CHANNEL_2
#define CH2E       TIM_CCER_CC2E
#define CH2NE      TIM_CCER_CC2NE

#define PHASE_3_CH TIM_CHANNEL_3
#define CH3E       TIM_CCER_CC3E
#define CH3NE      TIM_CCER_CC3NE


#define DRV8323R_CS_GPIO_Port	 		SPI1_CS_GPIO_Port
#define DRV8323R_CS_Pin 					SPI1_CS_Pin
#define DRV8323R_EN_GPIO_Port 		SPI1_EN_GPIO_Port
#define DRV8323R_EN_Pin 					SPI1_EN_Pin
#define DRV8323R_FAULT_GPIO_Port	SPI1_FAULT_GPIO_Port
#define DRV8323R_FAULT_Pin 				SPI1_FAULT_Pin

#define ADC_TO_VOLT(x)            (((float)(x) / ADC_MAX_COUNT) * ADC_REF_VOLT)
#define ADC_TO_CURR(x)         		(ADC_TO_VOLT(x) - PHASE_CURRENT_ZERO_V) / PHASE_CURRENT_V_PER_A

#define IIR_FILTER_ALPHA 0.1f
#define IIR_FILTER(prev, curr) prev = ((IIR_FILTER_ALPHA * (curr)) + ((1.0f - IIR_FILTER_ALPHA) * (prev)))

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif
/* CLAMP assigns the clamped value back into x (used as a statement) */
#define CLAMP(x, min_val, max_val) ((x) = MIN(MAX((x), (min_val)), (max_val)))
/* -------------------------------------------------------------------------- */
/* Type definitions                                                             */
/* -------------------------------------------------------------------------- */

typedef enum {
    PHASE_FLOAT = 0,
    PHASE_PWM_HIGH,
    PHASE_PWM_LOW
} PhaseState;

/**
 * @brief BLDC hardware handle used by the low-level motor driver.
 */
typedef struct {
    TIM_HandleTypeDef *htim; // Timer handle for PWM generation
    uint32_t chA;
    uint32_t chB;
    uint32_t chC;

 		ADC_HandleTypeDef hadc; // ADC handle for current/voltage sensing
} BLDC_Handle_t;

/**
 * @brief BLDC Settings structure for real-time FOC control parameters.
 */
typedef struct {
    /* Motor identity */
    float pole_pairs;
    float motor_kv;
    float phase_resistance;
    float phase_inductance;

    /* FOC control */
    float current_kp;
    float current_ki;
    float speed_kp;
    float speed_ki;
    float i_d_target;

    /* Sensorless observer */
    float pll_kp;
    float pll_ki;
    float bemf_filter_cutoff_hz;
    float observer_gain;
    float min_rpm_closed_loop;
    float max_rpm_open_loop;

    /* Startup behavior */
    float startup_ramp_time_ms;
    float alignment_current;
    uint8_t startup_mode;

    /* Limits & protection */
    float max_phase_current;
    float max_bus_voltage;
    float max_temperature;
    float current_derating_start;
} bldc_settings_t;

/**
 * @brief BLDC Telemetry data structure for real-time FOC diagnostics.
 */
typedef struct {
    /* Motor speed */
    float rpm_actual;
    float rpm_target;

    /* Phase currents (Amps) */
    float current_phase_a;
    float current_phase_b;
    float current_phase_c;

    /* Phase voltages (Volts) */
    float voltage_phase_a;
    float voltage_phase_b;
    float voltage_phase_c;

    /* FOC DQ reference frame */
    float i_d;
    float i_q;

    /* Position and timing */
    float angle_mechanical;
    float angle_electrical;
    uint64_t timestamp_ms;

    /* Battery and power */
    float battery_voltage;
    float battery_current;
    float energy_used_wh;
    float energy_rem_wh;

    /* Observer/FOC characteristics */
    uint8_t bemf_strength;
    uint8_t obs_confidence;
    uint8_t pll_lock_status;
    uint8_t angle_error_deg;

		// TODO: not added to the Electron cbor IPC yet. 
		float temp_c;
} bldc_telemetry_t;

/**
 * @brief USB message types for the unified USB queue.
 */
typedef enum {
    USB_MSG_TELEMETRY,
    USB_MSG_SETTINGS,
    USB_MSG_DEBUG_STR,
    USB_MSG_ERROR
} usb_msg_type_t;

/**
 * @brief Unified USB message structure.
 */
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

void bldc_comm_init(BLDC_Handle_t *motor);
void bldc_comm_set_duty(uint16_t duty);
void bldc_comm_commutate(uint8_t step);
void bldc_comm_enable(void);
void bldc_comm_disable(void);

void bldc_dronecan_init(void);
void bldc_dronecan_update(void);
void bldc_dronecan_pub(void);

void 		 bldc_drv8323r_init(void);
uint16_t bldc_drv8323r_read_reg(uint8_t reg);
void 		 bldc_drv8323r_write_reg(uint8_t reg, uint16_t data);
uint32_t bldc_drv8323r_read_faults(void);
void 		 bldc_drv8323r_reset_faults(void);
void 		 bldc_drv8323r_set_oc_adj(int val);
void 		 bldc_drv8323r_set_oc_mode(int mode);
void 		 bldc_drv8323r_set_current_amp_gain(int gain);
void 		 bldc_drv8323r_dccal_on(void);
void 		 bldc_drv8323r_dccal_off(void);
char* 	 bldc_drv8323r_faults_to_string(uint32_t faults);

void bldc_telem_init(void);
void bldc_telem_pub(void);
bldc_settings_t* bldc_get_settings(void);

void usb_msg_tx(usb_msg_t* msg, uint8_t* buf, uint16_t buf_size);
void usb_msg_rx(uint8_t *Buf, uint32_t *Len);


#ifdef BLDC_TELEM_USE_DEMO
void gen_demo_telemetry(bldc_telemetry_t* telem_data);
#endif



extern BLDC_Handle_t bldc_h;

#ifdef __cplusplus
}
#endif

#endif /* __BLDC_H */