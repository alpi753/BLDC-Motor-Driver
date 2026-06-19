#ifndef BLDC_OBSERVER_H
#define BLDC_OBSERVER_H

#include <stdint.h>
#include "bsp_autoconf.h"

#if CONFIG_FOC_ENABLE

#include "bldc.h"

#ifdef __cplusplus
extern "C" {
#endif

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
void bldc_foc_comm_thread(void *argument);
void bldc_foc_get_state(bldc_foc_state_t *state);
void bldc_foc_fill_telemetry(bldc_telemetry_t *telem);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_FOC_ENABLE */

#endif /* BLDC_OBSERVER_H */