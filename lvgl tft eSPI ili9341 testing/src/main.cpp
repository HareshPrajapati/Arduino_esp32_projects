#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <Ticker.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "GUI.h"
#include <ArduinoJson.h>
#include <SPIFFSEditor.h>
WebServer server(80);

#define TFT_BL (22)
#define LED (27)
#define SERVER_NAME "192.168.29.253" //  address is http://192.168.29.253/

SDcard mySd;
TFT_Gui TFTGUI;
Ticker tick; /* timer for interrupt handler */
TaskHandle_t task1, task2;
File uploadFile;

/* Constants */
const char *const WIFI_SSDI = "Mikrotroniks_Jio";
const char *const WIFI_PASS = "51251251";
const char *default_httpuser = "admin";
const char *default_httppassword = "admin";
const char *host = "192.168.29.253";
int currentPer = 1;
String arg;
String uploadPath, mainDirectory = "/", previousDir = "/";

static void LvTickHandler(void);
void sdCardHandler(void *pParameters);
void handleFileDownload();
bool downloadFile(const char *const fileURL);
void deleteConfirm();
void doDelete();
void handlePrintDir();
void printHomeDir();
void printDirectory(String path = "/");
void handleReadFile();
void readFile(String dlPath);
String getContentType(String filename);
void handleNotFound();
void handlePer();
void handlePreviousDir();

void setup() {
  Serial.begin(921600UL); /* prepare for possible serial debug */
  Serial.setDebugOutput(true);

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("Card failed or not present.."));
    mySd.SDPresent = false;
  } else {
    Serial.println(F("Card initialised... file access enabled..."));
    mySd.SDPresent = true;
  }
  pinMode(LED, OUTPUT);
  ledcAttachPin(TFT_BL, 1);  // assign TFT_BL pin to channel 1
  ledcSetup(1, 12000UL, 10); // 12 kHz PWM, 10-bit resolution
  analogReadResolution(10);
  ledcWrite(1, 768);         // brightness 0 - 255

  Serial.print("\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSDI, WIFI_PASS);

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

  guiInIt();
  /*Initialize the graphics library's tick*/
  tick.attach_ms(LVGL_TICK_PERIOD, LvTickHandler);

  if (TFTGUI.initSD()) {
    TFTGUI.lvMain();
  } else{
    TFTGUI.lvErrorPage();
  }

  if (MDNS.begin(host)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(host);
    Serial.println("/");
  }
  server.serveStatic("/", SPIFFS, "/index.html");
  server.serveStatic("/test.js", SPIFFS, "/test.js");
  server.on("/home", printHomeDir);
  server.on("/list", handlePrintDir);
  server.on("/previousDir", handlePreviousDir);
  server.on("/fupload", handleFileDownload);
  server.on("/deleteConfirm", deleteConfirm);
  server.on("/doDelete", doDelete);
  server.on("/readFile", handleReadFile);
  server.on("/showPer", handlePer);
  server.begin();
  Serial.println("\r\nHTTP server started");
  xTaskCreatePinnedToCore(sdCardHandler,"SD card GUI",10000,NULL,2,&task2,0); //pin task to core 0
}

void loop() {
  server.handleClient();
}

void sdCardHandler(void *pParameters) {
  Serial.print("sdCard GUI running on core....");
  Serial.println(xPortGetCoreID());
  for (;;) {
    lv_task_handler(); /* let the GUI do its work */
    mySd.feedTheDog();
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

static void LvTickHandler(void) {
  lv_tick_inc(LVGL_TICK_PERIOD);
}

/***************************** server reader funcions *****************************************/

String getContentType(String filename) {
  filename.toUpperCase();
  if (server.hasArg("download"))
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

void handleNotFound() {
  String webpage = "";
  webpage += F("<hr>File Not Found<br>");
  webpage += F("<br>URI:");
  webpage += server.uri();
  webpage += F("<br>Method: ");
  webpage += (server.method() == HTTP_GET) ? "GET" : "POST";
  webpage += F("<br>Arguments: ");
  webpage += server.args();
  webpage += F("<br>");
  for (uint8_t i = 0; i < server.args(); i++) {
    webpage += server.argName(i) + ": " + server.arg(i) + "<br>";
  }
  webpage += F("<br><button class='buttons' onclick=\"location.href='/';\">OK</button>");
  server.send(404, "text/html", webpage);
}

void doDelete() {
  String webpage = "";
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "file") {
      Serial.printf("Deleting file: %s\n", (char *)server.arg(i).c_str());
      webpage += F("<hr>Deleting file: <br>");
      String filename = strrchr(server.arg(i).c_str(), '/');
      webpage += server.arg(i);
      // const char *filename = strrchr((const char *)server.arg(i).c_str(),'/');
      if (SD.remove(server.arg(i))) {
        webpage += F("<br>File deleted<br>");
      }
      else if (SD.rmdir((char *)server.arg(i).c_str())) {
        webpage += F("<br>directory deleted<br>");
      } else{
        webpage += F("<br>Delete failed<br>");
      }
    }
  }
  webpage += F("<br><button class='buttons' onclick=\"location.href='/';\">OK</button>");
  server.send(200, "text/html", webpage);
}

void deleteConfirm() {
  String webpage = "";
  for (uint8_t i = 0; i < server.args(); i++) {
    Serial.println(server.argName(i));
    if (server.argName(i) == "file") {
      webpage += F("<hr>Do you want to delete the file:<br>");
      webpage += server.arg(i);
      webpage += F("<br><br><button class='buttons' onclick=\"location.href='/doDelete?file=");
      webpage += server.arg(i);
      webpage += F("';\">Yes</button>");
    }
  }

  webpage += F("<button class='buttons' onclick='window.history.back();'>No</button>");
  server.send(200, "text/html", webpage);
}

void handleFileDownload() {
  String webpage = "";
  for (uint8_t index = 0; index < server.args(); index++) {
    if (server.argName(index) == "fupload") {
      Serial.printf("Uploading file: %s\n", (char *)server.arg(index).c_str());
      webpage += server.arg(index);
      if (downloadFile((const char *)server.arg(index).c_str())) {
        webpage += F("<br>File downloaded<br>");
        Serial.printf("File downloaded \r\n");
        mySd.fileList("/");
      } else {
        webpage += F("<br>downloading failed<br>");
        Serial.printf(" downloading failed\r\n");
        mySd.fileList("/");
      }
    }
  }
  webpage += F("<br><button class='buttons' onclick=\"location.href='/';\">OK</button>");
  server.send(200, "text/html", webpage);
}

void handlePer() {
  StaticJsonDocument<1024> doc;
  doc["percentage"] = String(currentPer);
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response); //Send web page
}

