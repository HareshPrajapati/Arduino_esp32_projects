#include <Arduino.h>
#include <WiFi.h>
#include "FTPserver.h"
#include <SPIFFS.h>

#define RESET_PIN 4

void formatDevice();

const char* ssid = "Mikrotroniks_Jio";
const char* password = "51251251";

WiFiServer server(80);

FTPserver FTPserver(RESET_PIN);

void setup()
{
  Serial.begin(921600);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String line = client.readStringUntil('\r');
    int HTTPopt = FTPserver.OptionCall(line);
    String urlParam = FTPserver.URLparams(line);
    if(HTTPopt == FTPserver.optIndex)
    {
      FTPserver.Index(&client);
    }
    else if(HTTPopt == FTPserver.optDelete)
    {
      FTPserver.DeleteFile(&client, urlParam);
    }
    else if(HTTPopt == FTPserver.optFlash)
    {
      FTPserver.FlashFile(&client, urlParam);
    }
    else if(HTTPopt == FTPserver.optList)
    {
      FTPserver.FileList(&client);
    }
    else if(HTTPopt == FTPserver.optUpload)
    {

      FTPserver.FileUpload(&client, urlParam);
    }
    else
    {
      FTPserver.Index(&client);
    }
    delay(1);
    client.stop();
  }
}

void formatDevice() {
  SPIFFS.begin();
  SPIFFS.format();
  SPIFFS.end();
}


