#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <Ticker.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "GUI.h"
#include <Update.h>
uint8_t fake_bar = 0;


WebServer server(80);

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
const char *const WIFI_SSDI = "Mikrotroniks";
const char *const WIFI_PASS = "51251251";
const char *host = "192.168.29.253";
const char *header = "<!DOCTYPE html>"
                     "<html>"
                     "<head>"
                     "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                     "<style>"
                     "  h1 {"
                     "font-family: Arial, Helvetica, sans-serif;"
                     "    border-bottom: 1px solid #c0c0c0;"
                     "    margin-bottom: 10px;"
                     "    padding-bottom: 10px;"
                     "    white-space: nowrap;"
                     "  }"
                     "body {"
                     "    background-color: #ccffff;"
                     "  }"
                     "  table {"
                     "font-family: Arial, Helvetica, sans-serif;"
                     "    border-collapse: collapse;"
                     "  }"
                     "  th {"
                     "font-family: Arial, Helvetica, sans-serif;"
                     "    cursor: pointer;"
                     "  }"
                     "  td.detailsColumn {"
                     "font-family: Arial, Helvetica, sans-serif;"
                     "    -webkit-padding-start: 2em;"
                     "    text-align: end;"
                     "    white-space: nowrap;"
                     "  }"
                     "  a.icon {"
                     "    -webkit-padding-start: 1.5em;"
                     "    text-decoration: none;"
                     "  }"
                     "  a.icon:hover {"
                     "    text-decoration: underline;"
                     "  }"
                     "  a.file {"
                     "    background : url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAIAAACQkWg2AAAABnRSTlMAAAAAAABupgeRAAABHUlEQVR42o2RMW7DIBiF3498iHRJD5JKHurL+CRVBp+i2T16tTynF2gO0KSb5ZrBBl4HHDBuK/WXACH4eO9/CAAAbdvijzLGNE1TVZXfZuHg6XCAQESAZXbOKaXO57eiKG6ft9PrKQIkCQqFoIiQFBGlFIB5nvM8t9aOX2Nd18oDzjnPgCDpn/BH4zh2XZdlWVmWiUK4IgCBoFMUz9eP6zRN75cLgEQhcmTQIbl72O0f9865qLAAsURAAgKBJKEtgLXWvyjLuFsThCSstb8rBCaAQhDYWgIZ7myM+TUBjDHrHlZcbMYYk34cN0YSLcgS+wL0fe9TXDMbY33fR2AYBvyQ8L0Gk8MwREBrTfKe4TpTzwhArXWi8HI84h/1DfwI5mhxJamFAAAAAElFTkSuQmCC \") left top no-repeat;"
                     "  }"
                     "  a.dir {"
                     "    background : url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAd5JREFUeNqMU79rFUEQ/vbuodFEEkzAImBpkUabFP4ldpaJhZXYm/RiZWsv/hkWFglBUyTIgyAIIfgIRjHv3r39MePM7N3LcbxAFvZ2b2bn22/mm3XMjF+HL3YW7q28YSIw8mBKoBihhhgCsoORot9d3/ywg3YowMXwNde/PzGnk2vn6PitrT+/PGeNaecg4+qNY3D43vy16A5wDDd4Aqg/ngmrjl/GoN0U5V1QquHQG3q+TPDVhVwyBffcmQGJmSVfyZk7R3SngI4JKfwDJ2+05zIg8gbiereTZRHhJ5KCMOwDFLjhoBTn2g0ghagfKeIYJDPFyibJVBtTREwq60SpYvh5++PpwatHsxSm9QRLSQpEVSd7/TYJUb49TX7gztpjjEffnoVw66+Ytovs14Yp7HaKmUXeX9rKUoMoLNW3srqI5fWn8JejrVkK0QcrkFLOgS39yoKUQe292WJ1guUHG8K2o8K00oO1BTvXoW4yasclUTgZYJY9aFNfAThX5CZRmczAV52oAPoupHhWRIUUAOoyUIlYVaAa/VbLbyiZUiyFbjQFNwiZQSGl4IDy9sO5Wrty0QLKhdZPxmgGcDo8ejn+c/6eiK9poz15Kw7Dr/vN/z6W7q++091/AQYA5mZ8GYJ9K0AAAAAASUVORK5CYII= \") left top no-repeat;"
                     "  }"
                     "  a.up {"
                     "    background : url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAmlJREFUeNpsU0toU0EUPfPysx/tTxuDH9SCWhUDooIbd7oRUUTMouqi2iIoCO6lceHWhegy4EJFinWjrlQUpVm0IIoFpVDEIthm0dpikpf3ZuZ6Z94nrXhhMjM3c8895977BBHB2PznK8WPtDgyWH5q77cPH8PpdXuhpQT4ifR9u5sfJb1bmw6VivahATDrxcRZ2njfoaMv+2j7mLDn93MPiNRMvGbL18L9IpF8h9/TN+EYkMffSiOXJ5+hkD+PdqcLpICWHOHc2CC+LEyA/K+cKQMnlQHJX8wqYG3MAJy88Wa4OLDvEqAEOpJd0LxHIMdHBziowSwVlF8D6QaicK01krw/JynwcKoEwZczewroTvZirlKJs5CqQ5CG8pb57FnJUA0LYCXMX5fibd+p8LWDDemcPZbzQyjvH+Ki1TlIciElA7ghwLKV4kRZstt2sANWRjYTAGzuP2hXZFpJ/GsxgGJ0ox1aoFWsDXyyxqCs26+ydmagFN/rRjymJ1898bzGzmQE0HCZpmk5A0RFIv8Pn0WYPsiu6t/Rsj6PauVTwffTSzGAGZhUG2F06hEc9ibS7OPMNp6ErYFlKavo7MkhmTqCxZ/jwzGA9Hx82H2BZSw1NTN9Gx8ycHkajU/7M+jInsDC7DiaEmo1bNl1AMr9ASFgqVu9MCTIzoGUimXVAnnaN0PdBBDCCYbEtMk6wkpQwIG0sn0PQIUF4GsTwLSIFKNqF6DVrQq+IWVrQDxAYQC/1SsYOI4pOxKZrfifiUSbDUisif7XlpGIPufXd/uvdvZm760M0no1FZcnrzUdjw7au3vu/BVgAFLXeuTxhTXVAAAAAElFTkSuQmCC \") left top no-repeat;"
                     "  }"
                     "  html[dir=rtl] a {"
                     "font-family: Arial, Helvetica, sans-serif;"
                     "    background-position-x: center;"
                     "  }"
                     "  #parentDirLinkBox {"
                     "font-family: Arial, Helvetica, sans-serif;"
                     "    margin-bottom: 10px;"
                     "    padding-bottom: 10px;"
                     "  }"
                     "  #listingParsingErrorBox {"
                     "font-family: Arial, Helvetica, sans-serif;"
                     "    border: 1px solid black;"
                     "    background: #fae691;"
                     "    padding: 10px;"
                     "    display: none;"
                     "  }"
                     ".content {"
                     "max-width: 500px;"
                     "margin: auto;"
                     "padding: 10px;"
                     "}"
                     "</style>"
                     "<title id=\"title\">Web FileBrowser</title>"
                     "</head>"
                     "<body>";

