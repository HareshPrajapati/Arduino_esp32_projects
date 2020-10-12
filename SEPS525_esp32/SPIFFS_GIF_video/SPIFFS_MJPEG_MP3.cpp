/* Video src: https://youtu.be/iN9PV8e-Rh0 */

#define MJPEG_FILENAME "/harry1.mjpeg"
#define FPS 24
#define MJPEG_BUFFER_SIZE (220 * 176 * 2 / 4)
// #define MJPEG_BUFFER_SIZE (320 * 240 * 2 / 4)
// #define FPS 12
#include <Arduino.h>
#include "Adafruit_I2CDevice.h"
#include <SPI.h>
#include "unity.h"
#include <WiFi.h>
#include <SPIFFS.h>


#include <Arduino_ESP32SPI.h>
#include <Arduino_Display.h>
#define TFT_BRIGHTNESS 128
#if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5STACK_FIRE)
#define TFT_BL 32
#define SS 4
Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(27 /* DC */, 14 /* CS */, SCK, MOSI, MISO);
Arduino_ILI9341_M5STACK *gfx = new Arduino_ILI9341_M5STACK(bus, 33 /* RST */, 1 /* rotation */);
#elif defined(ARDUINO_ODROID_ESP32)
#define TFT_BL 14
Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(21 /* DC */, 5 /* CS */, SCK, MOSI, MISO);
// Arduino_ILI9341 *gfx = new Arduino_ILI9341(bus, -1 /* RST */, 3 /* rotation */);
Arduino_ST7789 *gfx = new Arduino_ST7789(bus, -1 /* RST */, 1 /* rotation */, true /* IPS */);
#elif defined(ARDUINO_T) // TTGO T-Watch
#define TFT_BL 12
Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(27 /* DC */, 5 /* CS */, SCK, MOSI, MISO);
Arduino_ST7789 *gfx = new Arduino_ST7789(bus, -1 /* RST */, 2 /* rotation */, true /* IPS */, 240, 240, 0, 80);
#else /* not a specific hardware */
// ST7789 Display
// #define TFT_BL 22
// Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(15 /* DC */, 12 /* CS */, 18 /* SCK */, 23 /* MOSI */, -1 /* MISO */);
// Arduino_ST7789 *gfx = new Arduino_ST7789(bus, -1 /* RST */, 2 /* rotation */, true /* IPS */, 240 /* width */, 240 /* height */, 0 /* col offset 1 */, 80 /* row offset 1 */);
// ILI9225 Display
#define TFT_BL 22
Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(2/* DC */, 15/* CS */, 14 /* SCK */, 13 /* MOSI */, 12/* MISO */);
Arduino_SEPS525 *gfx = new Arduino_SEPS525(bus, 33 /* RST */,0 /* rotation */);
#endif /* not a specific hardware */

#include "MjpegClass.h"
static MjpegClass mjpeg;

int next_frame = 0;
int skipped_frames = 0;
unsigned long total_sd_mjpeg = 0;
unsigned long total_decode_video = 0;
unsigned long total_remain = 0;
unsigned long start_ms, curr_ms, next_frame_ms;



void setup()
{
  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);

  // Init Video
  gfx->begin();
  gfx->fillScreen(BLACK);

#ifdef TFT_BL
  ledcAttachPin(TFT_BL, 1);     // assign TFT_BL pin to channel 1
  ledcSetup(1, 12000, 8);       // 12 kHz PWM, 8-bit resolution
  ledcWrite(1, TFT_BRIGHTNESS); // brightness 0 - 255
#endif

  // Init SPIFFS
  if (!SPIFFS.begin())
  {
    Serial.println(F("ERROR: mount failed!"));
    gfx->println(F("ERROR:  mount failed!"));
  }
  else
  {


    File vFile = SPIFFS.open(MJPEG_FILENAME);
    if (!vFile || vFile.isDirectory())
    {
      Serial.println(F("ERROR: Failed to open " MJPEG_FILENAME " file for reading"));
      gfx->println(F("ERROR: Failed to open " MJPEG_FILENAME " file for reading"));
    }
    else
    {
      uint8_t *mjpeg_buf = (uint8_t *)malloc(MJPEG_BUFFER_SIZE);
      if (!mjpeg_buf)
      {
        Serial.println(F("mjpeg_buf malloc failed!"));
      }
      else
      {
        Serial.println(F("MJPEG video start"));
        start_ms = millis();
        curr_ms = millis();
        next_frame_ms = start_ms + (++next_frame * 1000 / FPS / 2);

        mjpeg.setup(vFile, mjpeg_buf, gfx, false);


        unsigned long start = millis();
        // Read video
        while (mjpeg.readMjpegBuf())
        {
          total_sd_mjpeg += millis() - curr_ms;
          curr_ms = millis();

          if (millis() < next_frame_ms) // check show frame or skip frame
          {
            // Play video
            mjpeg.drawJpg();
            total_decode_video += millis() - curr_ms;

            int remain_ms = next_frame_ms - millis();
            if (remain_ms > 0)
            {
              total_remain += remain_ms;
              delay(remain_ms);
            }
          }
          else
          {
            ++skipped_frames;
            Serial.println(F("Skip frame"));
          }

          curr_ms = millis();
          next_frame_ms = start_ms + (++next_frame * 1000 / FPS);
        }
        vFile.close();

        // int time_used = millis() - start_ms;
        // int played_frames = next_frame - 1 - skipped_frames;
        // float fps = 1000.0 * played_frames / time_used;

      }
    }
  }
#ifdef TFT_BL
  delay(1240000);
  ledcDetachPin(TFT_BL);
#endif
  gfx->displayOff();
  esp_deep_sleep_start();
}

void loop()
{
}
