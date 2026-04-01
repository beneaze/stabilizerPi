#ifndef CONFIG_H
#define CONFIG_H

// --- Pin assignments ---
#define PHOTODIODE_ADC_PIN   26   // GP26 = ADC channel 0
#define PHOTODIODE_ADC_CH     0
#define AOM_PWM_PIN          15   // Any GPIO for PWM output to AOM driver

// --- ADC parameters ---
#define ADC_BITS             12
#define ADC_MAX              ((1 << ADC_BITS) - 1)  // 4095
#define ADC_VREF             3.3f

// --- PWM parameters ---
#define PWM_WRAP             4095  // 12-bit resolution to match ADC
#define PWM_CLKDIV           1.0f  // Clock divider (125 MHz / 1 / 4096 ≈ 30 kHz)

// --- PID gains (tune these for your setup) ---
#define PID_KP               1.0f
#define PID_KI               0.5f
#define PID_KD               0.01f

// --- Setpoint in volts ---
#define SETPOINT_V           1.5f

// --- Control loop period in microseconds ---
#define LOOP_PERIOD_US       1000  // 1 kHz loop rate

// --- PID output limits (in volts, clamped to 0–3.3 V) ---
#define PID_OUT_MIN          0.0f
#define PID_OUT_MAX          3.3f

// --- Anti-windup: maximum integral accumulator (in volt·seconds) ---
#define PID_INTEGRAL_MAX     2.0f

// --- Serial output decimation (print every N-th loop) ---
#define SERIAL_DECIMATION    10

#endif // CONFIG_H
