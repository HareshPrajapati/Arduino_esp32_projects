#include <Arduino.h>

void setup() {
 Serial.begin(921600UL);
}

void loop() {
  Serial.println("hello world");
}