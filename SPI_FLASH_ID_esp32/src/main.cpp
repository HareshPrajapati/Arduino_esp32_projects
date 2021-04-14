#include <Arduino.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <esp_spi_flash.h>
#define MEMORY 5 // Chip select pin

void setup() {
  Serial.begin(921600UL);
  spi_flash_init();
  size_t var = spi_flash_get_chip_size();
  Serial.println(var);
  // SPI.begin();
  // SPI.setDataMode(SPI_MODE0);
  // SPI.transfer(0x90);
  // SPI.transfer(0x00);
  // SPI.transfer(0x00);
  // SPI.transfer(0x00);
  // byte response1 = SPI.transfer( 0x00);
  // byte response2 = SPI.transfer(0x00);

  // Serial.print("Response: ");
  // Serial.println(response1);
  // Serial.println(response2);

}

void loop() {
  // put your main code here, to run repeatedly:
}