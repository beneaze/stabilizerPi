#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include "config.h"
#include "pid.h"
#include "aom_lut.h"

static inline float adc_raw_to_volts(uint16_t raw) {
    return (float)raw * ADC_VREF / (float)ADC_MAX;
}

static inline uint16_t volts_to_pwm(float v) {
    float ratio = (v * PWM_DUTY_VOLTAGE_SCALE) / ADC_VREF;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    return (uint16_t)(ratio * (float)PWM_WRAP);
}

/** Active-low buttons with pull-up: one step per press, debounced. */
static void poll_setpoint_buttons(pid_state_t *pid, float *setpoint_v) {
    static bool armed_minus = true;
    static bool armed_plus = true;
    static uint8_t db_m = 0;
    static uint8_t db_p = 0;
    enum { DEBOUNCE_LOOPS = 5 };

    bool m = !gpio_get(BTN_SETPOINT_MINUS_PIN);
    bool p = !gpio_get(BTN_SETPOINT_PLUS_PIN);

    if (m) {
        if (db_m < 250) {
            db_m++;
        }
    } else {
        db_m = 0;
        armed_minus = true;
    }

    if (p) {
        if (db_p < 250) {
            db_p++;
        }
    } else {
        db_p = 0;
        armed_plus = true;
    }

    if (db_m >= DEBOUNCE_LOOPS && armed_minus) {
        *setpoint_v -= SETPOINT_STEP_V;
        if (*setpoint_v < SETPOINT_MIN_V) {
            *setpoint_v = SETPOINT_MIN_V;
        }
        armed_minus = false;
        pid_reset(pid);
        printf("# setpoint_V=%.3f\n", (double)*setpoint_v);
    }
    if (db_p >= DEBOUNCE_LOOPS && armed_plus) {
        *setpoint_v += SETPOINT_STEP_V;
        if (*setpoint_v > SETPOINT_MAX_V) {
            *setpoint_v = SETPOINT_MAX_V;
        }
        armed_plus = false;
        pid_reset(pid);
        printf("# setpoint_V=%.3f\n", (double)*setpoint_v);
    }
}

int main(void) {
    stdio_init_all();

    // --- Setpoint buttons ---
    gpio_init(BTN_SETPOINT_MINUS_PIN);
    gpio_set_dir(BTN_SETPOINT_MINUS_PIN, GPIO_IN);
    gpio_pull_up(BTN_SETPOINT_MINUS_PIN);

    gpio_init(BTN_SETPOINT_PLUS_PIN);
    gpio_set_dir(BTN_SETPOINT_PLUS_PIN, GPIO_IN);
    gpio_pull_up(BTN_SETPOINT_PLUS_PIN);

    // --- ADC setup (photodiode input) ---
    adc_init();
    adc_gpio_init(PHOTODIODE_ADC_PIN);
    adc_select_input(PHOTODIODE_ADC_CH);

    // --- PWM setup (AOM drive output) ---
    gpio_set_function(AOM_PWM_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(AOM_PWM_PIN);
    pwm_set_wrap(slice, PWM_WRAP);
    pwm_set_clkdiv(slice, PWM_CLKDIV);
    pwm_set_gpio_level(AOM_PWM_PIN, 0);
    pwm_set_enabled(slice, true);

    // --- PID init ---
    pid_state_t pid;
    pid_init(&pid, PID_KP, PID_KI, PID_KD,
             PID_OUT_MIN, PID_OUT_MAX, PID_INTEGRAL_MAX);

    float setpoint_v = SETPOINT_V;

    const float dt = (float)LOOP_PERIOD_US / 1e6f;
    uint32_t loop_count = 0;

    // Allow USB serial to connect before first print
    sleep_ms(2000);
    printf("# stabilizerpi  v1.1\n");
    printf("# setpoint_V=%.3f  Kp=%.3f Ki=%.3f Kd=%.4f  loop_us=%d\n",
           (double)setpoint_v, (double)PID_KP, (double)PID_KI, (double)PID_KD,
           LOOP_PERIOD_US);
    printf("# columns: input_V output_V setpoint_V\n");

    while (true) {
        absolute_time_t t_start = get_absolute_time();

        poll_setpoint_buttons(&pid, &setpoint_v);

        uint16_t raw = adc_read();
        float input_v = adc_raw_to_volts(raw);

        float pid_out = pid_update(&pid, setpoint_v, input_v, dt, PID_ERROR_SIGN);
        float drive_v = aom_linearize(pid_out);

        uint16_t pwm_level = volts_to_pwm(drive_v);
        pwm_set_gpio_level(AOM_PWM_PIN, pwm_level);

        if (++loop_count >= SERIAL_DECIMATION) {
            loop_count = 0;
            printf("%.4f %.4f %.4f %u %.1f%%\n",
                   (double)input_v, (double)drive_v, (double)setpoint_v,
                   pwm_level,
                   (double)(100.0f * (float)pwm_level / (float)(PWM_WRAP + 1)));
        }

        // Maintain fixed loop period
        int64_t elapsed = absolute_time_diff_us(t_start, get_absolute_time());
        int64_t remaining = (int64_t)LOOP_PERIOD_US - elapsed;
        if (remaining > 0) {
            sleep_us((uint64_t)remaining);
        }
    }

    return 0;
}
