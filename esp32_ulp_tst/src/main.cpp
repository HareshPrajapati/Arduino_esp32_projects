
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <soc/rtc.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/rtc_io_reg.h>
#include <soc/soc_ulp.h>
#include <esp32/ulp.h>
#include <Arduino.h>

const gpio_num_t LED_PIN = GPIO_NUM_2;

void blink(uint32_t onTime, uint32_t offTime, uint8_t repeat = 1, bool reverse = true)
{
    while (repeat--)
    {
        rtc_gpio_set_level(LED_PIN, !reverse);
        delay(onTime);
        rtc_gpio_set_level(LED_PIN, reverse);
        delay(offTime);
    }
}

void blink_ulp(uint32_t onTime, uint32_t offTime, uint8_t repeat = 1)
{
    const uint16_t ULP_DATA_OFFSET = 0;
    const uint16_t ULP_CODE_OFFSET = 3;

    const ulp_insn_t program[] = {
        I_MOVI(R1, ULP_DATA_OFFSET),                                           // R1 <= @DATA
        I_LD(R2, R1, 2),                                                       // R2 <= repeat
        M_LABEL(1),                                                            // cycle:
        I_WR_REG_BIT(RTC_GPIO_OUT_W1TC_REG, RTC_GPIO_OUT_DATA_W1TC_S + 12, 1), // rtc_gpio_set_level(GPIO_NUM_2, LOW);
        I_LD(R3, R1, 0),                                                       // R3 <= onTime
        M_LABEL(3),                                                            // delay1:
        I_DELAY(25000),
        I_SUBI(R3, R3, 1),
        M_BXZ(4),
        M_BX(3),
        M_LABEL(4),
        I_WR_REG_BIT(RTC_GPIO_OUT_W1TS_REG, RTC_GPIO_OUT_DATA_W1TS_S + 12, 1), // rtc_gpio_set_level(GPIO_NUM_2, HIGH);
        I_LD(R3, R1, 1),                                                       // R3 <= offTime
        M_LABEL(5),                                                            // delay2:
        I_DELAY(25000),
        I_SUBI(R3, R3, 1),
        M_BXZ(6),
        M_BX(5),
        M_LABEL(6),
        I_SUBI(R2, R2, 1), // --repeat;
        M_BXZ(2),          // if (! repeat) break;
        M_BX(1),           // goto cycle
        M_LABEL(2),        // exit:
        I_MOVI(R3, 340),   // R3 <= 340
        M_LABEL(7),        // delay3:
        I_DELAY(25000),
        I_SUBI(R3, R3, 1),
        M_BXZ(8),
        M_BX(7),
        M_LABEL(8),
        I_RD_REG(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP_S, RTC_CNTL_RDY_FOR_WAKEUP_S),
        I_ANDI(R0, R0, 1),
        M_BXZ(8),
        I_WAKE(),
        I_END(),
        I_HALT()};
    size_t size = sizeof(program) / sizeof(ulp_insn_t);

    RTC_SLOW_MEM[ULP_DATA_OFFSET] = (RTC_FAST_CLK_FREQ_APPROX / 25000) * onTime / 1000;
    RTC_SLOW_MEM[ULP_DATA_OFFSET + 1] = (RTC_FAST_CLK_FREQ_APPROX / 25000) * offTime / 1000;
    RTC_SLOW_MEM[ULP_DATA_OFFSET + 2] = repeat;
    if (ulp_process_macros_and_load(ULP_CODE_OFFSET, program, &size) != ESP_OK)
    {
        Serial.println(F("Error loading ULP code!"));
        return;
    }
    esp_sleep_enable_ulp_wakeup();
    if (ulp_run(ULP_CODE_OFFSET) != ESP_OK)
    {
        Serial.println(F("Error running ULP code!"));
        return;
    }
    Serial.println(F("Going to sleep and execute ULP code..."));
    Serial.flush();
    esp_deep_sleep_start();
}


#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;
/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup(){
  Serial.begin(921600UL);
  delay(1000); 
  rtc_gpio_init(LED_PIN);
  rtc_gpio_set_direction(LED_PIN, RTC_GPIO_MODE_OUTPUT_ONLY);
  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  print_wakeup_reason();
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33,1); //1 = High, 0 = Low

  Serial.println("Going to sleep now");
  delay(1000);
  Serial.flush();
  esp_deep_sleep_start();
}

void loop(){
}
