#include <Arduino.h>
#include <avr/interrupt.h>

constexpr uint8_t PIN_NRZ_MASK = _BV(3);          // PORTE3, digital pin 5
constexpr uint8_t PIN_NRZI_MASK = _BV(3);         // PORTH3, digital pin 6
constexpr uint8_t PIN_MANCHESTER_MASK = _BV(4);   // PORTH4, digital pin 7
constexpr uint8_t PORT_H_MASK = PIN_NRZI_MASK | PIN_MANCHESTER_MASK;

constexpr uint8_t BIT_SEQUENCE[] = {0, 1, 0, 1, 1, 0, 1};
constexpr uint8_t BIT_COUNT = sizeof(BIT_SEQUENCE);

constexpr unsigned long INTER_TX_DELAY_MS = 10;
constexpr uint16_t TIMER1_COMPARE = 49;  // 25 µs with 16 MHz clock, prescaler 8

volatile bool isTransmitting = false;
volatile bool halfPhase = false;
volatile uint8_t currentBit = 0;
volatile uint8_t nrziLevel = 0;
volatile bool transmissionCompleted = false;

volatile uint8_t portEShadow = 0;
volatile uint8_t portHShadow = 0;

unsigned long nextStartMillis = 0;

void startTransmission() {
  noInterrupts();

  currentBit = 0;
  halfPhase = false;
  nrziLevel = 0;
  transmissionCompleted = false;

  portEShadow &= ~PIN_NRZ_MASK;
  portHShadow &= ~PORT_H_MASK;
  PORTE = portEShadow;
  PORTH = portHShadow;

  TCNT1 = 0;
  isTransmitting = true;
  TIMSK1 |= _BV(OCIE1A);

  interrupts();
}

ISR(TIMER1_COMPA_vect) {
  if (!isTransmitting) {
    return;
  }

  const uint8_t bit = BIT_SEQUENCE[currentBit];

  if (!halfPhase) {
    // First half-bit window.
    portEShadow = (portEShadow & ~PIN_NRZ_MASK) | (bit ? PIN_NRZ_MASK : 0);
    PORTE = portEShadow;

    if (bit) {
      nrziLevel ^= 1;
    }

    uint8_t newH = (portHShadow & ~PORT_H_MASK);
    if (nrziLevel) {
      newH |= PIN_NRZI_MASK;
    }
    if (bit) {
      newH |= PIN_MANCHESTER_MASK;
    }
    portHShadow = newH;
    PORTH = portHShadow;

    halfPhase = true;
  } else {
    // Second half-bit window.
    uint8_t newH = portHShadow & ~PIN_MANCHESTER_MASK;
    if (!bit) {
      newH |= PIN_MANCHESTER_MASK;
    }
    portHShadow = newH;
    PORTH = portHShadow;

    halfPhase = false;
    ++currentBit;

    if (currentBit >= BIT_COUNT) {
      portEShadow &= ~PIN_NRZ_MASK;
      portHShadow &= ~PORT_H_MASK;
      PORTE = portEShadow;
      PORTH = portHShadow;

      isTransmitting = false;
      transmissionCompleted = true;
      TIMSK1 &= ~_BV(OCIE1A);
    }
  }
}

void setupTimer() {
  noInterrupts();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = TIMER1_COMPARE;
  TCCR1B |= _BV(WGM12);  // CTC mode
  TCCR1B |= _BV(CS11);   // prescaler 8
  TIMSK1 = 0;

  interrupts();
}

void setup() {
  noInterrupts();

  DDRE |= PIN_NRZ_MASK;
  DDRH |= PORT_H_MASK;

  portEShadow = PORTE & ~PIN_NRZ_MASK;
  portHShadow = PORTH & ~PORT_H_MASK;
  PORTE = portEShadow;
  PORTH = portHShadow;

  interrupts();

  setupTimer();
}

void loop() {
  if (transmissionCompleted) {
    noInterrupts();
    transmissionCompleted = false;
    interrupts();

    nextStartMillis = millis() + INTER_TX_DELAY_MS;
  }

  if (!isTransmitting && millis() >= nextStartMillis) {
    startTransmission();
  }
}

