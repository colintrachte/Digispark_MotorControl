/*
 * ATtiny85 Digispark – DRV8833 Single-Channel Forward PWM Motor Driver
 *
 * HARDWARE
 *   MCU    : Digispark (ATtiny85 @ 8 MHz internal oscillator)
 *   Driver : DRV8833 breakout, single channel (AIN1/AIN2 → AOUT1/AOUT2)
 *   Motor  : 3 V, 1 A DC motor with external flyback diode
 *   Control: 10 kΩ linear pot with integral SPST power switch
 *            Switch cuts the entire supply – firmware needs no sleep logic.
 *
 * PINOUT (Digispark pin numbers → ATtiny85 port bits)
 *   P0  PB0 / OC0A  →  DRV8833 AIN1   (PWM output)
 *   P1  PB1         →  DRV8833 AIN2   (held LOW = forward / coast on 0 PWM)
 *   P2  PB2 / ADC1  →  Pot wiper      (add 100 nF cap wiper-to-GND)
 *
 * DRV8833 TRUTH TABLE (one H-bridge channel)
 *   AIN1  AIN2  →  Output
 *   PWM   LOW   →  Forward PWM   (fast decay)
 *   LOW   LOW   →  Coast         (both outputs float)
 *   HIGH  HIGH  →  Brake         (not used here)
 *
 * POWER
 *   Li-ion (+) → Digispark 5V pin  (bypasses onboard regulator)
 *   Li-ion (–) → GND
 *   DRV8833 VM → same Li-ion (+) rail
 *   All GNDs tied together.
 *   NOTE: Desolder the Digispark power LED or it will draw 3-5 mA always.
 *
 * PWM
 *   Timer0, Fast PWM, OC0A non-inverting, prescaler = 1
 *   f_PWM = 8 MHz / 256 = 31.25 kHz  (above audible range)
 */

#include <avr/io.h>
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Pin assignments
// ---------------------------------------------------------------------------
#define PIN_AIN1   0    // PB0 – OC0A – PWM to DRV8833 AIN1
#define PIN_AIN2   1    // PB1 – static LOW (forward direction)
#define ADC_CH     1    // ADC1 on PB2 (Digispark P2)

// ---------------------------------------------------------------------------
// Tuning
// ---------------------------------------------------------------------------
// ADC counts (0-1023) below this are treated as "off".
// Raise if motor creeps at minimum pot position.
#define POT_DEAD_LOW   100

// ADC counts above this are treated as "full speed".
// Prevents a hard stop at top of travel if pot doesn't quite reach 1023.
#define POT_DEAD_HIGH  1011

// Control loop period (ms). 20 ms = 50 Hz, plenty for a hand-turned pot.
#define LOOP_MS        20

// ---------------------------------------------------------------------------
void setup() {
    // Disable digital input buffer on ADC pin – saves a few µA
    DIDR0 |= (1 << ADC1D);

    pinMode(PIN_AIN1, OUTPUT);
    pinMode(PIN_AIN2, OUTPUT);

    // AIN2 held LOW throughout – selects forward / coast mode on DRV8833
    digitalWrite(PIN_AIN2, LOW);

    // --- Timer0: Fast PWM, non-inverting on OC0A, prescaler = 1 ----------
    // WGM01|WGM00 = Fast PWM mode
    // COM0A1      = clear OC0A on compare match, set at BOTTOM (non-inv)
    // CS00        = clk/1  →  8 MHz / 256 = 31.25 kHz
    TCCR0A = (1 << WGM01) | (1 << WGM00) | (1 << COM0A1);
    TCCR0B = (1 << CS00);
    OCR0A  = 0;   // Motor off until pot is read

    // --- ADC: VCC reference, right-adjust result, channel 1 --------------
    // REFS0 = 0  →  VCC reference (correct for pot ratiometric to VCC)
    // MUX   = ADC_CH (ADC1 / PB2)
    // Prescaler /64  →  8 MHz / 64 = 125 kHz (within 50-200 kHz ADC spec)
    ADMUX  = (ADC_CH & 0x07);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);

    // Throwaway conversion: first result after ADC enable is unreliable
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
}

// ---------------------------------------------------------------------------
// Direct ADC read – leaner than Arduino analogRead()
// ---------------------------------------------------------------------------
static inline uint16_t readADC() {
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

// ---------------------------------------------------------------------------
void loop() {
    uint16_t raw = readADC();

    uint8_t pwm;
    if (raw <= POT_DEAD_LOW) {
        pwm = 0;
    } else if (raw >= POT_DEAD_HIGH) {
        pwm = 255;
    } else {
        pwm = (uint8_t) map(raw, POT_DEAD_LOW, POT_DEAD_HIGH, 1, 254);
    }

    OCR0A = pwm;   // 0 = coast (AIN1 LOW + AIN2 LOW), 255 = full forward

    delay(LOOP_MS);
}
