#include <Arduino.h>
bool Start = true , startUpdateProcess = false;
uint8_t newBuff[4];
int ledState = LOW;                                      // ledState used to set the LED
long previousMillis = 0;                                 // will store last time LED was updated
long interval = 250;                                     // interval at which to blink

void setup() {
  Serial.begin(921600UL);
  pinMode(D2,OUTPUT);
}

void loop() {
  Serial.print("250 ms blink \r\n");
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    if (ledState == LOW) {
        ledState = HIGH;
    } else {
        ledState = LOW;
    }
    digitalWrite(D2, ledState);                            // set the LED with the ledState of the variable:
  }

  size_t aval = Serial.available();
  if (aval) {
    if (Start) {
      Serial.print("aval : ");
      Serial.println(aval);
      size_t readed = Serial.readBytes(newBuff, sizeof(newBuff));
      uint32_t fileSize;
      memcpy(&fileSize, newBuff, 4);
      Serial.print("read Size : ");
      Serial.println(fileSize);
      Update.begin(fileSize);
      Start = false;
      startUpdateProcess = true;
    }
  }

  if (startUpdateProcess) {
    size_t aval = Serial.available();
    if (aval) {
      size_t written = Update.writeStream(Serial);
      Serial.print("written  : ");
      Serial.println(written);
      if (Update.end()) {
        Serial.println("OTA done!");
        if (Update.isFinished()) {
          startUpdateProcess = false;
          Serial.println("Update successfully completed. rebooting...");
          ESP.restart();
        } else {
          Serial.println("Update not finished? Something went wrong!");
        }
      }
    }
  }
}