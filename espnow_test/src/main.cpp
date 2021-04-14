<<<<<<< HEAD
#include <Arduino.h>

void setup() {
  // Init Serial Monitor
  Serial.begin(921600UL);
  pinMode(2,OUTPUT);
  

}

void loop() {

digitalWrite(2,HIGH);
delay(500);
digitalWrite(2,LOW);
delay(500);

=======
#include <Arduino.h>

void setup() {
  // Init Serial Monitor
  Serial.begin(921600UL);
  pinMode(2,OUTPUT);
  

}

void loop() {

digitalWrite(2,HIGH);
delay(500);
digitalWrite(2,LOW);
delay(500);

>>>>>>> 36b003facc6f6b0250deccaa20cc6805e5ccb6b7
}