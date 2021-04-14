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
#include <Update.h>

/***************************************** DEFINE *******************************************************************/

#define TFT_BL                   (22)


/**************************************** CONSTANT *****************************************************************/

const char *const WIFI_SSDI      = "Mikrotroniks_Jio";
const char *const WIFI_PASS      = "51251251";
const char *const DEFAULT_USER   = "admin";
const char *const DEFAULT_PASS   = "51251251";
const char *const host           = "192.168.29.253";

/***************************************** GLOBLES *****************************************************************/

String mainDirectory = "/", previousDir = "/", photoPath = "";
bool downloadPending = false , currentFileOpen = false;
size_t readChar = 0, readedPercentage = 0, finalPercentage = 0,currentPer = 0;
long readData , wb;
char *fileName;
// create buffer for read
uint8_t charBuff[1024] , buff[1024] = {0} ;


/***************************************** GLOBLE OBJECTS *********************************************************/

File uploadFile , currentFile;     /* File  */
WiFiClient *stream;                /* Wifi Client for stream file */
AsyncWebServer server(80);         /* Async web server object */
HTTPClient Http;                   /* HTTPClient */
Ticker tick;                       /* timer for interrupt handler */
TaskHandle_t task1, task2;         /* task handlers*/
SDcard mySd;                       /* Sd card object */
TFT_Gui TFTGUI;                    /* LittleVgl GUI object */

/******************************************* FUNCTION DECLARATION *************************************************/

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
void performUpdate(Stream &updateSource, size_t updateSize);
void updateFromFS(fs::FS &fs);
/****************************************************************************************************************/

void setup() {
  Serial.begin(921600UL);
  Serial.setDebugOutput(true);
  if (!SPIFFS.begin()) {                                     //SPIFFS For html files of web browser
    Serial.printf("Error to open SPIFFS file !!!! \r\n");
    ESP.restart();
  }
  guiInIt();                                                  ////LIttleVgl GUI init
  if (!SD.begin(SS)) {                                        //SD card for read, write and save files
    Serial.println(F("Card failed or not present.."));
    TFTGUI.lvErrorPage();
    mySd.SDPresent = false;
  } else {
    Serial.println(F("Card initialised... file access enabled..."));
    TFTGUI.lvFileBrowser();
    mySd.SDPresent = true;
  }
  ledcAttachPin(TFT_BL, 1);                                // assign TFT_BL pin to channel 1
  ledcSetup(1, 12000UL, 10);                               // 12 kHz PWM, 10-bit resolution
  analogReadResolution(10);
  ledcWrite(1, 768U);                                       // brightness
  Serial.print("\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSDI, WIFI_PASS);                         //connect to wifi
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSDI);
  uint8_t wifiRetry = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetry++ < 20) {
    delay(500);
  }
  if (wifiRetry == 21) {
    Serial.print("Could not connect to");
    Serial.println(WIFI_SSDI);
    ESP.restart();
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  tick.attach_ms(LVGL_TICK_PERIOD, LvTickHandler);                    //Initialize the graphics library's tick
  if (MDNS.begin(host))                                              //MDNS begin
  {
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(host);
    Serial.println("/");
  }
  serverSetup();                                                     //server aetup for events requests
  xTaskCreatePinnedToCore(downloadhandler, "Downloader", 10000, NULL, 2, &task1, 1); //pin task to core 1
  xTaskCreatePinnedToCore(sdCardHandler, "TFT GUI", 10000, NULL, 1, &task2, 0);      //pin task to core 0
}


void loop()
{
  if (LedShareFileOpen)
  {
    currentFile = myFile;
    size_t size = currentFile.available();
    if (size)
    {
      readChar = currentFile.read(charBuff, ((size > sizeof(charBuff)) ? sizeof(charBuff) : size));
      Serial.write(charBuff, readChar);
      readData += readChar;
      readedPercentage = (readData*100)/currentFile.size();
      currentPer = readedPercentage;
      lv_bar_set_value(bar, currentPer, LV_ANIM_ON);
      if (readData == currentFile.size())
      {
        currentPer = 0;
        lv_bar_set_value(bar, currentPer, LV_ANIM_ON);
        readChar = 0;
        readData = 0;
        currentFile.close();
        LedShareFileOpen = false;
      }
    }
  }
}