String uploadPath;

static void LvTickHandler(void);
void taskFileDownload(void *pvParameters);
void sdCardHandler(void *pParameters);

String split(String s, char parser, int index);
unsigned char h2int(char c);
String urlencode(String str);
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
void handleLED();
void handlePer();

void setup()
{

  Serial.begin(921600UL); /* prepare for possible serial debug */
  Serial.setDebugOutput(true);

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

  server.on("/", []() {
    handleRoot();
  });

  server.onNotFound(handleRoot);

  server.on("/fupload", handleFileDownload);
  server.on("/deleteConfirm", deleteConfirm);
  server.on("/doDelete", doDelete);
  server.on("/setLED", handleLED);
  server.on("/showPer",handlePer);
  server.begin();
  Serial.println("\r\nHTTP server started");

  // xTaskCreatePinnedToCore(sdCardHandler,"SD card GUI",10000,NULL,2,&task2,1); //pin task to core 1
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
    // lv_task_handler(); /* let the GUI do its work */
    // 
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
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}



void handleLED()
{
  String ledState = "OFF";
  String t_state = server.arg("LEDstate"); //Refer  xhttp.open("GET", "setLED?LEDstate="+led, true);
  Serial.println(t_state);
  if (t_state == "1")
  {
    digitalWrite(LED, HIGH); //LED ON
    ledState = "ON";         //Feedback parameter
  }
  else
  {
    digitalWrite(LED, LOW); //LED OFF
    ledState = "OFF";       //Feedback parameter
  }

  server.send(200, "text/html", ledState); //Send web page
}

