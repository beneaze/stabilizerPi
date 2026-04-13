#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include "config.h"
#include "aom_lut.h"

#define RAMP_STEPS    200
#define RAMP_DWELL_MS 20

static inline float adc_raw_to_volts(uint16_t raw) {
    return (float)raw * ADC_VREF / (float)ADC_MAX;
}

static inline uint16_t volts_to_pwm(float v) {
    float ratio = (v * PWM_DUTY_VOLTAGE_SCALE) / ADC_VREF;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    return (uint16_t)(ratio * (float)PWM_WRAP);
}

int main(void) {
    stdio_init_all();

    adc_init();
    adc_gpio_init(PHOTODIODE_ADC_PIN);
    adc_select_input(PHOTODIODE_ADC_CH);

    gpio_set_function(AOM_PWM_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(AOM_PWM_PIN);
    pwm_set_wrap(slice, PWM_WRAP);
    pwm_set_clkdiv(slice, PWM_CLKDIV);
    pwm_set_gpio_level(AOM_PWM_PIN, 0);
    pwm_set_enabled(slice, true);

    sleep_ms(2000);
    printf("# ramp_test_lut  v1.0\n");
    printf("# normalized %.3f -> %.3f (same as PID output), then aom_linearize()\n",
           (double)PID_OUT_MIN, (double)PID_OUT_MAX);
    printf("# %d steps, %d ms/step\n", RAMP_STEPS, RAMP_DWELL_MS);
    printf("# columns: input_V output_V setpoint_V\n");

    const float span = PID_OUT_MAX - PID_OUT_MIN;
    const float step_n = span / (float)(RAMP_STEPS - 1);

    while (true) {
        for (int i = 0; i < RAMP_STEPS; i++) {
            float norm = PID_OUT_MIN + step_n * (float)i;
            float drive_v = aom_linearize(norm);
            uint16_t pwm_level = volts_to_pwm(drive_v);
            pwm_set_gpio_level(AOM_PWM_PIN, pwm_level);

            sleep_ms(RAMP_DWELL_MS);

            uint16_t raw = adc_read();
            float input_v = adc_raw_to_volts(raw);

            printf("%.4f %.4f %.4f\n",
                   (double)input_v, (double)drive_v, (double)drive_v);
        }
        for (int i = RAMP_STEPS - 1; i >= 0; i--) {
            float norm = PID_OUT_MIN + step_n * (float)i;
            float drive_v = aom_linearize(norm);
            uint16_t pwm_level = volts_to_pwm(drive_v);
            pwm_set_gpio_level(AOM_PWM_PIN, pwm_level);

            sleep_ms(RAMP_DWELL_MS);

            uint16_t raw = adc_read();
            float input_v = adc_raw_to_volts(raw);

            printf("%.4f %.4f %.4f\n",
                   (double)input_v, (double)drive_v, (double)drive_v);
        }
    }

    return 0;
}
