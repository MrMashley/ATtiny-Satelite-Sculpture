/*
ATTiny85 based satellite Sculpture
x2 solar cells charge a 3V 1uf capacitor which provides power to the attiny85  

The ATTiny goes into sleep mode and wakes every ~16 seconds reads the voltage from the solar panels and blink an LED if the voltage is below a darkness_threshold.

*/

#include <avr/sleep.h>
#include <avr/interrupt.h>

const uint8_t led_pin = 1;          // PB1 (physical pin 6) - PWM capable
const uint8_t volt_read_pin = A1;   // PB2 (physical pin 7)
const uint8_t pot_power_pin = 3;   // PB3 (physical pin 2)
const uint8_t pot_pin = A2;         // PB4 (physical pin 3)


//int pot_reading = 0; // reading of the pot pin
const int darkness_threshold = 800; // Threshold to start blinking (lower = darker)

volatile uint8_t count = 0;
volatile bool do_blink = false;

ISR(WDT_vect) {
  // Keep ISR minimal
  count++;
  if (count > 2) {      // every 2 wakeups => ~16s
    count = 0;
    do_blink = true;    // set flag for main loop
  }
}

void safeBlinkRoutine() {
  // Briefly enable ADC, read panel voltage and disable ADC
  ADCSRA |= (1 << ADEN);
  int panel_reading = analogRead(volt_read_pin);
  ADCSRA &= ~(1 << ADEN);

 
  if (panel_reading < darkness_threshold) {

    // light LED and fade off
    for (int i = 235; i >= 0; i -= 10) {
      analogWrite(led_pin, i);
      delay(1);
    }
    analogWrite(led_pin, 0);
  }
}

void setup() {
  pinMode(led_pin, OUTPUT);
  pinMode(volt_read_pin, INPUT);
  pinMode(pot_pin, INPUT);
  pinMode(pot_power_pin, OUTPUT);
  digitalWrite(pot_power_pin, LOW);

  // Disable ADC initially to save power
  ADCSRA &= ~(1 << ADEN);

  // Optional startup blink
  digitalWrite(led_pin, HIGH);
  delay(200);
  digitalWrite(led_pin, LOW);

  cli();                    // Disable interrupts while configuring WDT
  MCUSR &= ~(1 << WDRF);    // Clear watchdog reset flag

  // Configure Watchdog for 8s interrupt (Mellis core style)
  WDTCR = (1 << WDIE) | (1 << WDP3); // interrupt mode, ~8s

  sei(); // enable global interrupts

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

void loop() {
  // If blink requested by ISR, perform reads and blink here (safe)
  if (do_blink) {
    // Clear the flag early to avoid missed requests
    do_blink = false;
    safeBlinkRoutine();
  }
  // Sleep until WDT wakes us again
  sleep_mode();
}