void handleRoot()
{

  String directory = urldecode(server.uri());
  uploadPath = directory;
  File dir = SD.open(directory);
  String entryName = "";
  bool emptyFolder = true;
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

      emptyFolder = false;
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

      emptyFolder = false;
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
  webpage += F(header);
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

  webpage += ("<script>");
  webpage += ("function sortTable(l){");
  webpage += (" var e=document.getElementById(\"theader\"),n=e.cells[l].dataset.order||\"1\",s=0-(n=parseInt(n,10));");
  webpage += (" e.cells[l].dataset.order=s;");
  webpage += ("	var t,a=document.getElementById(\"tbody\"),r=a.rows,d=[];");
  webpage += ("	for(t=0;t<r.length;t++) d.push(r[t]);");
  webpage += ("	for(d.sort(function(e,t){var a=e.cells[l].dataset.value,r=t.cells[l].dataset.value;return l?(a=parseInt(a,10),(r=parseInt(r,10))<a?s:a<r?n:0):r<a?s:a<r?n:0}),t=0;t<d.length;t++)");
  webpage += ("	a.appendChild(d[t])");
  webpage += ("}");

  webpage += ("function sendData(led) {");
  webpage += ("var xhttp = new XMLHttpRequest();");
  webpage += ("xhttp.onreadystatechange = function() { ");
  webpage += ("if (this.readyState == 4 && this.status == 200) {");
  webpage += ("document.getElementById(\"LEDState\").innerHTML = this.responseText;} };");
  webpage += ("xhttp.open(\"GET\", \"setLED?LEDstate=\"+led, true);");
  webpage += ("xhttp.send();");
  webpage += ("	};");

  webpage += F("function _(el) {");
  webpage += F("return document.getElementById(el);");
  webpage += F("}");


  webpage += F("function progress() {");
  webpage += F("var x = document.getElementById(\"progressBar\");");
  webpage += F(" var xhttp = new XMLHttpRequest();");
  webpage += F("xhttp.onreadystatechange = function() {");
  webpage += F(" if (this.readyState == 4 && this.status == 200) {");
  webpage += F("   x.value = parseInt(this.responseText);");
  webpage += F("  console.log(\"progress\" + x.value);");
  webpage += F("  }");
  webpage += F(" };");
  webpage += F("xhttp.open(\"GET\", \"showPer\", true);");
  webpage += F("xhttp.send();");
  webpage += F("}");
  webpage += F("setInterval(progress, 100);");
  webpage += F("</script>");



  webpage += F("<table>");
  webpage += F("<thead>");
  webpage += F("<tr class=\"header\" id=\"theader\">");
  webpage += F("<th onclick=\"sortTable(0);\">Name</th>");
  webpage += F("<th></th>");
  webpage += F("</tr>");
  webpage += F("</thead>");
  webpage += F("<tbody id=\"tbody\">");
  webpage += tree;
  webpage += F("</tbody>");
  webpage += F("</table>");
  webpage += F("<hr>");


  webpage += F("<h1>Uploader</h1>");
  webpage += F("<form action='fupload' id='fupload'>");
  webpage += F("<input class='file-url' type='text' name='fupload' id = 'fupload'  size='60'  value=''>");
  webpage += F("<button class='buttons' style='margin-left:50px' type='submit'  onclick = \"progress()\"  >Upload </button></form><br>");
  webpage += F("<progress id=\"progressBar\" value=\"0\" max=\"100\" style=\"width:300px;\" ></progress>");
  // webpage += F("<p id=\"status\"></p>");
  // webpage += F("<p id=\"detailsheader\"></p>");
  // webpage += F("<p id=\"details\"></p>");
  /*************************************************************************/
  webpage += F("<div id=\"demo\">");
  webpage += F("<h1>The ESP32 Update web page without refresh</h1>");
  webpage += F("<button type=\"button\" onclick=\"sendData(1)\">LED ON</button>");
  webpage += F("<button type=\"button\" onclick=\"sendData(0)\">LED OFF</button><BR>");
  webpage += F("</div>");
  /****************************************************************************/

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
  webpage += F(header);
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
  webpage += F(header);
  for (uint8_t i = 0; i < server.args(); i++)
  {
    if (server.argName(i) == "file")
    {
      Serial.printf("Deleting file: %s\n", server.arg(i));

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
      Serial.printf("Removing Dir: %s\n", server.arg(i));
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
  webpage += F(header);
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
  fake_bar = 0;
  String webpage = "";
  server.on("/", []() {
    handleRoot();
  });
  webpage += F(header);
  for (uint8_t index = 0; index < server.args(); index++)
  {
    if (server.argName(index) == "fupload")
    {

      Serial.printf("Uploading file: %s\n", server.arg(index));
      webpage += server.arg(index);
      if (downloadFile((const char *)server.arg(index).c_str()))
      { 
        webpage += F("<br>File downloaded<br>");
        Serial.printf("File downloaded \r\n");
        mySd.fileList("/");
      }
      else
      {
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

String urlencode(String str)
{
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i = 0; i < str.length(); i++)
  {
    c = str.charAt(i);
    if (c == ' ')
    {
      encodedString += '+';
    }
    else if (isalnum(c))
    {
      encodedString += c;
    }
    else
    {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9)
      {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9)
      {
        code0 = c - 10 + 'A';
      }
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
      //encodedString+=code2;
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

String split(String s, char parser, int index)
{
  String rs = "";
  int parserIndex = index;
  int parserCnt = 0;
  int rFromIndex = 0, rToIndex = -1;
  while (index >= parserCnt)
  {
    rFromIndex = rToIndex + 1;
    rToIndex = s.indexOf(parser, rFromIndex);
    if (index == parserCnt)
    {
      if (s.substring(rFromIndex, rToIndex) == "")
        break;
    }
    else
      parserCnt++;
    rs = s.substring(rFromIndex, rToIndex);
  }
  return rs;
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
            Serial.printf("Payload size [%d] bytes.\r\n", len);
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
                    // Serial.printf("len = %d\r\n",len);
                    // open file in append mode.
                    uploadFile.write(buff, c);
                    wb += c;
                    mySd.per = map(wb,0,mySd.totalLen,0,100);
                    currentPer = mySd.per;
                    Serial.printf("%d %%.... Downloaded \r\n",currentPer);


                    server.on("/", []() {
                      handleRoot();
                    });
                    server.on("/showPer", handlePer);
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