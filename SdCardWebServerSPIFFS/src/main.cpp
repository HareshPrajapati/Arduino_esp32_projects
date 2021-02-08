/*
  Sketch to generate the AsyncWeb Server (MSP_ESP32_OTA_FLASHER Using SD Card) for esp32 and GUI of
  File Manager Using LittleVgl graphics library and ILI9341 Display
  The sketch has been tested on the ESP32 and screen with XPT2046 driver for touch.
  web Server is based on Async HTTP and Web Server  ESP32 Arduino.
  created  By - Haresh Prajapati
*/

/************************************ Includes ***************************************************************/
#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <Ticker.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "GUI.h"
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <debug.h>
#include <Update.h>
#include <HexParser.h>
#include <Wire.h>

/***************************************  DEFINE  **************************************************************/

#define TFT_BL (22)

/***************************************  CONSTANT  *************************************************************/

const char *const WIFI_SSDI = "Mikrotroniks_Jio";
const char *const WIFI_PASS = "51251251";
const char *const DEFAULT_USER = "admin";
const char *const DEFAULT_PASS = "51251251";
const char *const host = "192.168.29.253";



/****************************************** CONSTANS FOR A PIC24 UPDATES ****************************************/
int lowAddess = 0, highAddress = 0, upperAddress = 0, extendedAddress = 0;
unsigned char resetData[11] = {0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 ,0x00};              //constant reset command buffer
unsigned char getversion[11] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};              //cosntant getVresion command buffer
unsigned char erashFlash[11] = {0x03, 0x52, 0x00, 0x55, 0x00, 0xAA, 0x00, 0x00, 0x18, 0x00, 0x00};              //constant erashFlash command buffer i.e : earsh size 0x5200 (20992) fix for PIC24FJ26GA70 

/****************************************  GLOBLES  *************************************************************/

String mainDirectory = "/", previousDir = "/", photoPath = "", parsedData;
bool downloadPending = false, currentFileOpen = false, fileNameChecker = false, firstByte = true;
size_t readChar = 0, readedPercentage = 0, finalPercentage = 0, currentPer = 0;
long readData, wb;
char *fileName;
uint8_t buff[1024] = {0}; // create buffer for read

/****************************************  GLOBLE OBJECTS  ******************************************************/

File uploadFile;           // File
WiFiClient *stream;        // Wifi Client for stream file
AsyncWebServer server(80); // Async web server object , for more  : https://github.com/me-no-dev/ESPAsyncWebServer
HTTPClient Http;           // HTTPClient
Ticker tick;               // timer for interrupt handler
TaskHandle_t task1, task2; // task handlers
SDcard mySd;               // Sd card object
TFT_Gui TFTGUI;            // LittleVgl GUI object

/***************************************  FUNCTION DECLARATION  *************************************************/

void serverSetup();
bool downloadFile(const char *const fileURL);
static void LvTickHandler(void);
void sdCardHandler(void *pParameters);
void printDirectory(AsyncWebServerRequest *request, String path = "/");
void openFile(AsyncWebServerRequest *request, String dlPath);
void doDelete(AsyncWebServerRequest *request);
void doShare(AsyncWebServerRequest *request);
void deleteConfirm(AsyncWebServerRequest *request);
String getContentType(AsyncWebServerRequest *request, String filename);
void downloadhandler(void *pParameters);

/****************************************************************************************************************/