void serverSetup()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_USER, DEFAULT_PASS))
      return request->requestAuthentication();
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.on("/test.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/test.js", "application/javascript");
  });

  // upload a file to /fupload
  server.on("/fupload", HTTP_ANY, [](AsyncWebServerRequest *request) {
    String webpage = "";
    for (int index = 0; index < request->args(); index++)
    {
      if (request->argName(index) == "fupload")
      {
        Serial.printf("Uploading file: %s\n", (char *)request->arg(index).c_str());
        if (downloadFile((const char *)request->arg(index).c_str()))
        {
          request->send(SPIFFS, "/index.html", String(), false);
        }
      }
    }
  });

  server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request) {
    printDirectory(request, mainDirectory);
  });

  server.on("/previousDir", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("previousDir is %s \r\n", previousDir.c_str());
    printDirectory(request, previousDir);
    int pos = previousDir.lastIndexOf("/");
    Serial.printf("pos of / is %d \r\n ", pos);
    Serial.printf("temDir is");
    Serial.println(previousDir.substring(0, pos));
    if (pos == 1 || pos == 0)
    {
      previousDir = "/";
      mainDirectory = "/";
    }
    else
    {
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
    for (size_t i = 0; i < request->args(); i++)
    {
      String path = request->urlDecode(request->arg(i));
      Serial.printf("path is %s \r\n", path.c_str());
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
    Serial.printf("photopath is %s \r\n", photoPath.c_str());
    String contentType = getContentType(request, photoPath);
    String pathWithGz = photoPath + ".gz";
    if (SD.exists(pathWithGz) || SD.exists(photoPath))
    {
      if (SD.exists(pathWithGz))
      {
        photoPath += ".gz";
      }
      Serial.printf("main dir is %s \r\n", mainDirectory.c_str());
      Serial.printf("previus dir is %s \r\n", previousDir.c_str());
      AsyncWebServerResponse *response = request->beginResponse(SD.open(photoPath, "r"), "/" + request->arg(photoPath), contentType);
      request->send(response);
    }
  });
  server.begin();
}

bool downloadFile(const char *const fileURL)
{
  char c = '/';
  fileName = strrchr(fileURL, c);
  mySd.deleteFile(SD, fileName);
  Serial.print("[HTTP] begin...\n");
  // configure server and url
  Http.setReuse(false);
  Http.begin(fileURL);
  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = Http.GET();
  if (httpCode > 0)
  {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("request->arg(index).c_str() == %s \r\n", fileURL);
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    // file found at server
    if (httpCode == HTTP_CODE_OK)
    {
      long len = Http.getSize();
      mySd.totalLen = len;
      Serial.printf("fileName is %s \r\n", fileName);
      // open file in append mode.
      uploadFile = SD.open(fileName, FILE_APPEND);
      // get lenght of document (is -1 when Server sends no Content-Length header)
      Serial.printf("Payload size [%lu] bytes.\r\n", mySd.totalLen);
      // get tcp stream
      stream = Http.getStreamPtr();
      wb = 0;
      downloadPending = true;
    }
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %s\n", Http.errorToString(httpCode).c_str());
  }
}

