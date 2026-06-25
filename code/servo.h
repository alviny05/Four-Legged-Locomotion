/*
Four-Legged Walker software, file for alphanumeric display
Justin Nascimento (U42983905) and Alvin Yan ()
*/
#ifndef SERVO_H
#define SERVO_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/mcpwm_prelude.h"

/* PWM config */
#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000   // 1 MHz → 1 us
#define SERVO_TIMEBASE_PERIOD        20000     // 20 ms (50 Hz)

/* GPIOs */
#define SERVO_LEFT_GPIO   12
#define SERVO_RIGHT_GPIO  27

/* Types */
typedef struct {
    mcpwm_cmpr_handle_t comparator;
    int min_pulse_us;
    int max_pulse_us;
    int min_degree;
    int max_degree;
    int current_angle;
} servo_t;

/* Walk */
typedef struct {
    int left_angle;
    int right_angle;
    int duration_ms;
} gait_phase_t;

// Global Variables
extern const gait_phase_t walk_gait[];
extern const size_t walk_gait_len;

// Function Prototypes
void servo_set_angle(servo_t *servo, int angle);
void servo_init(servo_t *left, servo_t *right);

#endif