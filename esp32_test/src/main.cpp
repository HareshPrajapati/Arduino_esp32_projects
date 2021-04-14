#include <Arduino.h>
#include <stdint.h>

#define digitCount(x)   ((x==0)?1:1+log10(x))

int countDigit(uint64_t num) {
  if (num == 0) {
    return 0;
  }
  return (1 + countDigit(num / 10));
}

void setup() {
  Serial.begin(921600UL);
  Serial.println();
  Serial.printf("number is :");
  Serial.println((int)digitCount(123456));
}

void loop() {
}