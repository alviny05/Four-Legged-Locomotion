/*
Four-Legged Walker software, file for alphanumeric display
Justin Nascimento (U42983905) and Alvin Yan ()
*/
#include "servo.h"

static const char *TAG = "servo_walk";

// Servo Constants
const gait_phase_t walk_gait[] = {
    {  45,  -45, 500 },   // move both servo to 30 degrees, wait 500ms
    { -45, 45, 500 }   // move both servo to -30 degrees, wait 500ms
};

const size_t walk_gait_len = sizeof(walk_gait) / sizeof(walk_gait[0]);

/* Servo Functions */
static uint32_t servo_angle_to_compare(servo_t *s, int angle)
{
    if (angle < s->min_degree) angle = s->min_degree;
    if (angle > s->max_degree) angle = s->max_degree;

    return (angle - s->min_degree) *
           (s->max_pulse_us - s->min_pulse_us) /
           (s->max_degree - s->min_degree) +
           s->min_pulse_us;
}

void servo_set_angle(servo_t *servo, int angle)
{
    servo->current_angle = angle;
    mcpwm_comparator_set_compare_value(
        servo->comparator,
        servo_angle_to_compare(servo, angle)
    );
}


/* Init function */
void servo_init(servo_t *left, servo_t *right){
    /* timer */
    mcpwm_timer_handle_t timer;
    mcpwm_timer_config_t timer_cfg = {
        .group_id = 0,
        .resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ,
        .period_ticks = SERVO_TIMEBASE_PERIOD,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_cfg, &timer));

    /* operator */
    mcpwm_oper_handle_t oper;
    mcpwm_operator_config_t oper_cfg = {
        .group_id = 0,
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&oper_cfg, &oper));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    /* comparators */
    mcpwm_comparator_config_t cmp_cfg = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &cmp_cfg, &left->comparator));
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &cmp_cfg, &right->comparator));

    /* generators */
    mcpwm_gen_handle_t gen_left, gen_right;

    mcpwm_generator_config_t gen_left_cfg = {
        .gen_gpio_num = SERVO_LEFT_GPIO,
    };
    mcpwm_generator_config_t gen_right_cfg = {
        .gen_gpio_num = SERVO_RIGHT_GPIO,
    };

    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gen_left_cfg, &gen_left));
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gen_right_cfg, &gen_right));

    /* PWM actions */
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(
        gen_left,
        MCPWM_GEN_TIMER_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            MCPWM_TIMER_EVENT_EMPTY,
            MCPWM_GEN_ACTION_HIGH)));

    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(
        gen_left,
        MCPWM_GEN_COMPARE_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            left->comparator,
            MCPWM_GEN_ACTION_LOW)));

    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(
        gen_right,
        MCPWM_GEN_TIMER_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            MCPWM_TIMER_EVENT_EMPTY,
            MCPWM_GEN_ACTION_HIGH)));

    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(
        gen_right,
        MCPWM_GEN_COMPARE_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            right->comparator,
            MCPWM_GEN_ACTION_LOW)));

    /* timer */
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));
}




/* walking task */

static void servo_walk_task(void *arg)
{
    servo_t *left  = &((servo_t *)arg)[0];
    servo_t *right = &((servo_t *)arg)[1];

    size_t phase = 0;

    while (1) {
        servo_set_angle(left,  walk_gait[phase].left_angle);
        servo_set_angle(right, walk_gait[phase].right_angle);

        vTaskDelay(pdMS_TO_TICKS(walk_gait[phase].duration_ms));

        phase = (phase + 1) % walk_gait_len;
    }
}