void setup()
{
  /*************************************  Debuger InIt  ***********************************************************/

  DEBUG_INIT();
  DEBUG_SET_OUTPUT();
  Serial2.begin(921600UL);

  /*************************************  SPIFFS Initialization  **************************************************/

  if (!SPIFFS.begin()) { 
    // SPIFFS For html files of web browser
    DEBUG_SPIFFS_NL("Error to open SPIFFS file !!!! ");
    ESP.restart();
  } else {
    DEBUG_SPIFFS_NL("SPIFFS File open Successfully");
  }

  /*************************************  SD Card and LittleVgl GUI  ************************************************/

  if (!SD.begin(SS)) { 
    // SD card for read, write and save files
    DEBUG_SD_CARD_NL("Card failed or not present..");
    TFTGUI.lvErrorPage();
    mySd.SDPresent = false;
  } else {
    DEBUG_SD_CARD_NL("Card initialised... file access enabled...");
    guiInIt(); //LIttleVgl GUI init
    TFTGUI.lvFileBrowser();
    mySd.SDPresent = true;
  }
  tick.attach_ms(LVGL_TICK_PERIOD, LvTickHandler);                                                                       // Initialize the graphics library's tick
  xTaskCreatePinnedToCore(sdCardHandler, "TFT GUI", 10000, NULL, 1, &task2, 0);                                          // pin task to core 0

  /*************************************  LCD BackLight of ILI9341 Display  ***********************************************/

  ledcAttachPin(TFT_BL, 1);  // assign TFT_BL pin to channel 1
  ledcSetup(1, 12000UL, 10); // 12 kHz PWM, 10-bit resolution
  analogReadResolution(10);  // 10 bit resolution
  ledcWrite(1, 768U);        // brightness

  /*************************************  WiFi Initialization  **********************************************************/

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSDI, WIFI_PASS); // connect to wifi
  DEBUG_MAIN_NL("Connecting to %s.... ", WIFI_SSDI);
  uint8_t wifiRetry = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetry++ < 20) {
    delay(500);
  }
  if (wifiRetry == 21) {
    DEBUG_MAIN_NL("Could not connect to %s", WIFI_SSDI);
    ESP.restart();
  }
  DEBUG_MAIN_NL("Connected! IP address: %s ", WiFi.localIP().toString().c_str());

  /************************************** MSDN Initialization  ***********************************************************/

  if (MDNS.begin(host)) { 
    // MDNS begin
    MDNS.addService("http", "tcp", 80);
    DEBUG_MAIN_NL("MDNS responder started");
    DEBUG_MAIN_NL("You can now connect to http://%s/", host);
  }

  /***************************************  Server Setup   ***************************************************************/

  serverSetup();                                                                 // server setup for events requests
  xTaskCreatePinnedToCore(downloadhandler, "Downloader", 10000, NULL, 2, &task1, 1); // pin task to core 1
}

