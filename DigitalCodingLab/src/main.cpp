#include <Arduino.h>

constexpr uint8_t PIN_NRZ = 5;
constexpr uint8_t PIN_MANCHESTER = 6;
constexpr unsigned long BIT_PERIOD_US = 50;
constexpr unsigned long HALF_BIT_PERIOD_US = BIT_PERIOD_US / 2;
constexpr unsigned long INTERFRAME_DELAY_MS = 10;

constexpr uint8_t DATA_BITS[] = {1, 0, 0, 0, 0, 0, 0, 0,
                                 1, 0, 0, 0, 0, 0, 0, 1};
constexpr size_t DATA_LENGTH = sizeof(DATA_BITS) / sizeof(DATA_BITS[0]);
constexpr uint8_t MAX_CONSECUTIVE_ZEROS = 6;
constexpr bool PRINT_STUFFED_SEQUENCE = false;
constexpr size_t MAX_STUFFED_BITS =
    DATA_LENGTH + (DATA_LENGTH / MAX_CONSECUTIVE_ZEROS) + 1;

uint8_t stuffedBits[MAX_STUFFED_BITS];
size_t stuffedLength = 0;

void transmitManchester(uint8_t bit) {
  const uint8_t firstHalf = bit ? HIGH : LOW;
  const uint8_t secondHalf = bit ? LOW : HIGH;

  digitalWrite(PIN_MANCHESTER, firstHalf);
  delayMicroseconds(HALF_BIT_PERIOD_US);
  digitalWrite(PIN_MANCHESTER, secondHalf);
  delayMicroseconds(HALF_BIT_PERIOD_US);
}

void transmitDataBit(uint8_t bit) {
  digitalWrite(PIN_NRZ, bit ? HIGH : LOW);
  transmitManchester(bit);
}

void transmitStuffedNRZBit() {
  digitalWrite(PIN_NRZ, HIGH);
  delayMicroseconds(BIT_PERIOD_US);
}

void encodeFrame() {
  stuffedLength = 0;
  uint8_t zeroCount = 0;

  for (size_t i = 0; i < DATA_LENGTH; ++i) {
    const uint8_t bit = DATA_BITS[i];
    stuffedBits[stuffedLength++] = bit;

    if (bit == 0) {
      ++zeroCount;
      if (zeroCount == MAX_CONSECUTIVE_ZEROS) {
        stuffedBits[stuffedLength++] = 1;
        zeroCount = 0;
      }
    } else {
      zeroCount = 0;
    }
  }
}

void logStuffedSequence() {
  if (!PRINT_STUFFED_SEQUENCE) {
    return;
  }

  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  Serial.print(F("Stuffed bits ("));
  Serial.print(stuffedLength);
  Serial.println(F("):"));

  for (size_t i = 0; i < stuffedLength; ++i) {
    Serial.print(stuffedBits[i]);
  }
  Serial.println();
}

void sendFrame() {
  uint8_t zeroCount = 0;

  for (size_t i = 0; i < DATA_LENGTH; ++i) {
    const uint8_t bit = DATA_BITS[i];
    transmitDataBit(bit);

    if (bit == 0) {
      ++zeroCount;
      if (zeroCount == MAX_CONSECUTIVE_ZEROS) {
        transmitStuffedNRZBit();
        zeroCount = 0;
      }
    } else {
      zeroCount = 0;
    }
  }

  digitalWrite(PIN_NRZ, LOW);
  digitalWrite(PIN_MANCHESTER, LOW);
}

void setup() {
  pinMode(PIN_NRZ, OUTPUT);
  pinMode(PIN_MANCHESTER, OUTPUT);

  digitalWrite(PIN_NRZ, LOW);
  digitalWrite(PIN_MANCHESTER, LOW);

  encodeFrame();
  logStuffedSequence(); // Optional serial trace for debugging bit stuffing.
}

void loop() {
  sendFrame();
  delay(INTERFRAME_DELAY_MS);
}
