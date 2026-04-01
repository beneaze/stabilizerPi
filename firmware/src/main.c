#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

#include "config.h"
#include "pid.h"

static inline float adc_raw_to_volts(uint16_t raw) {
    return (float)raw * ADC_VREF / (float)ADC_MAX;
}

static inline uint16_t volts_to_pwm(float v) {
    float ratio = v / ADC_VREF;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    return (uint16_t)(ratio * (float)PWM_WRAP);
}

int main(void) {
    stdio_init_all();

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

    const float dt = (float)LOOP_PERIOD_US / 1e6f;
    uint32_t loop_count = 0;

    // Allow USB serial to connect before first print
    sleep_ms(2000);
    printf("# stabilizerpi  v1.0\n");
    printf("# setpoint_V=%.3f  Kp=%.3f Ki=%.3f Kd=%.4f  loop_us=%d\n",
           SETPOINT_V, PID_KP, PID_KI, PID_KD, LOOP_PERIOD_US);
    printf("# columns: input_V output_V\n");

    while (true) {
        absolute_time_t t_start = get_absolute_time();

        uint16_t raw = adc_read();
        float input_v = adc_raw_to_volts(raw);

        float output_v = pid_update(&pid, SETPOINT_V, input_v, dt);

        pwm_set_gpio_level(AOM_PWM_PIN, volts_to_pwm(output_v));

        if (++loop_count >= SERIAL_DECIMATION) {
            loop_count = 0;
            printf("%.4f %.4f\n", input_v, output_v);
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
