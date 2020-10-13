#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <Ticker.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "webpages.h"
#include "GUI.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>


WebServer server(80);
// AsyncWebServer server(80);

#define TFT_BL (22)
#define LED    (27)
#define SERVER_NAME "192.168.29.253" //  address is http://192.168.29.253/

SDcard mySd;
TFT_Gui TFTGUI;
Ticker tick; /* timer for interrupt handler */
TaskHandle_t task1, task2;
String arg;
int currentPer = 0;


/* Constants */
const char *const WIFI_SSDI = "Mikrotroniks_Jio";
const char *const WIFI_PASS = "51251251";
const String default_httpuser = "admin";
const String default_httppassword = "admin";
const char *host = "192.168.29.253";
String uploadPath;

static void LvTickHandler(void);
void sdCardHandler(void *pParameters);
unsigned char h2int(char c);
String urldecode(String str);
String file_size(int bytes);
void handleFileDownload();
bool downloadFile(const char * const fileURL);
void deleteConfirm();
void doDelete();
void handleNotFound();
void handleRoot();
bool handleFileRead(String path);
String getContentType(String filename);
void handlePer();
bool checkUserWebAuth();

void setup()
{

  Serial.begin(921600UL); /* prepare for possible serial debug */
  Serial.setDebugOutput(true);

  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  if (!SD.begin(SD_CS_PIN))
  {
    Serial.println(F("Card failed or not present.."));
    mySd.SDPresent = false;
  }
  else
  {
    Serial.println(F("Card initialised... file access enabled..."));
    mySd.SDPresent = true;
  }
  pinMode(LED, OUTPUT);
  ledcAttachPin(TFT_BL, 1);  // assign TFT_BL pin to channel 1
  ledcSetup(1, 12000UL, 10); // 12 kHz PWM, 10-bit resolution
  analogReadResolution(10);
  ledcWrite(1, 768); // brightness 0 - 255

  Serial.print("\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSDI, WIFI_PASS);

  Serial.print("Connecting to ");
  Serial.println(WIFI_SSDI);

  uint8_t wifiRetry = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetry++ < 20)
  {
    delay(500);
  }
  if (wifiRetry == 21)
  {
    Serial.print("Could not connect to");
    Serial.println(WIFI_SSDI);
    ESP.restart();
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  guiInIt();
  /*Initialize the graphics library's tick*/
  tick.attach_ms(LVGL_TICK_PERIOD, LvTickHandler);

  if (TFTGUI.initSD())
  {
    TFTGUI.lvMain();
  }
  else
  {
    TFTGUI.lvErrorPage();
  }

  if (MDNS.begin(host))
  {
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(host);
  }


  // server.serveStatic("/",SPIFFS,"/");
  server.on("/", []() {
    if (checkUserWebAuth())
    {
      handleRoot();
    }
    else
    {
      server.requestAuthentication();
    }
  });

  server.onNotFound(handleRoot);
  server.on("/fupload",handleFileDownload);
  server.on("/deleteConfirm", deleteConfirm);
  server.on("/doDelete", doDelete);
  server.on("/showPer",handlePer);
  server.begin();

  Serial.println("\r\nHTTP server started");

  xTaskCreatePinnedToCore(sdCardHandler,"SD card GUI",10000,NULL,2,&task2,0); //pin task to core 0
  // delay(1);
}

uint32_t last = 0;
void loop()
{
  server.handleClient();

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

static void LvTickHandler(void)
{
  lv_tick_inc(LVGL_TICK_PERIOD);
}

/***********************************************************************************************/

String getContentType(String filename)
{
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

bool handleFileRead(String path)
{
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/"))
    path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SD.exists(pathWithGz) || SD.exists(path))
  {
    if (SD.exists(pathWithGz))
      path += ".gz";
    File file = SD.open(path, "r");
    // size_t sent = server.streamFile(file, contentType);
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}


void handleRoot()
{ 

  String  directory = urldecode(server.uri());
  uploadPath = directory;
  File dir = SD.open(directory);
  String entryName = "";
  String tree = "";
  while (true)
  {
    File entry = dir.openNextFile();
    entryName = entry.name();
    entryName.replace(directory + "/", "");

    if (!entry)
    {
      // no more files
      break;
    }

    if (entry.isDirectory())
    {
      tree += F("<tr>");
      tree += F("<td data-value=\"");
      tree += entryName;
      tree += F("/\"><a class=\"icon dir\" href=\"");
      tree += entry.name();
      tree += F("\">");
      tree += entryName;
      tree += F("/</a></td>");
      tree += F("<td class=\"detailsColumn\" data-value=\"0\">-</td>");
      tree += F("<td class=\"detailsColumn\" data-value=\"0\">");
      tree += F("<button class='buttons' onclick=\"location.href='/deleteConfirm?folder=");
      tree += entry.name();
      tree += F("';\">Delete</button></td>");
      tree += F("<td class=\"detailsColumn\" data-value=\"0\">");
      tree += F("<button class='buttons' onclick=\"location.href='/deleteConfirm?folder=");
      tree += entry.name();
      tree += F("';\">Share</button></td>");
      tree += F("</tr>");
    }
    else
    {
      tree += F("<tr>");
      tree += F("<td data-value=\"");
      tree += entry.name();
      tree += F("\"><a class=\"icon file\" draggable=\"true\" href=\"");
      tree += entry.name();
      tree += F("\">");
      tree += entryName;
      tree += F("</a></td>");
      tree += F("<td class=\"detailsColumn\" data-value=\")");
      tree += file_size(entry.size());
      tree += F("\">");
      tree += file_size(entry.size());
      tree += F("</td>");
      tree += F("<td class=\"detailsColumn\" data-value=\"0\">");
      tree += F("<button class='buttons' onclick=\"location.href='/deleteConfirm?file=");
      tree += entry.name();
      tree += F("';\">Delete</button></td>");
      tree += F("<td class=\"detailsColumn\" data-value=\"0\">");
      tree += F("<button class='buttons' onclick=\"location.href='/deleteConfirm?file=");
      tree += entry.name();
      tree += F("';\">Share</button></td>");
      tree += F("</tr>");

    }
    entry.close();
  }

  int i, count;
  for (i = 0, count = 0; directory[i]; i++)
    count += (directory[i] == '/');
  count++;

  int parserCnt = 0;
  int rFromIndex = 0, rToIndex = -1;
  String lastElement = "";
  String tempElement = "";
  String path = directory;
  path.remove(0, 1);
  path += "/";
  while (count >= parserCnt)
  {
    rFromIndex = rToIndex + 1;
    rToIndex = path.indexOf('/', rFromIndex);
    if (count == parserCnt)
    {
      if (rToIndex == 0 || rToIndex == -1)
        break;
      tempElement = lastElement;
      lastElement = path.substring(rFromIndex, rToIndex);
    }
    else
      parserCnt++;
  }
  String webpage = "";
  webpage += header;
  webpage += F("<h1 id=\"header\">MikroTroniks Sd Card");
  webpage += directory;
  webpage += F("</h1>");

  if (directory != "/")
  {
    webpage += F("<div id=\"parentDirLinkBox\" style=\"display:block;\">");
    webpage += F("<a id=\"parentDirLink\" class=\"icon up\" href=\"");
    directory.replace(lastElement, "");
    if (directory.length() > 1)
      directory = directory.substring(0, directory.length() - 1);
    webpage += directory;
    webpage += F("\">");
    webpage += F("<span id=\"parentDirText\">BACK</span>");
    webpage += F("</a>");
    webpage += F("</div>");
  }
  webpage += script;
  webpage += F("<table>");
  webpage += F("<thead>");
  webpage += F("<tr class=\"header\" id=\"theader\">");
  webpage += F("<th onclick=\"sortTable(0);\">Name</th>");
  webpage += F("<th class=\"detailsColumn\" onclick=\"sortTable(1);\">Size</th>");
  webpage += F("<th></th>");
  webpage += F("</tr>");
  webpage += F("</thead>");
  webpage += F("<tbody id=\"tbody\">");
  webpage += tree;
  webpage += F("</tbody>");
  webpage += F("</table>");
  webpage += F("<hr>");
  webpage += body;
  webpage += footer;
  if (tree == "")
  {
    String dlPath = urldecode(server.uri());
    if (SD.exists(dlPath))
    {
      File entry = SD.open(dlPath);
      if (!entry.isDirectory())
      {
        Serial.println(dlPath);
        handleFileRead(dlPath);
      }
    }
    else
    {
      handleNotFound();
    }
  }
  server.send(200, "text/html", webpage);
}

void handleNotFound()
{
  String webpage = "";
  webpage += header;
  webpage += F("<hr>File Not Found<br>");
  webpage += F("<br>URI:");
  webpage += server.uri();
  webpage += F("<br>Method: ");
  webpage += (server.method() == HTTP_GET) ? "GET" : "POST";
  webpage += F("<br>Arguments: ");
  webpage += server.args();
  webpage += F("<br>");
  for (uint8_t i = 0; i < server.args(); i++)
  {
    webpage += server.argName(i) + ": " + server.arg(i) + "<br>";
  }
  webpage += F("<br><button class='buttons' onclick=\"location.href='/';\">OK</button>");
  server.send(404, "text/html", webpage);
}



void doDelete()
{

  String webpage = "";
  webpage += header;
  for (uint8_t i = 0; i < server.args(); i++)
  {
    if (server.argName(i) == "file")
    {
      Serial.printf("Deleting file: %s\n", (char *)server.arg(i).c_str());

      webpage += F("<hr>Deleting file: <br>");
      webpage += server.arg(i);
      if (SD.remove(server.arg(i)))
      {
        webpage += F("<br>File deleted<br>");
      }
      else
      {
        webpage += F("<br>Delete failed<br>");
      }
    }
    if (server.argName(i) == "folder")
    {
      Serial.printf("Removing Dir: %s\n", (char *)server.arg(i).c_str());
      webpage += F("<hr>Removing Dir: <br>");
      webpage += server.arg(i);
      if (SD.rmdir(server.arg(i)))
      {
        webpage += F("<br>Dir removed<br>");
      }
      else
      {
        webpage += F("<br>rmdir failed<br>");
      }
    }
  }
  webpage += F("<br><button class='buttons' onclick=\"location.href='/';\">OK</button>");
  server.send(200, "text/html", webpage);
}



void deleteConfirm()
{
  String webpage = "";
  webpage += header;
  for (uint8_t i = 0; i < server.args(); i++)
  {
    if (server.argName(i) == "file")
    {
      webpage += F("<hr>Do you want to delete the file:<br>");
      webpage += server.arg(i);
      webpage += F("<br><br><button class='buttons' onclick=\"location.href='/doDelete?file=");
      webpage += server.arg(i);
      webpage += F("';\">Yes</button>");
    }
    if (server.argName(i) == "folder")
    {
      webpage += F("<hr>Do you want to delete the Directory:<br>");
      webpage += server.arg(i);
      webpage += F("<br><br><button class='buttons' onclick=\"location.href='/doDelete?folder=");
      webpage += server.arg(i);
      webpage += F("';\">Yes</button>");
    }
  }

  webpage += F("<button class='buttons' onclick='window.history.back();'>No</button>");
  server.send(200, "text/html", webpage);
}



void handleFileDownload()
{

  String webpage = "";
  server.on("/", []() {
    if (checkUserWebAuth())
    {
      handleRoot();
    }
    else
    {
      server.requestAuthentication();
    }
  });
  for (uint8_t index = 0; index < server.args(); index++)
  {
    if (server.argName(index) == "fupload")
    {

      Serial.printf("Uploading file: %s\n", (char *)server.arg(index).c_str());
      webpage += server.arg(index);
      if (downloadFile((const char *)server.arg(index).c_str()))
      {
        webpage += F("<br>File downloaded<br>");
        Serial.printf("File downloaded \r\n");
        mySd.fileList("/");
      }else{ 
        webpage += F("<br> downloading failed<br>");
        Serial.printf(" downloading failed\r\n");
        mySd.fileList("/");
      }
    }
  }
  webpage += F("<br><button class='buttons' onclick=\"location.href='/';\">OK</button>");
  server.send(200, "text/html", webpage);
}



String file_size(int bytes)
{
  String fsize = "";
  if (bytes < 1024)
    fsize = String(bytes) + " B";
  else if (bytes < (1024 * 1024))
    fsize = String(bytes / 1024.0, 3) + " KB";
  else if (bytes < (1024 * 1024 * 1024))
    fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
  else
    fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
  return fsize;
}

String urldecode(String str)
{

  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++)
  {
    c = str.charAt(i);
    if (c == '+')
    {
      encodedString += ' ';
    }
    else if (c == '%')
    {
      i++;
      code0 = str.charAt(i);
      i++;
      code1 = str.charAt(i);
      c = (h2int(code0) << 4) | h2int(code1);
      encodedString += c;
    }
    else
    {
      encodedString += c;
    }

    yield();
  }

  return encodedString;
}
unsigned char h2int(char c)
{
  if (c >= '0' && c <= '9')
  {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f')
  {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F')
  {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}


void handlePer()
{
    server.send(200, "text/html", String(currentPer).c_str()); //Send web page
}

File uploadFile;

bool downloadFile(const char * const fileURL) {
    bool result = false;
    char *fileName = strrchr(fileURL, '/' );
    mySd.deleteFile(SD, fileName);
    HTTPClient Http;
    Serial.print("[HTTP] begin...\n");
    // configure server and url
    Http.begin(fileURL);
    // http.begin("media.gettyimages.com", 80, "/photos/red-heart-shape-balloon-flying-above-buildings-in-city-picture-id608954473");
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
            while (Http.connected() && (len > 0 || len == -1)){
                // get available data size
                size_t size = stream->available();
                if (size){   // read up to 2048 byte
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
                    // server.on("/showPer", handlePer);
                    // close file.
                    if (len > 0){
                        len -= c;
                    }
                }
                delayMicroseconds(1);
            }
            Serial.println();
            Serial.print("[HTTP] connection closed or file downloaded.\n");
            result = true;
        }
    }
    else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", Http.errorToString(httpCode).c_str());
    }
    Http.end();
    uploadFile.close();
    return (result);
}


bool checkUserWebAuth() {
  bool isAuthenticated = false;

  if (server.authenticate(default_httpuser.c_str(), default_httppassword.c_str())) {
    Serial.println("is authenticated via username and password");
    isAuthenticated = true;
  }
  return isAuthenticated;
}