void loop() {   
  if (LedShareFileOpen) {
      static String sdfileName;
      if (fileNameChecker) { 
        String tempName = uploadFile.name();
        sdfileName = tempName.substring((tempName.length() - 3), tempName.length()).c_str();
        if(sdfileName == "bin") {
        unsigned long data = uploadFile.size();
        Serial2.write((uint8_t*)&data, sizeof(uploadFile.size()));
        DEBUG_UPDATER_NL("data : ",data);
        }
        if (sdfileName == "hex") {
        Serial2.write(getversion, sizeof(getversion));
        delay(50);                                                                                                                                                    //this dellay use for recieving bootloader commands from slave
        Serial2.write(erashFlash, sizeof(erashFlash));
        delay(50);
        }
        fileNameChecker = false;
        parsedData = "";
      }
      if (sdfileName == "hex") {
        size_t size = uploadFile.available();
        if (size) {

          String readLine = uploadFile.readStringUntil('\n');
          static unsigned int parseLength = 0;
          unsigned char dataLength = 0;
          if (checkChecksumForHex(readLine.c_str())) { 
            long int temAddress , pastAddress = 0 , length ;
            String DataType = readLine.substring(1, 3);
            String Address = readLine.substring(3, 7);
            String RecordType = readLine.substring(7, 9);
            String tempData = readLine.substring(9);
            String data = tempData.substring(0, (tempData.length() - 3));
            String checkSum = readLine.substring((readLine.length() - 3), readLine.length());
            long tempLength = length;
            if (Address.length() == 4) {
              char *pEnd;
              pastAddress = temAddress;
              long int tem1Address = strtol(Address.c_str(), &pEnd, 16);
              if (tem1Address >= 12288UL && tem1Address <= 14216UL) {
                temAddress = strtol(Address.c_str(), &pEnd, 16);
                if (temAddress == 12288UL) {
                  tempLength = 0;
                  pastAddress = 12288UL;
                }
                char *pEnd1;
                length = strtol(DataType.c_str(), &pEnd1, 16);
                if (length == 4 || length == 8 || length == 12 || length == 16 ) {
                  if ((temAddress - pastAddress) > tempLength) {
                    int var = (temAddress - pastAddress);
                    int var2 = (var - tempLength);
                    String tempData = "";
                    for (size_t i = 1; i <= var2; i++) {
                      if (i == var2) {
                        tempData += "00";
                      } else {
                        tempData += "FF";
                      }
                    }
                    data = tempData + data;
                  }
                  parsedData = parsedData + data;
                }
              }
            }
            if (RecordType == "01") {
              String firstAddress = "3000";
              static int ll = (hexToInt((char *)firstAddress.c_str()) / 2);
              int index = ((parsedData.length() / 480UL) + 1);
              String voidMain = "" , voidAddress = "";
              int mainLowAddress = 0 ,mainHighAddress = 0, mainUpperAddress = 0 , mainExtendedAddress = 0 ;
              for (size_t i = 0; i < index; i++) {
                uint8_t length = 240UL;
                lowAddess = 0 ,highAddress = 0 , upperAddress = 0,extendedAddress = 0 ;
                String ff = parsedData.substring(parseLength, (parseLength + 480));
                String newAddress = decToHex(ll);
                lowAddess = hexToInt((char *)newAddress.substring(2, 4).c_str());
                highAddress = hexToInt((char *)newAddress.substring(0, 2).c_str());
                upperAddress = 0;
                extendedAddress = 0;
                if(i == 0) {
                  voidMain = ff.c_str();
                  voidAddress = newAddress;
                  mainLowAddress = lowAddess;
                  mainHighAddress = highAddress;
                  mainUpperAddress = upperAddress;
                  mainExtendedAddress = extendedAddress;
                  ll += 120UL;
                  parseLength += ff.length();
                  continue;
                }
                delay(100);
                if(i == (index -1)) {
                  uint8_t originalLength = length;
                  length = (ff.length()/2);
                  if (length < 16) {
                    int tempVar = length;
                    length = 16UL;
                    for (size_t i = 1; i <= (length - tempVar); i++) {
                      if (i == (length - tempVar))
                      {
                        ff = ff + "00";
                      } else {
                        ff = ff + "FF";
                      }
                    }
                  }
                  unsigned char writeFlash[length+11] = {0x02, length , 0x00, 0x55, 0x00, 0xAA, 0x00, lowAddess, highAddress, upperAddress, extendedAddress};
                  unsigned char tempararyBuff[500] = {};
                  int pos = 11U;
                  for (size_t startLength = 0; i < (ff.length()); startLength += 2) {
                    sscanf((char *)(ff.c_str()) + startLength, "%02hhX", &tempararyBuff[startLength]);
                    writeFlash[pos] = tempararyBuff[startLength];
                    pos++;
                  }
                  Serial2.write(writeFlash,sizeof(writeFlash));
                  delay(500);
                  unsigned char writeMainFlash[originalLength+11] = {0x02, originalLength , 0x00, 0x55, 0x00, 0xAA, 0x00, mainLowAddress, mainHighAddress, mainUpperAddress, mainExtendedAddress};
                  unsigned char tempararyMainBuff[500] = {};
                  int mainPos = 11U;
                  for (size_t i = 0; i < voidMain.length(); i += 2) {
                    sscanf((char *)(voidMain.c_str()) + i, "%02hhX", &tempararyMainBuff[i]);
                    writeMainFlash[mainPos] = tempararyMainBuff[i];
                    mainPos++;
                  }
                  Serial2.write(writeMainFlash,sizeof(writeMainFlash));
                  delay(250);
                  Serial2.write(resetData, sizeof(resetData));
                  DEBUG_UPDATER_NL("updated Succesfully...\r\n");
                  uploadFile.close();
                  parsedData = "";
                  LedShareFileOpen = false;
                } else {
                unsigned char writeFlash[length + 11] = {0x02, length , 0x00, 0x55, 0x00, 0xAA, 0x00, lowAddess, highAddress, upperAddress, extendedAddress};
                unsigned char tempararyBuff[500] = {};
                int pos = 11U;
                for (size_t i = 0; i < (ff.length()); i += 2) {
                    sscanf((char *)(ff.c_str()) + i, "%02hhX", &tempararyBuff[i]);
                    writeFlash[pos] = tempararyBuff[i];
                    pos++;
                }
                ll += 120UL;
                parseLength += ff.length();
                Serial2.write(writeFlash,sizeof(writeFlash));
                delay(500);
                }
              }
            }
          }
          readData = (readData + readLine.length() + 1);
          readedPercentage = (readData * 100) / uploadFile.size();
          currentPer = readedPercentage;
          lv_bar_set_value(bar, currentPer, LV_ANIM_ON);
          if ((readData == uploadFile.size()) ) {
            currentPer = 0;
            readData = 0;
            TFTGUI.lvFileBrowser();
            uploadFile.close();
            LedShareFileOpen = false;
          }
        }
      } else if (sdfileName == "s19" || sdfileName == "s28" || sdfileName == "s37") {
        size_t size = uploadFile.available();
        if (size) {
          String readLine = uploadFile.readStringUntil('\n');
          if (checkChecksumForS19(readLine)) {
            String recordType = readLine.substring(0, 2);
            String byteCount = readLine.substring(2, 4);
            String address = readLine.substring(4, 8);
            String tempData = readLine.substring(8);
            String data = tempData.substring(0, (tempData.length() - 3));
            String checkSum = readLine.substring((readLine.length() - 3), readLine.length());
            parsedData = (parsedData + data + '\n');
            if (data.length() < 32UL) {
              int spaceLine = (32UL - data.length());
              DEBUG_UPDATER("  %s\t%s\t%s\t%s %*\t%s\n", recordType.c_str(), byteCount.c_str(), address.c_str(), data.c_str(), spaceLine, checkSum.c_str());
            } else {
              DEBUG_UPDATER("  %s\t%s\t%s\t%s\t%s\n", recordType.c_str(), byteCount.c_str(), address.c_str(), data.c_str(), checkSum.c_str());
            }
          }
          readData = (readData + readLine.length() + 1);
          readedPercentage = (readData * 100) / uploadFile.size();
          currentPer = readedPercentage;
          lv_bar_set_value(bar, currentPer, LV_ANIM_ON);
          if ((readData == uploadFile.size())) {
            currentPer = 0;
            readData = 0;
            uploadFile.close();
            TFTGUI.lvFileBrowser();
            LedShareFileOpen = false;
            DEBUG_UPDATER(parsedData.c_str());
          }
        }
      } else if (sdfileName == "bin") {
        size_t size1 = uploadFile.available();
        uint8_t tempraryBuff[128];
        static int inc = 0;
          if (size1) {
          int avalLength = uploadFile.readBytes((char *)tempraryBuff, ((size1 > sizeof(tempraryBuff)) ? sizeof(tempraryBuff) : size1));
          int writeData = Serial2.write(tempraryBuff, avalLength);
          delay(40);
          wb += avalLength;
          DEBUG_UPDATER_NL("SIZE : %ld ",wb);
          readedPercentage  = map(wb, 0, uploadFile.size(), 0, 100);
          currentPer = readedPercentage ;
          lv_bar_set_value(bar, currentPer, LV_ANIM_ON);
          if (wb == uploadFile.size()) {
            wb = 0;
            mySd.per = 0;
            currentPer = 0;
            // lv_bar_set_value(bar, 0, LV_ANIM_ON);
            TFTGUI.lvFileBrowser();
            if (Serial2.getWriteError()) {
                DEBUG_UPDATER("write error");
            }
            uploadFile.close();
            LedShareFileOpen = false;
          }
        }
      } else {
        DEBUG_MAIN_NL("this is not Hex or s19 or s28 or s37 file..");
        DEBUG_MAIN_NL("please Select Hex or s19 or s28 or s37 file ..");
        LedShareFileOpen = false;
      }
  }
}

