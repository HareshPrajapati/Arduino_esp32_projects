#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Update.h>
#include "Wire.h"
#include "WiFi.h"
#include "WiFiClientSecure.h"

void updateHandler(void *pParameters);
TaskHandle_t task;
bool Start = true , startUpdateProcess = false;
uint8_t newBuff[4];
uint8_t newBuff2[128];

void setup()
{
    Serial.begin(921600UL);
    Serial2.begin(921600UL);
    Serial.println();
    Serial.println(Serial2.getWriteError());
    pinMode(2, OUTPUT);
    xTaskCreatePinnedToCore(updateHandler, "Updater", 10000, NULL, 2, &task, 1); // pin task to core 1

}

void loop() {
    digitalWrite(LED_BUILTIN,HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN,LOW);
    delay(500);
}




void updateHandler(void *pParameters)
{
    Serial.printf("downloader running on core....%u", xPortGetCoreID());
    for (;;)
    {
        size_t aval = Serial2.available();
        if (aval)
        {
            if (Start)
            {
                Serial.print("aval : ");
                Serial.println(aval);
                size_t readed = Serial2.readBytes(newBuff, sizeof(newBuff));
                uint32_t fileSize;
                memcpy(&fileSize, newBuff, 4);
                Serial.print("read Size : ");
                Serial.println(fileSize);
                Update.begin(fileSize);
                Start = false;
                startUpdateProcess = true;
            }
        }

        if (startUpdateProcess)
        {
            size_t aval = Serial2.available();
            if (aval)
            {   
                // Serial2.readBytes(newBuff2, sizeof(newBuff2));
                size_t written = Update.writeStream(Serial2);
                Serial.print("written  : ");
                Serial.println(written);
                if (Update.end())
                {
                    Serial.println("OTA done!");
                    if (Update.isFinished()) {   
                        startUpdateProcess = false;
                        Serial.println("Update successfully completed. rebooting...");
                        ESP.restart();
                    }
                    else
                    {
                        Serial.println("Update not finished? Something went wrong!");
                    }
                }
            }
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}