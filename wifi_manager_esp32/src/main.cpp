#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>  //ESP8266 Core WiFi Library         
#else
#include <WiFi.h>      //ESP32 Core WiFi Library    
#endif

#if defined(ESP8266)

#include <ESP8266WebServer.h>  //Local WebServer used to serve the configuration portal

#else

#include <WebServer.h> //Local DNS Server used for redirecting all requests to the configuration portal (  https://github.com/zhouhan0126/DNSServer---esp32  )
#endif

#include <DNSServer.h> //Local WebServer used to serve the configuration portal (  https://github.com/zhouhan0126/DNSServer---esp32  )
#include <WiFiManager.h>   // WiFi Configuration Magic (  https://github.com/zhouhan0126/DNSServer---esp32  ) >>  https://github.com/zhouhan0126/DNSServer---esp32  (ORIGINAL)

const int PIN_AP = 2;
//callback
void configModeCallback (WiFiManager *myWiFiManager) {  
 Serial.println("Entered config mode");

  Serial.println(WiFi.softAPIP()); //imprime o IP do AP
  Serial.println(myWiFiManager->getConfigPortalSSID()); 
}
void saveConfigCallback () {
 Serial.println("Should save config");
  Serial.println(WiFi.softAPIP()); //imprime o IP do AP
}


void setup() {
 Serial.begin(115200);
  pinMode(PIN_AP, INPUT);
  //declar  wifiManager
  WiFiManager wifiManager;


//  wifiManager.resetSettings();
 

  wifiManager.setAPCallback(configModeCallback); 

  wifiManager.setSaveConfigCallback(saveConfigCallback); 
 

  wifiManager.autoConnect("myesp", "11111111");
}

void loop() {
  
  WiFiManager wifiManager;
  
   if ( digitalRead(PIN_AP) == HIGH  ) {
      
      Serial.println("restart"); 
      if(!wifiManager.startConfigPortal("myesp", "11111111") ){
        
        delay(500);
        ESP.restart();
        delay(400);
      }
      Serial.println("Conected ESP_AP!!!");
}
}