void serverSetup() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {                                                                        // HTML WebPage from SPIFFS
    if (!request->authenticate(DEFAULT_USER, DEFAULT_PASS)) {
      return request->requestAuthentication();
    }
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.on("/test.js", HTTP_GET, [](AsyncWebServerRequest *request) {                                                                  // Javascript from SPIFFS
    request->send(SPIFFS, "/test.js", "application/javascript");
  });

  server.on("/fupload", HTTP_ANY, [](AsyncWebServerRequest *request) {                                                                  // upload a file to /fupload
    String webpage = "";
    for (int index = 0; index < request->args(); index++) {
      if (request->argName(index) == "fupload") {
        DEBUG_SERVER_NL("Uploading file: %s\n", (char *)request->arg(index).c_str());
        if (downloadFile((const char *)request->arg(index).c_str())) {
          request->send(SPIFFS, "/index.html", String(), false);
        }
      }
    }
  });

  server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request) {
    printDirectory(request, mainDirectory);
  });

  server.on("/previousDir", HTTP_GET, [](AsyncWebServerRequest *request) {
    DEBUG_SERVER_NL("previousDir is %s ", previousDir.c_str());
    printDirectory(request, previousDir);
    int pos = previousDir.lastIndexOf("/");
    DEBUG_SERVER_NL("pos of / is %d  ", pos);
    DEBUG_SERVER_NL("temDir is %s", previousDir.substring(0, pos));
    if ((pos == 1) || (pos == 0)) {
      previousDir = "/";
      mainDirectory = "/";
    } else {
      previousDir = previousDir.substring(0, pos);
      mainDirectory = previousDir.substring(0, pos);
    }
  });

  server.on("/home", HTTP_GET, [](AsyncWebServerRequest *request) {
    mainDirectory = "/";
    previousDir = "/";
    printDirectory(request, mainDirectory);
  });

  server.on("/openFile", HTTP_GET, [](AsyncWebServerRequest *request) {
    for (size_t i = 0; i < request->args(); i++) {
      String path = request->urlDecode(request->arg(i));
      DEBUG_SERVER_NL("path is %s ", path.c_str());
      openFile(request, path);
    }
  });

  server.on("/deleteConfirm", HTTP_GET, [](AsyncWebServerRequest *request) {
    deleteConfirm(request);
  });

  server.on("/doDelete", HTTP_GET, [](AsyncWebServerRequest *request) {
    doDelete(request);
  });

  server.on("/share", HTTP_GET, [](AsyncWebServerRequest *request) {
    doShare(request);
  });

  server.on("/readPer", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", (String)currentPer);
  });

  server.on("/photo", HTTP_GET, [](AsyncWebServerRequest *request) {
    DEBUG_SERVER_NL("path of file is %s ", photoPath.c_str());
    String contentType = getContentType(request, photoPath);
    String pathWithGz = photoPath + ".gz";
    if (SD.exists(pathWithGz) || SD.exists(photoPath)) {
      if (SD.exists(pathWithGz)) {
        photoPath += ".gz";
      }
      DEBUG_SERVER_NL("main directory is %s ", mainDirectory.c_str());
      DEBUG_SERVER_NL("previus directory is %s ", previousDir.c_str());
      AsyncWebServerResponse *response = request->beginResponse(SD.open(photoPath, "r"), "/" + request->arg(photoPath), contentType);
      request->send(response);
    }
  });
  server.begin();
}

