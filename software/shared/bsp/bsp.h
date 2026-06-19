#ifndef BLDC_BSP_H
#define BLDC_BSP_H

#include <stdint.h>
#include "bsp_autoconf.h"

#if CONFIG_STM32_FAMILY_G4
#include "stm32g4xx_hal.h"
#elif CONFIG_STM32_FAMILY_F4
#include "stm32f4xx_hal.h"
#else
#error "Unsupported CONFIG_STM32_FAMILY — check board.conf"
#endif

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Board constants (selected at build time via board.conf / family)           */
/* -------------------------------------------------------------------------- */

#if CONFIG_STM32_FAMILY_F4

#define ADC_MAX_COUNT             4095.0f
#define ADC_REF_VOLT              3.3f
#define PHASE_CURRENT_ZERO_V      1.65f
#define PHASE_CURRENT_V_PER_A     0.100f
#define BUS_VOLTAGE_DIVIDER_RATIO 11.0f
#define THERMISTOR_PULLUP         10000.0f
#define THERMISTOR_R25            10000.0f
#define THERMISTOR_BETA           3950.0f
#define BATTERY_CAPACITY_WH       100.0f

#define ADC_HAS_VBUS_ADC          1
#define ADC_IDX_PHASE_A           0U
#define ADC_IDX_PHASE_B           1U
#define ADC_IDX_PHASE_C           2U
#define ADC_IDX_VBUS              3U
#define ADC_IDX_TEMP              4U
#define ADC_IDX_TEMP_MTR          5U

#define MAJOR_SW                  1
#define MINOR_SW                  0
#define MAJOR_HW                  1
#define MINOR_HW                  0

#define DRV8323R_CS_GPIO_Port     SPI1_CS_GPIO_Port
#define DRV8323R_CS_Pin           SPI1_CS_Pin
#define DRV8323R_EN_GPIO_Port     SPI1_EN_GPIO_Port
#define DRV8323R_EN_Pin           SPI1_EN_Pin
#define DRV8323R_FAULT_GPIO_Port  SPI1_FAULT_GPIO_Port
#define DRV8323R_FAULT_Pin        SPI1_FAULT_Pin

#define PHASE_1_CH                TIM_CHANNEL_1
#define PHASE_2_CH                TIM_CHANNEL_2
#define PHASE_3_CH                TIM_CHANNEL_3

#define BLDC_PHASE1_PWM_CH        TIM_CHANNEL_1
#define BLDC_PHASE1_LOW_CCR_CH    TIM_CHANNEL_1
#define BLDC_PHASE1_CCER_E        TIM_CCER_CC1E
#define BLDC_PHASE1_CCER_NE       0U
#define BLDC_PHASE1_LOW_USE_GPIO  1
#define BLDC_PHASE1_LOW_GPIO_Port INLA_GPIO_Port
#define BLDC_PHASE1_LOW_GPIO_Pin  INLA_Pin

#define BLDC_PHASE2_PWM_CH        TIM_CHANNEL_2
#define BLDC_PHASE2_LOW_CCR_CH    TIM_CHANNEL_2
#define BLDC_PHASE2_CCER_E        TIM_CCER_CC2E
#define BLDC_PHASE2_CCER_NE       TIM_CCER_CC2NE

#define BLDC_PHASE3_PWM_CH        TIM_CHANNEL_3
#define BLDC_PHASE3_LOW_CCR_CH    TIM_CHANNEL_3
#define BLDC_PHASE3_CCER_E        TIM_CCER_CC3E
#define BLDC_PHASE3_CCER_NE       TIM_CCER_CC3NE

#elif CONFIG_STM32_FAMILY_G4

#define ADC_MAX_COUNT             4095.0f
#define ADC_REF_VOLT              3.3f
#define PHASE_CURRENT_ZERO_V      1.65f
#define PHASE_CURRENT_V_PER_A     0.100f
#define BUS_VOLTAGE_DIVIDER_RATIO 11.0f
#define THERMISTOR_PULLUP         10000.0f
#define THERMISTOR_R25            10000.0f
#define THERMISTOR_BETA           3950.0f
#define BATTERY_CAPACITY_WH       100.0f

