#include "pid.h"

static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void pid_init(pid_state_t *pid, float kp, float ki, float kd,
              float out_min, float out_max, float integral_max) {
    pid->kp           = kp;
    pid->ki           = ki;
    pid->kd           = kd;
    pid->integral     = 0.0f;
    pid->prev_error   = 0.0f;
    pid->out_min      = out_min;
    pid->out_max      = out_max;
    pid->integral_max = integral_max;
}

void pid_reset(pid_state_t *pid) {
    pid->integral   = 0.0f;
    pid->prev_error = 0.0f;
}

float pid_update(pid_state_t *pid, float setpoint, float measurement, float dt) {
    float error = setpoint - measurement;

    pid->integral += error * dt;
    pid->integral = clampf(pid->integral, -pid->integral_max, pid->integral_max);

    float derivative = (dt > 0.0f) ? (error - pid->prev_error) / dt : 0.0f;
    pid->prev_error = error;

    float output = pid->kp * error
                 + pid->ki * pid->integral
                 + pid->kd * derivative;

    return clampf(output, pid->out_min, pid->out_max);
}