bool downloadFile(const char *const fileURL) {
  bool result = false;
  char c = '/';
  fileName = strrchr(fileURL, c);
  mySd.deleteFile(SD, fileName);
  DEBUG_SERVER_NL("[HTTP] begin...");
  Http.setReuse(false);
  Http.begin(fileURL);                                                                                                                              // configure server and url
  DEBUG_SERVER_NL("[HTTP] GET...\n");
  int httpCode = Http.GET();                                                                                                                        // start connection and send HTTP header
  if ((httpCode > 0)) {
    DEBUG_SERVER_NL("[HTTP] GET... code: %d\n", httpCode);                                                                                          // HTTP header has been send and Server response header has been handled
    if (httpCode == HTTP_CODE_OK) {                                                                                                                 // file found at server
      long len = Http.getSize();
      mySd.totalLen = len;
      DEBUG_SERVER_NL("fileName is %s", fileName);
      uploadFile = SD.open(fileName, FILE_APPEND);                                                                                                   // open file in append mode.
      DEBUG_SERVER_NL("Payload size [%lu] bytes.", mySd.totalLen);
      stream = Http.getStreamPtr();                                                                                                                  // get tcp stream
      wb = 0;
      downloadPending = true;
      result = true;
    }
  } else {
    DEBUG_SERVER_NL("[HTTP] GET... failed, error: %s\n", Http.errorToString(httpCode).c_str());
    result = false;
  }
  return result;
}

static void LvTickHandler(void) {
  lv_tick_inc(LVGL_TICK_PERIOD);
}