#define ADC_HAS_VBUS_ADC          1
#define ADC_IDX_PHASE_A           0U
#define ADC_IDX_PHASE_B           1U
#define ADC_IDX_PHASE_C           2U
#define ADC_IDX_VBUS              3U
#define ADC_IDX_TEMP              4U
#define ADC_SLOW_RANK_NTC_FET     0U
#define ADC_SLOW_RANK_NTC_MTR     1U
#define ADC_SLOW_RANK_VBUS        2U

#define MAJOR_SW                  1
#define MINOR_SW                  0
#define MAJOR_HW                  1
#define MINOR_HW                  0

#define PHASE_1_CH                TIM_CHANNEL_1
#define PHASE_2_CH                TIM_CHANNEL_2
#define PHASE_3_CH                TIM_CHANNEL_3

#define BLDC_PHASE1_PWM_CH        TIM_CHANNEL_1
#define BLDC_PHASE1_LOW_CCR_CH    TIM_CHANNEL_1
#define BLDC_PHASE1_CCER_E        TIM_CCER_CC1E
#define BLDC_PHASE1_CCER_NE       TIM_CCER_CC1NE

#define BLDC_PHASE2_PWM_CH        TIM_CHANNEL_2
#define BLDC_PHASE2_LOW_CCR_CH    TIM_CHANNEL_2
#define BLDC_PHASE2_CCER_E        TIM_CCER_CC2E
#define BLDC_PHASE2_CCER_NE       TIM_CCER_CC2NE

#define BLDC_PHASE3_PWM_CH        TIM_CHANNEL_3
#define BLDC_PHASE3_LOW_CCR_CH    TIM_CHANNEL_3
#define BLDC_PHASE3_CCER_E        TIM_CCER_CC3E
#define BLDC_PHASE3_CCER_NE       TIM_CCER_CC3NE

#define DRV8323R_CS_GPIO_Port     SPI1_CS_GPIO_Port
#define DRV8323R_CS_Pin           SPI1_CS_Pin
#define DRV8323R_EN_GPIO_Port     SPI1_EN_GPIO_Port
#define DRV8323R_EN_Pin           SPI1_EN_Pin
#define DRV8323R_FAULT_GPIO_Port  SPI1_FAULT_GPIO_Port
#define DRV8323R_FAULT_Pin        SPI1_FAULT_Pin

#endif

/* -------------------------------------------------------------------------- */
/* Portable math / filter helpers                                             */
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

void bsp_hw_accel_init(void);
float bsp_sin_f(float radians);
float bsp_log_f(float x);
float bsp_iir_lowpass_f(float prev, float sample, float alpha);
uint32_t bsp_rand32(void);

/* -------------------------------------------------------------------------- */
/* BSP core                                                                   */
/* -------------------------------------------------------------------------- */

typedef struct {
    TIM_HandleTypeDef *htim_high;
    TIM_HandleTypeDef *htim_low;
    TIM_HandleTypeDef *htim_aux;
    uint32_t chA;
    uint32_t chB;
    uint32_t chC;
    uint32_t aux_chA;
    uint32_t aux_chB;
    uint32_t aux_chC;
    ADC_HandleTypeDef *hadc;
} BLDC_Handle_t;

void bsp_init(void);
void bsp_usb_init(void);
const char *bsp_board_name(void);
BLDC_Handle_t *bsp_get_motor_handle(void);

void bsp_board_init(void);
BLDC_Handle_t *bsp_board_get_motor_handle(void);

#if !BLDC_TELEM_USE_DEMO
int bsp_telem_adc_init(void);
int bsp_telem_adc_snapshot(uint16_t *samples, unsigned count);
void bsp_telem_adc_conv_cplt(ADC_HandleTypeDef *hadc);
#endif

#if CONFIG_FOC_ENABLE
int bsp_foc_sample_sensors(float *ia, float *ib, float *ic, float *vbus);
int bsp_foc_loop_uses_hw_adc_trigger(void);
void bsp_foc_loop_hw_init(void (*notify_from_isr)(void));
#endif

