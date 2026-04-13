#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include "config.h"

#define RAMP_MIN_V    0.0f
#define RAMP_MAX_V    1.25f
#define RAMP_STEPS    200
#define RAMP_DWELL_MS 20      /* time at each step (total sweep ≈ 4 s) */

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
    printf("# ramp_test  v1.0\n");
    printf("# AOM drive %.2f -> %.2f V, %d steps, %d ms/step\n",
           (double)RAMP_MIN_V, (double)RAMP_MAX_V, RAMP_STEPS, RAMP_DWELL_MS);
    printf("# columns: input_V output_V setpoint_V\n");

    const float step_v = (RAMP_MAX_V - RAMP_MIN_V) / (float)(RAMP_STEPS - 1);

    while (true) {
        /* Ramp up */
        for (int i = 0; i < RAMP_STEPS; i++) {
            float drive_v = RAMP_MIN_V + step_v * (float)i;
            uint16_t pwm_level = volts_to_pwm(drive_v);
            pwm_set_gpio_level(AOM_PWM_PIN, pwm_level);

            sleep_ms(RAMP_DWELL_MS);

            uint16_t raw = adc_read();
            float input_v = adc_raw_to_volts(raw);

            printf("%.4f %.4f %.4f\n",
                   (double)input_v, (double)drive_v, (double)drive_v);
        }
        /* Ramp down */
        for (int i = RAMP_STEPS - 1; i >= 0; i--) {
            float drive_v = RAMP_MIN_V + step_v * (float)i;
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
