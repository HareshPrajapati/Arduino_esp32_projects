#include <Arduino.h>
#include <FS.h>


void updateHandler(void *pParameters);
bool Start = true , startUpdateProcess = false;
uint8_t newBuff[4];
int ledState = LOW;             // ledState used to set the LED
long previousMillis = 0;        // will store last time LED was updated
long interval = 500;           // interval at which to blink 

void setup()
{
    Serial1.begin(921600UL);
    Serial.begin(921600UL);
    Serial1.println();
    Serial1.println("Serial1 ready for debug");
    pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis > interval) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;   
 
    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;
 
    // set the LED with the ledState of the variable:
    digitalWrite(LED_BUILTIN, ledState);
  }

  size_t aval = Serial.available();
  if (aval)
  {
    if (Start)
    {
      Serial1.print("aval : ");
      Serial1.println(aval);
      size_t readed = Serial1.readBytes(newBuff, sizeof(newBuff));
      uint32_t fileSize;
      memcpy(&fileSize, newBuff, 4);
      Serial1.print("read Size : ");
      Serial1.println(fileSize);
      Update.begin(fileSize);
      Start = false;
      startUpdateProcess = true;
    }
  }

  if (startUpdateProcess)
  {
    size_t aval = Serial.available();
    if (aval)
    {
      // Serial1.readBytes(newBuff2, sizeof(newBuff2));
      size_t written = Update.writeStream(Serial1);
      Serial1.print("written  : ");
      Serial1.println(written);
      if (Update.end())
      {
        Serial1.println("OTA done!");
        if (Update.isFinished())
        {
          startUpdateProcess = false;
          Serial1.println("Update successfully completed. rebooting...");
          ESP.restart();
        }
        else
        {
          Serial1.println("Update not finished? Something went wrong!");
        }
      }
    }
  }
}


void updateHandler(void *pParameters)
{
    for (;;)
    {
        size_t aval = Serial.available();
        if (aval)
        {
            if (Start)
            {
                Serial1.print("aval : ");
                Serial1.println(aval);
                size_t readed = Serial1.readBytes(newBuff, sizeof(newBuff));
                uint32_t fileSize;
                memcpy(&fileSize, newBuff, 4);
                Serial1.print("read Size : ");
                Serial1.println(fileSize);
                Update.begin(fileSize);
                Start = false;
                startUpdateProcess = true;
            }
        }

        if (startUpdateProcess)
        {
            size_t aval = Serial.available();
            if (aval) {   
                // Serial1.readBytes(newBuff2, sizeof(newBuff2));
                size_t written = Update.writeStream(Serial1);
                Serial.print("written  : ");
                Serial.println(written);
                if (Update.end())
                {
                    Serial1.println("OTA done!");
                    if (Update.isFinished()) {   
                        startUpdateProcess = false;
                        Serial1.println("Update successfully completed. rebooting...");
                        ESP.restart();
                    }
                    else
                    {
                        Serial1.println("Update not finished? Something went wrong!");
                    }
                }
            }
        }
    }
}