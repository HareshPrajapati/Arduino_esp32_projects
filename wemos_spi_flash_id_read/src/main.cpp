#include <Arduino.h>
#include <spi_flash.h>
#include <SPI.h>
byte  responce1;
byte  responce2;


void setup() {
  Serial.begin(921600UL);
  SPI.begin();

  // digitalWrite(PIN_SPI_SS,LOW);
  SPI.transfer(0x90);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  responce1 = SPI.transfer(0x00);
  responce2 = SPI.transfer(0x00);
  //  digitalWrite(PIN_SPI_SS,HIGH);

}

void loop() {
  Serial.print("Response: ");
  Serial.println(responce1);
  Serial.println(responce2);
}