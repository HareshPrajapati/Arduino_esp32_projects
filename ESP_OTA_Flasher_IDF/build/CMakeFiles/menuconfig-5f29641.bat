@echo off
cd /D C:\Users\Haesh_MikoToniks\Documents\PlatformIO\Projects\ESP_OTA_Flasher_IDF\build || (set FAIL_LINE=2& goto :ABORT)
C:\Users\Haesh_MikoToniks\.espressif\python_env\idf4.0_py3.7_env\Scripts\python.exe C:/Users/Haesh_MikoToniks/esp-idf/tools/kconfig_new/confgen.py --kconfig C:/Users/Haesh_MikoToniks/esp-idf/Kconfig --sdkconfig-rename C:/Users/Haesh_MikoToniks/esp-idf/sdkconfig.rename --config C:/Users/Haesh_MikoToniks/Documents/PlatformIO/Projects/ESP_OTA_Flasher_IDF/sdkconfig --env-file C:/Users/Haesh_MikoToniks/Documents/PlatformIO/Projects/ESP_OTA_Flasher_IDF/build/config.env --env IDF_TARGET=esp32 --dont-write-deprecated --output config C:/Users/Haesh_MikoToniks/Documents/PlatformIO/Projects/ESP_OTA_Flasher_IDF/sdkconfig || (set FAIL_LINE=3& goto :ABORT)
"C:\Program Files\CMake\bin\cmake.exe" -E env "COMPONENT_KCONFIGS=C:/Users/Haesh_MikoToniks/esp-idf/components/app_trace/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/bt/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/driver/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/efuse/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/esp-tls/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/esp32/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/esp_adc_cal/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/esp_common/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/esp_eth/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/esp_event/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/esp_gdbstub/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/esp_http_client/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/esp_http_server/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/esp_https_ota/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/esp_https_server/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/esp_wifi/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/espcoredump/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/fatfs/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/freemodbus/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/freertos/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/heap/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/libsodium/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/log/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/lwip/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/mbedtls/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/mdns/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/mqtt/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/newlib/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/nvs_flash/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/openssl/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/pthread/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/spi_flash/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/spiffs/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/tcpip_adapter/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/unity/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/vfs/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/wear_levelling/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/wifi_provisioning/Kconfig C:/Users/Haesh_MikoToniks/esp-idf/components/wpa_supplicant/Kconfig" "COMPONENT_KCONFIGS_PROJBUILD=C:/Users/Haesh_MikoToniks/esp-idf/components/app_update/Kconfig.projbuild C:/Users/Haesh_MikoToniks/esp-idf/components/bootloader/Kconfig.projbuild C:/Users/Haesh_MikoToniks/esp-idf/components/esptool_py/Kconfig.projbuild C:/Users/Haesh_MikoToniks/esp-idf/components/partition_table/Kconfig.projbuild" IDF_CMAKE=y KCONFIG_CONFIG=C:/Users/Haesh_MikoToniks/Documents/PlatformIO/Projects/ESP_OTA_Flasher_IDF/sdkconfig IDF_TARGET=esp32 C:/Users/Haesh_MikoToniks/.espressif/tools/mconf/v4.6.0.0-idf-20190628/mconf-idf.exe C:/Users/Haesh_MikoToniks/esp-idf/Kconfig || (set FAIL_LINE=4& goto :ABORT)
C:\Users\Haesh_MikoToniks\.espressif\python_env\idf4.0_py3.7_env\Scripts\python.exe C:/Users/Haesh_MikoToniks/esp-idf/tools/kconfig_new/confgen.py --kconfig C:/Users/Haesh_MikoToniks/esp-idf/Kconfig --sdkconfig-rename C:/Users/Haesh_MikoToniks/esp-idf/sdkconfig.rename --config C:/Users/Haesh_MikoToniks/Documents/PlatformIO/Projects/ESP_OTA_Flasher_IDF/sdkconfig --env-file C:/Users/Haesh_MikoToniks/Documents/PlatformIO/Projects/ESP_OTA_Flasher_IDF/build/config.env --env IDF_TARGET=esp32 --output config C:/Users/Haesh_MikoToniks/Documents/PlatformIO/Projects/ESP_OTA_Flasher_IDF/sdkconfig || (set FAIL_LINE=5& goto :ABORT)
goto :EOF

:ABORT
set ERROR_CODE=%ERRORLEVEL%
echo Batch file failed at line %FAIL_LINE% with errorcode %ERRORLEVEL%
exit /b %ERROR_CODE%