void sdCardHandler(void *pParameters) {
  DEBUG_MAIN_NL("sdCard GUI running on core....%u", xPortGetCoreID());
  for (;;) {
    lv_task_handler();                                                                                                                                  // let the GUI do its work
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;                                                                                                         // feed Wd for reducing a triggered Wd event and protect
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void downloadhandler(void *pParameters) {
  DEBUG_MAIN_NL("downloader running on core....%u", xPortGetCoreID());
  for (;;) {
    if (downloadPending) {
      if ((Http.connected())) {
        size_t size = stream->available(); // get available data size
        if ((size)) {
          int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size)); // read up to 1024 byte
          DEBUG_SERVER_NL("%d bytes read[c].", c);
          DEBUG_SERVER_NL("%d bytes available for read ", size); // write it to Serial
          uploadFile.write(buff, c);
          wb += c;
          mySd.per = map(wb, 0, mySd.totalLen, 0, 100);
          currentPer = mySd.per;
          DEBUG_SERVER_NL("%d %%.... Downloaded", currentPer);
        }
        if ((wb == mySd.totalLen)) {
          currentPer = 0;
          DEBUG_SERVER_NL("[HTTP] connection closed or file downloaded.\n");
          Http.end();
          uploadFile.close(); // close file.
          downloadPending = false;
          mySd.fileList("/");
        }
      }
    }

    if (currentFileOpen) {
      static String sdfileName;
      if (fileNameChecker) { 
        String tempName = uploadFile.name();
        sdfileName = tempName.substring((tempName.length() - 3), tempName.length()).c_str();
        if(sdfileName == "bin") {
        unsigned long data = uploadFile.size();
        Serial2.write((uint8_t*)&data, sizeof(uploadFile.size()));
        DEBUG_UPDATER_NL("data : ",data);
        }
        if (sdfileName == "hex") {
        Serial2.write(getversion, sizeof(getversion));
        delay(50);                                                                                                                                                    //this dellay use for recieving bootloader commands from slave
        Serial2.write(erashFlash, sizeof(erashFlash));
        delay(50);
        }
        fileNameChecker = false;
        parsedData = "";
      }
      if (sdfileName == "hex") {
        size_t size = uploadFile.available();
        if (size) {

          String readLine = uploadFile.readStringUntil('\n');
          static unsigned int parseLength = 0;
          unsigned char dataLength = 0;
          if (checkChecksumForHex(readLine.c_str())) { 
            long int temAddress , pastAddress = 0 , length ;
            String DataType = readLine.substring(1, 3);
            String Address = readLine.substring(3, 7);
            String RecordType = readLine.substring(7, 9);
            String tempData = readLine.substring(9);
            String data = tempData.substring(0, (tempData.length() - 3));
            String checkSum = readLine.substring((readLine.length() - 3), readLine.length());
            long tempLength = length;
            if (Address.length() == 4) {
              char *pEnd;
              pastAddress = temAddress;
              long int tem1Address = strtol(Address.c_str(), &pEnd, 16);
              if (tem1Address >= 12288UL && tem1Address <= 14216UL) {
                temAddress = strtol(Address.c_str(), &pEnd, 16);
                if (temAddress == 12288UL) {
                  tempLength = 0;
                  pastAddress = 12288UL;
                }
                char *pEnd1;
                length = strtol(DataType.c_str(), &pEnd1, 16);
                if (length == 4 || length == 8 || length == 12 || length == 16 ) {
                  if ((temAddress - pastAddress) > tempLength) {
                    int var = (temAddress - pastAddress);
                    int var2 = (var - tempLength);
                    String tempData = "";
                    for (size_t i = 1; i <= var2; i++) {
                      if (i == var2) {
                        tempData += "00";
                      } else {
                        tempData += "FF";
                      }
                    }
                    data = tempData + data;
                  }
                  parsedData = parsedData + data;
                }
              }
            }
            if (RecordType == "01") {
              String firstAddress = "3000";
              static int ll = (hexToInt((char *)firstAddress.c_str()) / 2);
              int index = ((parsedData.length() / 480UL) + 1);
              String voidMain = "" , voidAddress = "";
              int mainLowAddress = 0 ,mainHighAddress = 0, mainUpperAddress = 0 , mainExtendedAddress = 0 ;
              for (size_t i = 0; i < index; i++) {
                uint8_t length = 240UL;
                lowAddess = 0 ,highAddress = 0 , upperAddress = 0,extendedAddress = 0 ;
                String ff = parsedData.substring(parseLength, (parseLength + 480));
                String newAddress = decToHex(ll);
                lowAddess = hexToInt((char *)newAddress.substring(2, 4).c_str());
                highAddress = hexToInt((char *)newAddress.substring(0, 2).c_str());
                upperAddress = 0;
                extendedAddress = 0;
                if(i == 0) {
                  voidMain = ff.c_str();
                  voidAddress = newAddress;
                  mainLowAddress = lowAddess;
                  mainHighAddress = highAddress;
                  mainUpperAddress = upperAddress;
                  mainExtendedAddress = extendedAddress;
                  ll += 120UL;
                  parseLength += ff.length();
                  continue;
                }
                delay(100);
                if(i == (index -1)) {
                  uint8_t originalLength = length;
                  length = (ff.length()/2);
                  if (length < 16) {
                    int tempVar = length;
                    length = 16UL;
                    for (size_t i = 1; i <= (length - tempVar); i++) {
                      if (i == (length - tempVar))
                      {
                        ff = ff + "00";
                      } else {
                        ff = ff + "FF";
                      }
                    }
                  }
                  unsigned char writeFlash[length+11] = {0x02, length , 0x00, 0x55, 0x00, 0xAA, 0x00, lowAddess, highAddress, upperAddress, extendedAddress};
                  unsigned char tempararyBuff[500] = {};
                  int pos = 11U;
                  for (size_t startLength = 0; i < (ff.length()); startLength += 2) {
                    sscanf((char *)(ff.c_str()) + startLength, "%02hhX", &tempararyBuff[startLength]);
                    writeFlash[pos] = tempararyBuff[startLength];
                    pos++;
                  }
                  Serial2.write(writeFlash,sizeof(writeFlash));
                  delay(500);
                  unsigned char writeMainFlash[originalLength+11] = {0x02, originalLength , 0x00, 0x55, 0x00, 0xAA, 0x00, mainLowAddress, mainHighAddress, mainUpperAddress, mainExtendedAddress};
                  unsigned char tempararyMainBuff[500] = {};
                  int mainPos = 11U;
                  for (size_t i = 0; i < voidMain.length(); i += 2) {
                    sscanf((char *)(voidMain.c_str()) + i, "%02hhX", &tempararyMainBuff[i]);
                    writeMainFlash[mainPos] = tempararyMainBuff[i];
                    mainPos++;
                  }
                  Serial2.write(writeMainFlash,sizeof(writeMainFlash));
                  delay(250);
                  Serial2.write(resetData, sizeof(resetData));
                  DEBUG_UPDATER_NL("updated Succesfully...\r\n");
                  uploadFile.close();
                  parsedData = "";
                  currentFileOpen = false;
                } else {
                unsigned char writeFlash[length + 11] = {0x02, length , 0x00, 0x55, 0x00, 0xAA, 0x00, lowAddess, highAddress, upperAddress, extendedAddress};
                unsigned char tempararyBuff[500] = {};
                int pos = 11U;
                for (size_t i = 0; i < (ff.length()); i += 2) {
                    sscanf((char *)(ff.c_str()) + i, "%02hhX", &tempararyBuff[i]);
                    writeFlash[pos] = tempararyBuff[i];
                    pos++;
                }
                ll += 120UL;
                parseLength += ff.length();
                Serial2.write(writeFlash,sizeof(writeFlash));
                delay(500);
                }
              }
            }
          }
          readData = (readData + readLine.length() + 1);
          readedPercentage = (readData * 100) / uploadFile.size();
          currentPer = readedPercentage;
          if ((readData == uploadFile.size()) ) {
            currentPer = 0;
            readData = 0;
          }
        }
      } else if (sdfileName == "s19" || sdfileName == "s28" || sdfileName == "s37") {
        size_t size = uploadFile.available();
        if (size) {
          String readLine = uploadFile.readStringUntil('\n');
          if (checkChecksumForS19(readLine)) {
            String recordType = readLine.substring(0, 2);
            String byteCount = readLine.substring(2, 4);
            String address = readLine.substring(4, 8);
            String tempData = readLine.substring(8);
            String data = tempData.substring(0, (tempData.length() - 3));
            String checkSum = readLine.substring((readLine.length() - 3), readLine.length());
            parsedData = (parsedData + data + '\n');
            if (data.length() < 32UL) {
              int spaceLine = (32UL - data.length());
              DEBUG_UPDATER("  %s\t%s\t%s\t%s %*\t%s\n", recordType.c_str(), byteCount.c_str(), address.c_str(), data.c_str(), spaceLine, checkSum.c_str());
            } else {
              DEBUG_UPDATER("  %s\t%s\t%s\t%s\t%s\n", recordType.c_str(), byteCount.c_str(), address.c_str(), data.c_str(), checkSum.c_str());
            }
          }
          readData = (readData + readLine.length() + 1);
          readedPercentage = (readData * 100) / uploadFile.size();
          currentPer = readedPercentage;
          if ((readData == uploadFile.size())) {
            currentPer = 0;
            readData = 0;
            uploadFile.close();
            currentFileOpen = false;
            DEBUG_UPDATER(parsedData.c_str());
          }
        }
      } else if (sdfileName == "bin") {
        size_t size1 = uploadFile.available();
        uint8_t tempraryBuff[128];
        static int inc = 0;
          if (size1) {
          int avalLength = uploadFile.readBytes((char *)tempraryBuff, ((size1 > sizeof(tempraryBuff)) ? sizeof(tempraryBuff) : size1));
          int writeData = Serial2.write(tempraryBuff, avalLength);
          delay(40);
          wb += avalLength;
          DEBUG_UPDATER_NL("SIZE : %l ",wb);
          mySd.per = map(wb, 0, uploadFile.size(), 0, 100);
          currentPer = mySd.per;
          if (wb == uploadFile.size()) {
            wb = 0;
            mySd.per = 0;
            currentPer = 0;
            if (Serial2.getWriteError()) {
                DEBUG_UPDATER("write error");
            }
            uploadFile.close();
            delay(500);
            currentFileOpen = false;
          }
        }
      } else {
        DEBUG_MAIN_NL("this is not Hex or s19 or s28 or s37 file..");
        DEBUG_MAIN_NL("please Select Hex or s19 or s28 or s37 file ..");
        currentFileOpen = false;
      }
    }
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE; // feed Wd for reducing a triggered Wd event and protect
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void printDirectory(AsyncWebServerRequest *request, String path) {
  StaticJsonDocument<4096> doc;
  String finalResponse = "";
  if (path != "/" && !SD.exists((char *)path.c_str())) {
    DEBUG_SERVER_NL("not found");
  }
  File dir = SD.open((char *)path.c_str());
  path = String();
  if (!dir.isDirectory()) {
    dir.close();
  }
  dir.rewindDirectory();
  File entry;
  int index = 0;
  while (entry = dir.openNextFile()) {
    doc["index"] = String(index);
    index++;
    doc["name"] = String(entry.name());
    int bytes = entry.size();
    String fsize = "";
    if (bytes < 1024) {
      fsize = String(bytes) + " B";
    } else if ((bytes < (1024 * 1024))) {
      fsize = String(bytes / 1024.0, 3) + " KB";
    } else if ((bytes < (1024 * 1024 * 1024))) {
      fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
    } else {
      fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
    }
    doc["size"] = fsize;
    String deletBtn = "Delete", shareBtn = "share";
    doc["Deletefile"] = deletBtn;
    doc["Sharefile"] = shareBtn;
    String response;
    serializeJsonPretty(doc, response);
    finalResponse += response + ',' + '\n';
    entry.close();
  }
  dir.close();
  finalResponse.remove(finalResponse.length() - 2);
  finalResponse = "[" + finalResponse + "]";
  request->send(200, "text/html", finalResponse);
}

void openFile(AsyncWebServerRequest *request, String dlPath) {
  String webpage = "";
  DEBUG_SERVER_NL("dlpath = %s", dlPath.c_str());
  if (SD.exists(dlPath)) {
    File entry = SD.open(dlPath);
    if (!entry.isDirectory()) {
      if (dlPath.endsWith("/")) {
        dlPath += "index.htm";
      }
      String contentType = getContentType(request, dlPath);
      String pathWithGz = dlPath + ".gz";
      if (SD.exists(pathWithGz) || SD.exists(dlPath)) {
        if (SD.exists(pathWithGz)) {
          dlPath += ".gz";
        }
        DEBUG_SERVER_NL("main directory is %s ", mainDirectory.c_str());
        DEBUG_SERVER_NL("previus directory is %s ", previousDir.c_str());
        photoPath = dlPath;
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/photoview.html", String(), false);
        request->send(response);
      }
    } else {
      previousDir = mainDirectory;
      mainDirectory = dlPath;
      request->send(SPIFFS, "/index.html", String(), false);
    }
  } else {
    server._handleDisconnect(request);
  }
}

void doDelete(AsyncWebServerRequest *request) {
  String webpage = "";
  for (uint8_t i = 0; i < request->args(); i++) {
    if (request->argName(i) == "file") {
      if (SD.remove(request->arg(i))) {
        request->send(SPIFFS, "/index.html", String(), false);
      } else if (SD.rmdir((char *)request->arg(i).c_str())) {
        request->send(SPIFFS, "/index.html", String(), false);
      } else {
        webpage += F("<br>Delete failed<br>");
        request->send(200, "text/html", webpage);
      }
    }
  }
}

void deleteConfirm(AsyncWebServerRequest *request) {
  String webpage = "";
  for (size_t index = 0; index < request->args(); index++) {
    DEBUG_SERVER_NL("%s", request->argName(index));
    if (request->argName(index) == "file") {
      webpage += F("<hr>Do you want to delete the file:<br>");
      webpage += request->arg(index);
      webpage += F("<br><br><button class='buttons' onclick=\"location.href='/doDelete?file=");
      webpage += request->arg(index);
      webpage += F("';\">Yes</button>");
    }
  }
  webpage += F("<button class='buttons' onclick='window.history.back();'>No</button>");
  request->send(200, "text/html", webpage);
}

void doShare(AsyncWebServerRequest *request) { 
  for (size_t index = 0; index < request->args(); index++) { 
    DEBUG_SERVER_NL("%s", request->argName(index));
    if (request->argName(index) == "file") {
      uploadFile = SD.open(request->arg(index),FILE_READ);
      if (uploadFile) {
        if (uploadFile.isDirectory()) {
          request->send(SPIFFS, "/index.html", String(), false);
        } else if (uploadFile.available()) {
          currentFileOpen = true;
          fileNameChecker = true;
          request->send(SPIFFS, "/index.html", String(), false);
        }
      }
    }
  }
}

String getContentType(AsyncWebServerRequest *request, String filename) {
  filename.toUpperCase();
  if (request->hasArg("download"))
    return "application/octet-stream";
  else if (filename.endsWith(".HTM"))
    return "text/html";
  else if (filename.endsWith(".HTML"))
    return "text/html";
  else if (filename.endsWith(".CSS"))
    return "text/css";
  else if (filename.endsWith(".JS"))
    return "application/javascript";
  else if (filename.endsWith(".PNG"))
    return "image/png";
  else if (filename.endsWith(".GIF"))
    return "image/gif";
  else if (filename.endsWith(".JPG"))
    return "image/jpeg";
  else if (filename.endsWith(".ICO"))
    return "image/x-icon";
  else if (filename.endsWith(".XML"))
    return "text/xml";
  else if (filename.endsWith(".PDF"))
    return "application/x-pdf";
  else if (filename.endsWith(".ZIP"))
    return "application/x-zip";
  else if (filename.endsWith(".GZ"))
    return "application/x-gzip";
  else if (filename.endsWith(".mp4"))
    return "video/mp4";
  return "text/plain";
}