/* -------------------------------------------------------------------------- */
/* Motor / protocol types                                                     */
/* -------------------------------------------------------------------------- */

typedef enum {
    PHASE_FLOAT = 0,
    PHASE_PWM_HIGH,
    PHASE_PWM_LOW
} PhaseState;

typedef struct {
    float pole_pairs;
    float phase_resistance;
    float phase_inductance;
    float current_kp;
    float current_ki;
    float speed_kp;
    float speed_ki;
    float pll_kp;
    float pll_ki;
    float observer_gain;
    float min_rpm_closed_loop;
    float max_rpm_open_loop;
    float startup_ramp_time_ms;
    float alignment_time_ms;
    float open_loop_ramp_rpm_s;
    float alignment_current;
    float open_loop_current;
    float open_loop_start_rpm;
    float handoff_angle_err_deg;
    float handoff_min_confidence;
    float rpm_target;
    uint8_t startup_mode;
    float max_phase_current;
} bldc_settings_t;

#define BLDC_DEVICE_ID_LEN 16U

typedef struct {
    uint8_t device_id[BLDC_DEVICE_ID_LEN];
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
/* Utils                                                                      */
/* -------------------------------------------------------------------------- */

void dwt_init(void);
uint32_t millis32(void);
uint64_t micros64(void);
void get_device_id(uint8_t id[16]);
uint32_t rand32(void);

/* -------------------------------------------------------------------------- */
/* Commutation / motor driver / telemetry                                     */
/* -------------------------------------------------------------------------- */

void bldc_comm_init(BLDC_Handle_t *motor);
void bldc_comm_set_duty(uint16_t duty);
void bldc_comm_commutate(uint8_t step);
void bldc_comm_enable(void);
void bldc_comm_disable(void);
void CommThread(void *argument);

void bldc_dronecan_init(void);
void bldc_dronecan_update(void);
void bldc_dronecan_pub(void);

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

void bldc_telem_init(void);
void bldc_telem_fetch(usb_msg_t *msg);
void TelemThread(void *argument);
bldc_settings_t *bldc_get_settings(void);

#if CONFIG_BLDC_HAS_USB_TELEM
void usb_msg_tx(usb_msg_t *msg, uint8_t *buf, uint16_t buf_size);
void usb_msg_rx(uint8_t *Buf, uint32_t *Len);
#endif

#if BLDC_TELEM_USE_DEMO
void bldc_telem_fake(void);
#else
void bldc_telem_update(void);
#endif

extern BLDC_Handle_t bldc_h;

/* -------------------------------------------------------------------------- */
/* Sensorless FOC / observer (CONFIG_FOC_ENABLE)                              */
/* -------------------------------------------------------------------------- */

#if CONFIG_FOC_ENABLE

typedef enum {
    BLDC_FOC_MODE_IDLE = 0,
    BLDC_FOC_MODE_ALIGN,
    BLDC_FOC_MODE_OPEN_LOOP,
    BLDC_FOC_MODE_CLOSED_LOOP,
    BLDC_FOC_MODE_FAULT,
} bldc_foc_mode_t;

typedef struct {
    bldc_foc_mode_t mode;
    float theta_elec_rad;
    float omega_elec_rad_s;
    float rpm_actual;
    float rpm_target;
    float i_d;
    float i_q;
    float i_alpha;
    float i_beta;
    float v_d;
    float v_q;
    float bemf_alpha;
    float bemf_beta;
    float bemf_magnitude;
    float angle_error_rad;
    uint8_t pll_locked;
    uint8_t confidence;
} bldc_foc_state_t;

void bldc_foc_init(void);
void bldc_foc_reset(void);
void bldc_foc_set_target_rpm(float rpm);
void bldc_foc_apply_settings(void);
void bldc_settings_init_defaults(void);
void bldc_foc_comm_thread(void *argument);
void bldc_foc_hal_period_callback(TIM_HandleTypeDef *htim);
void bldc_foc_get_state(bldc_foc_state_t *state);
void bldc_foc_fill_telemetry(bldc_telemetry_t *telem);

#endif /* CONFIG_FOC_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* BLDC_BSP_H */