static void LvTickHandler(void)
{
  lv_tick_inc(LVGL_TICK_PERIOD);
}
void sdCardHandler(void *pParameters)
{
  Serial.print("sdCard GUI running on core....");
  Serial.println(xPortGetCoreID());
  for (;;)
  {
    lv_task_handler(); /* let the GUI do its work */
    mySd.feedTheDog();
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void downloadhandler(void *pParameters)
{
  Serial.print("downloader running on core....");
  Serial.println(xPortGetCoreID());
  for (;;)
  {
    if (downloadPending)
    {
      if (Http.connected())
      {
        // get available data size
        size_t size = stream->available();
        if (size)
        { // read up to 1024 byte
          int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
          // write it to Serial
          Serial.printf("%d bytes read[c].\r\n", c);
          Serial.printf("%d bytes available for read \r\n", size);
          uploadFile.write(buff, c);
          wb += c;
          mySd.per = map(wb, 0, mySd.totalLen, 0, 100);
          currentPer = mySd.per;
          Serial.printf("%d %%.... Downloaded \r\n", currentPer);
          //close file.
        }
        if (wb == mySd.totalLen)
        {
          currentPer = 0;
          Serial.println();
          Serial.print("[HTTP] connection closed or file downloaded.\n");
          Http.end();
          uploadFile.close();
          downloadPending = false;
          mySd.fileList("/");
        }
      }
    }

    if (currentFileOpen)
    {
      size_t size = currentFile.available();
      if (size)
      {
        readChar = currentFile.read(charBuff, ((size > sizeof(charBuff)) ? sizeof(charBuff) : size));
        Serial.write(charBuff, readChar);
        readData += readChar;
        readedPercentage = (readData*100)/currentFile.size();
        // readedPercentage = map(readData, 0, currentFile.size(), 0, 100);
        currentPer = readedPercentage;
      }
      if (readData == currentFile.size())
      {
        currentPer = 0;
        readChar = 0;
        readData = 0;
        currentFile.close();
        currentFileOpen = false;
      }
    }
    mySd.feedTheDog();
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void printDirectory(AsyncWebServerRequest *request, String path)
{
  StaticJsonDocument<4096> doc;
  Serial.printf("arg is %s \r\n", path.c_str());
  String finalResponse = "";
  if (path != "/" && !SD.exists((char *)path.c_str()))
    Serial.printf("not found  /r/n");
  File dir = SD.open((char *)path.c_str());
  path = String();
  if (!dir.isDirectory())
  {
    dir.close();
  }
  dir.rewindDirectory();
  File entry;
  int index = 0;
  while (entry = dir.openNextFile())
  {
    doc["index"] = String(index);
    index++;
    doc["name"] = String(entry.name());
    int bytes = entry.size();
    String fsize = "";
    if (bytes < 1024)
      fsize = String(bytes) + " B";
    else if (bytes < (1024 * 1024))
      fsize = String(bytes / 1024.0, 3) + " KB";
    else if (bytes < (1024 * 1024 * 1024))
      fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
    else
      fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
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

void openFile(AsyncWebServerRequest *request, String dlPath)
{
  String webpage = "";
  Serial.printf("dlpath = %s \r\n", dlPath.c_str());
  if (SD.exists(dlPath))
  {
    File entry = SD.open(dlPath);
    if (!entry.isDirectory())
    {
      if (dlPath.endsWith("/"))
      {
        dlPath += "index.htm";
      }
      String contentType = getContentType(request, dlPath);
      String pathWithGz = dlPath + ".gz";
      if (SD.exists(pathWithGz) || SD.exists(dlPath))
      {
        if (SD.exists(pathWithGz))
        {
          dlPath += ".gz";
        }
        Serial.printf("main dir is %s \r\n", mainDirectory.c_str());
        Serial.printf("previus dir is %s \r\n", previousDir.c_str());
        photoPath = dlPath;
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/photoview.html", String(), false);
        request->send(response);
      }
    }
    else
    {
      previousDir = mainDirectory;
      mainDirectory = dlPath;
      request->send(SPIFFS, "/index.html", String(), false);
    }
  }
  else
  {
    server._handleDisconnect(request);
  }
}

void doDelete(AsyncWebServerRequest *request)
{
  String webpage = "";
  for (uint8_t i = 0; i < request->args(); i++)
  {
    if (request->argName(i) == "file")
    {
      if (SD.remove(request->arg(i)))
      {
        request->send(SPIFFS, "/index.html", String(), false);
      }
      else if (SD.rmdir((char *)request->arg(i).c_str()))
      {
        request->send(SPIFFS, "/index.html", String(), false);
      }
      else
      {
        webpage += F("<br>Delete failed<br>");
        request->send(200, "text/html", webpage);
      }
    }
  }
}

void deleteConfirm(AsyncWebServerRequest *request)
{
  String webpage = "";
  for (uint8_t i = 0; i < request->args(); i++)
  {
    Serial.println(request->argName(i));
    if (request->argName(i) == "file")
    {
      webpage += F("<hr>Do you want to delete the file:<br>");
      webpage += request->arg(i);
      webpage += F("<br><br><button class='buttons' onclick=\"location.href='/doDelete?file=");
      webpage += request->arg(i);
      webpage += F("';\">Yes</button>");
    }
  }
  webpage += F("<button class='buttons' onclick='window.history.back();'>No</button>");
  request->send(200, "text/html", webpage);
}

void doShare(AsyncWebServerRequest *request)
{
  for (size_t index = 0; index < request->args(); index++)
  {
    Serial.println(request->argName(index));
    if (request->argName(index) == "file")
    {
      // Serial.println(request->arg(index));
      currentFile = SD.open(request->arg(index));
      if (currentFile)
      {
        if (currentFile.isDirectory())
        {
          request->send(SPIFFS, "/index.html", String(), false);
        }
        else if (currentFile.available())
          {
            currentFileOpen = true;
            request->send(SPIFFS, "/index.html", String(), false);
          }
      }
    }
  }
}

String getContentType(AsyncWebServerRequest *request, String filename)
{
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

// perform the actual update from a given stream
void performUpdate(Stream &updateSource, size_t updateSize)
{
  if (Update.begin(updateSize)) {
    size_t written = Update.writeStream(updateSource);
    if (written == updateSize) {
      Serial.println("Written : " + String(written) + " successfully");
    } else {
      Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
    }
    if (Update.end()) {
      Serial.println("OTA done!");
      if (Update.isFinished()) {
        Serial.println("Update successfully completed. Rebooting.");
      } else {
        Serial.println("Update not finished? Something went wrong!");
      }
    } else {
      Serial.println("Error Occurred. Error #: " + String(Update.getError()));
    }
  } else {
    Serial.println("Not enough space to begin OTA");
  }
}

// check given FS for valid firmware.bin and perform update if available
void updateFromFS(fs::FS &fs) {
  File updateBin = fs.open("/firmware.bin");
  if (updateBin) {
    if (updateBin.isDirectory()) {
      Serial.println("Error, update.bin is not a file");
      updateBin.close();
      return;
    }
    size_t updateSize = updateBin.size();
    if (updateSize > 0) {
      Serial.println("Try to start update");
      performUpdate(updateBin, updateSize);
    } else {
      Serial.println("Error, file is empty");
    }
    updateBin.close();
    // whe finished remove the binary from sd card to indicate end of the process
    fs.remove("/firmware.bin");
  } else {
    Serial.println("Could not load update.bin from sd root");
  }
}
