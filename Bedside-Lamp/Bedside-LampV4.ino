#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

const byte LEDS_PIN = 3;     // PB3 (Pin 2) - LED MOSFET
const byte V_SENSE_PIN = 2;  // PB2 (Pin 7) - ADC1 (Μέση διαιρέτη)
const byte V_GND_PIN = 1;    // PB1 (Pin 6) - Γείωση διαιρέτη
uint16_t timer_8s_counts = 0;
const uint16_t TARGET_COUNTS =    113; // ~15 λεπτά

ISR(WDT_vect) { }

void setup() {
  pinMode(LEDS_PIN, OUTPUT);
  
  // Αρχικοποίηση αχρησιμοποίητων Pins
  pinMode(0, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  
  // Έλεγχος Μπαταρίας (< 2.8V -> Shutdown)
  if (get_battery_adc() < 645) { 
    // Πιο έντονη ειδοποίηση: 20 αναβοσβήματα
    for(byte i=0; i<20; i++) {
      digitalWrite(LEDS_PIN, HIGH); 
      delay(100);
      digitalWrite(LEDS_PIN, LOW); 
      delay(100);
    }
    shutdown_completely(); 
  }

  digitalWrite(LEDS_PIN, HIGH); // Άναμμα
}

void loop() {
  if (timer_8s_counts < TARGET_COUNTS) {
    setup_watchdog(9); // 8 δευτερόλεπτα
    go_to_sleep();
    timer_8s_counts++;
  } else {
    shutdown_completely();
  }
}

uint16_t get_battery_adc() {
  // 1. Ενεργοποίηση διαιρέτη (Γείωση μέσω PB4)
  pinMode(V_GND_PIN, OUTPUT);
  digitalWrite(V_GND_PIN, LOW);
  
  // 2. Ρύθμιση ADC: Internal 1.1V Ref και είσοδος ADC1 (PB2)
  ADMUX = (1 << REFS0) | (1 << MUX0); 
  ADCSRA |= (1 << ADEN); 
  delay(10); // Σταθεροποίηση αναφοράς
  
  ADCSRA |= (1 << ADSC); // Έναρξη μέτρησης
  while (ADCSRA & (1 << ADSC));
  uint16_t val = ADC;

  // 3. Απενεργοποίηση για 0μA κατανάλωση
  ADCSRA &= ~(1 << ADEN);
  digitalWrite(V_GND_PIN, HIGH); // Διακοπή ροής στον διαιρέτη
  pinMode(V_GND_PIN, INPUT);
  
  return val;
}

void go_to_sleep() {
  ADCSRA &= ~(1 << ADEN);
  ACSR |= (1 << ACD);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
  sleep_disable();
}

void shutdown_completely() {
  digitalWrite(LEDS_PIN, LOW);
  wdt_disable();
  while(1) { go_to_sleep(); }
}

void setup_watchdog(uint8_t prescaler) {
  uint8_t bb = prescaler & 7;
  if (prescaler > 7) bb |= (1 << 5);
  cli();
  wdt_reset();
  WDTCR |= (1 << WDCE) | (1 << WDE);
  WDTCR = bb | (1 << WDTIE);
  sei();
}
