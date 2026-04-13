#ifndef CONFIG_H
#define CONFIG_H

// --- Pin assignments ---
#define PHOTODIODE_ADC_PIN   26   // GP26 = ADC channel 0
#define PHOTODIODE_ADC_CH     0
#define AOM_PWM_PIN          15   // Any GPIO for PWM output to AOM driver

// --- Setpoint adjust buttons (active low to GND, internal pull-up) ---
#define BTN_SETPOINT_MINUS_PIN  20   // decrease setpoint
#define BTN_SETPOINT_PLUS_PIN   22   // increase setpoint
#define SETPOINT_STEP_V         0.05f
#define SETPOINT_MIN_V          0.0f

// --- ADC parameters ---
#define ADC_BITS             12
#define ADC_MAX              ((1 << ADC_BITS) - 1)  // 4095
#define ADC_VREF             3.3f

#define SETPOINT_MAX_V          ADC_VREF

// --- PWM parameters ---
#define PWM_WRAP             1023  // 10-bit resolution (~3.2 mV steps)
#define PWM_CLKDIV           1.0f  // 150 MHz / 1024 ≈ 146.5 kHz PWM frequency

// DC calibration after your RC low-pass.
// Measurement shows the RC passes DC at ~96%, so no correction needed.
#define PWM_DUTY_VOLTAGE_SCALE  1.0f

// --- PID gains (tune these for your setup) ---
// Higher Kp/Ki = faster response; reduce if the loop rings or oscillates.
#define PID_KP               4.0f
#define PID_KI               2.0f
#define PID_KD               0.04f

// --- Feedback polarity ---
// +1.0f: more AOM drive = more light on photodiode (1st-order / diffracted beam)
// -1.0f: more AOM drive = less light on photodiode (0th-order / pass-through)
#define PID_ERROR_SIGN       1.0f

// --- Setpoint in volts ---
#define SETPOINT_V           1.5f

// --- Control loop period in microseconds ---
#define LOOP_PERIOD_US       1000  // 1 kHz control loop

// --- PID output limits (normalised power: 0.0 = null, 1.0 = max) ---
// The linearisation LUT maps these to the physical 0.18–1.25 V range.
#define PID_OUT_MIN          0.0f
#define PID_OUT_MAX          1.0f

// --- Anti-windup: maximum integral accumulator (in volt·seconds) ---
#define PID_INTEGRAL_MAX     0.5f

// --- Serial output decimation (print every N-th loop) ---
// ~100 Hz: 1000 µs * 10 = 10 ms per line
#define SERIAL_DECIMATION    10

#endif // CONFIG_H