void handlePreviousDir() {
  Serial.printf("previousDir is %s \r\n", previousDir.c_str());
  printDirectory(previousDir);
  int pos = previousDir.lastIndexOf("/");
  Serial.printf("pos of / is %d \r\n ", pos);
  String tempDirectory = previousDir.substring(0, pos);
  Serial.printf("temDir is %s \r\n", tempDirectory.c_str());
  if (pos == 1 || pos == 0)  {
    previousDir = "/";
    mainDirectory = tempDirectory;
  } else {
    previousDir = tempDirectory;
    mainDirectory = tempDirectory;
  }
}

void handlePrintDir() {
  printDirectory(mainDirectory);
}

void printHomeDir() {
  mainDirectory = "/";
  previousDir = "/";
  printDirectory("/");
}


void printDirectory(String path) {
  StaticJsonDocument<1024> doc;
  Serial.printf("arg is %s \r\n", path.c_str());
  String finalResponse = "";
  if (path != "/" && !SD.exists((char *)path.c_str()))
    Serial.printf("not found  /r/n");
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
  server.send(200, "text/html", finalResponse);
}

void readFile(String dlPath) {
  String webpage = "";
  Serial.printf("dlpath = %s \r\n", dlPath.c_str());
  if (SD.exists(dlPath)) {
    File entry = SD.open(dlPath);
    if (!entry.isDirectory()) {
      // Serial.println(dlPath);
      Serial.println("handleFileRead: " + dlPath);
      if (dlPath.endsWith("/")) {
        dlPath += "index.htm";
      }
      String contentType = getContentType(dlPath);
      String pathWithGz = dlPath + ".gz";
      if (SD.exists(pathWithGz) || SD.exists(dlPath)) {
        if (SD.exists(pathWithGz)) {
          dlPath += ".gz";
        }
        File file = SD.open(dlPath, "r");
        server.streamFile(file, contentType);
        file.close();
      }
  } else {
      previousDir = mainDirectory;
      mainDirectory = dlPath;
      Serial.printf("Directory is = %s \r\n", dlPath.c_str());
      webpage += F("<body><h1> Do You Want to open : ");
      webpage += dlPath;
      webpage += F("Folder ? </h1>");
      webpage += F("<br><button class='buttons' onclick=\"location.href='/';\">yes</button>");
    }
  } else {
    handleNotFound();
  }
  server.send(200, "text/html", webpage);
}

void handleReadFile() {
  for (size_t i = 0; i < server.args(); i++) {
    String path = server.urlDecode(server.arg(i));
    Serial.printf("path is %s \r\n", path.c_str());
    readFile(path);
  }
}

bool downloadFile(const char *const fileURL) {
  bool result = false;
  char *fileName = strrchr(fileURL, '/');
  mySd.deleteFile(SD, fileName);
  HTTPClient Http;
  Serial.print("[HTTP] begin...\n");
  // configure server and url
  Http.begin(fileURL);
  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = Http.GET();
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      uploadFile = SD.open(fileName, FILE_APPEND);
      // get lenght of document (is -1 when Server sends no Content-Length header)
      long len = Http.getSize();
      mySd.totalLen = len;
      Serial.printf("Payload size [%lu] bytes.\r\n", len);
      // create buffer for read
      uint8_t buff[2048] = {0};
      // get tcp stream
      WiFiClient *stream = Http.getStreamPtr();
      long wb = 0;
      // read all data from server
      unsigned long start = millis();
      while (Http.connected() && (len > 0 || len == -1)) {
        // get available data size
        size_t size = stream->available();
        if (size){ // read up to 2048 byte

          int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
          // write it to Serial
          Serial.printf("%d bytes read[c].\r\n", c);
          Serial.printf("%d bytes available for read \r\n", size);
          // open file in append mode.
          uploadFile.write(buff, c);
          wb += c;
          mySd.per = map(wb,0,mySd.totalLen,0,100);
          currentPer = mySd.per;
          Serial.printf("%d %%.... Downloaded \r\n",currentPer);
          //close file.
          if (len > 0) {
            len -= c;
          }
        }
      }
      unsigned long And = millis();
      float tmm = (float)(And-start);
      float takenTime = (tmm/1000);
      Serial.printf("taken time is %.2f second\r\n",takenTime);
      Serial.println();
      Serial.print("[HTTP] connection closed or file downloaded.\n");
      result = true;
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", Http.errorToString(httpCode).c_str());
  }
  Http.end();
  time_t t = uploadFile.getLastWrite();
  struct tm *tmstruct = localtime(&t);
  Serial.printf("LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
  uploadFile.close();
  return (result);
}