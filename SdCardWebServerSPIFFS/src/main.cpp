#include <Arduino.h>
#include <SD.h>
#include "UDHttp.h"



const char* ssid     = "Mikrotroniks_Jio";
const char* password = "51251251";

File root;
//these callbacks will be invoked to read and write data to sdcard
//and process response
//and showing progress 
int responsef(uint8_t *buffer, int len){
  Serial.printf("%s\n", buffer);
  return 0;
}

int rdataf(uint8_t *buffer, int len){
  //read file to upload
  if (root.available()) {
    return root.read(buffer, len);
  }
  return 0;
}

int wdataf(uint8_t *buffer, int len){
  //write downloaded data to file
  return root.write(buffer, len);
}

void progressf(int percent){
  Serial.printf("%d\n", percent);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(921600UL);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
    
  Serial.print("Initializing SD card...");
  if (!SD.begin(SS)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  SD.remove("/photo1.pdf");
  {
    UDHttp udh;
    //open file on sdcard to write
    root = SD.open("/photo1.pdf", FILE_WRITE);
    if (!root) {
       Serial.println("can not open file!");
       return;
    }
    //download the file from url
    udh.download("http://www.smart-words.org/linking-words/linking-words.pdf", wdataf, progressf);
    root.close();
    Serial.printf("done downloading\n");
  }

}

void loop() {
  // put your main code here, to run repeatedly:

}



