#include <Arduino.h>
#include <SPIFFS.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiAP.h>
#define LED_BUILTIN 2

String split(String s, char parser, int index);
unsigned char h2int(char c);
String urlencode(String str);
String urldecode(String str);
String file_size(int bytes);
void handleFileUpload();
void deleteConfirm();
void doDelete();
void handleNotFound();
void handleRoot();
bool handleFileRead(String path);
String getContentType(String filename);

const char* ssid     = "Mikrotroniks";
const char* password = "51251251";
const char *host     = "192.168.29.253";
const char *header   = "<!DOCTYPE html>"
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

const char* script ="<script>function sortTable(l){var e=document.getElementById(\"theader\"),n=e.cells[l].dataset.order||\"1\",s=0-(n=parseInt(n,10));e.cells[l].dataset.order=s;var t,a=document.getElementById(\"tbody\"),r=a.rows,d=[];for(t=0;t<r.length;t++)d.push(r[t]);for(d.sort(function(e,t){var a=e.cells[l].dataset.value,r=t.cells[l].dataset.value;return l?(a=parseInt(a,10),(r=parseInt(r,10))<a?s:a<r?n:0):r<a?s:a<r?n:0}),t=0;t<d.length;t++)a.appendChild(d[t])}</script>";

String uploadPath = "";
WebServer server(80);

String getContentType(String filename) {
  filename.toUpperCase();
  if (server.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".HTM")) return "text/html";
  else if (filename.endsWith(".HTML")) return "text/html";
  else if (filename.endsWith(".CSS")) return "text/css";
  else if (filename.endsWith(".JS")) return "application/javascript";
  else if (filename.endsWith(".PNG")) return "image/png";
  else if (filename.endsWith(".GIF")) return "image/gif";
  else if (filename.endsWith(".JPG")) return "image/jpeg";
  else if (filename.endsWith(".ICO")) return "image/x-icon";
  else if (filename.endsWith(".XML")) return "text/xml";
  else if (filename.endsWith(".PDF")) return "application/x-pdf";
  else if (filename.endsWith(".ZIP")) return "application/x-zip";
  else if (filename.endsWith(".GZ")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SD.exists(pathWithGz) || SD.exists(path)) {
    if (SD.exists(pathWithGz))
      path += ".gz";
    File file = SD.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleRoot() {

  String directory = urldecode(server.uri());
  uploadPath = directory;
  File dir = SD.open(directory);
  String entryName = "";
  bool emptyFolder = true;
  String tree = "";
  while (true) {
    File entry =  dir.openNextFile();
    entryName = entry.name();
    entryName.replace(directory + "/", "");

    if (! entry) {
      // no more files
      break;
    }

    if (entry.isDirectory()) {
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
    } else {
      tree += F("<tr>");
      tree += F("<td data-value=\"");
      tree += entry.name();
      tree += F("\"><a class=\"icon file\" draggable=\"true\" href=\"");
      tree += entry.name();
      tree +=  F("\">");
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
  while (count >= parserCnt) {
    rFromIndex = rToIndex + 1;
    rToIndex = path.indexOf('/', rFromIndex);
    if (count == parserCnt) {
      if (rToIndex == 0 || rToIndex == -1) break;
      tempElement = lastElement;
      lastElement = path.substring(rFromIndex, rToIndex);
    } else parserCnt++;
  }
  /*
    Serial.print("directory:");
    Serial.println(directory);
    Serial.print("path:");
    Serial.println(path);
    Serial.print("lastElement:");
    Serial.println(lastElement);
  */
  String webpage = "";
  webpage += F(header);
  webpage += F("<h1 id=\"header\">MikroTroniks Sd Card");
  webpage += directory;
  webpage += F("</h1>");

  if (directory != "/") {
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

  webpage += F(script);

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

  webpage += F("<h1>Uploader</h1>");
  webpage += F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>");
  webpage += F("<input class='url' type='url' name='fupload' id = 'fupload'  size='60'  value=''>");
  webpage += F("<button class='buttons' style='margin-left:50px' type='submit'>Upload</button></form><br>");



  if (tree == "") {
    String dlPath = urldecode(server.uri());
    if (SD.exists(dlPath)) {
      File entry = SD.open(dlPath);
      if (!entry.isDirectory()) {
        Serial.println(dlPath);
        handleFileRead(dlPath);
      }
    }
    else {
      handleNotFound();
    }
  }

  server.send(200, "text/html", webpage);
}

void handleNotFound() {
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
  for (uint8_t i = 0; i < server.args(); i++) {
    webpage += server.argName(i) + ": " + server.arg(i) + "<br>";
  }
  webpage += F("<br><button class='buttons' onclick=\"location.href='/';\">OK</button>");
  server.send(404, "text/html", webpage);
}

void doDelete() {

  String webpage = "";
  webpage += F(header);
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "file") {
      Serial.printf("Deleting file: %s\n", server.arg(i));

      webpage += F("<hr>Deleting file: <br>");
      webpage += server.arg(i);
      if (SD.remove(server.arg(i))) {
        webpage += F("<br>File deleted<br>");

      } else {
        webpage += F("<br>Delete failed<br>");

      }
    }
    if (server.argName(i) == "folder") {
      Serial.printf("Removing Dir: %s\n", server.arg(i));
      webpage += F("<hr>Removing Dir: <br>");
      webpage += server.arg(i);
      if (SD.rmdir(server.arg(i))) {
        webpage += F("<br>Dir removed<br>");

      } else {
        webpage += F("<br>rmdir failed<br>");
      }
    }
  }
  webpage += F("<br><button class='buttons' onclick=\"location.href='/';\">OK</button>");
  server.send(200, "text/html", webpage);
}

void deleteConfirm() {
  String webpage = "";
  webpage += F(header);
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "file") {
      webpage += F("<hr>Do you want to delete the file:<br>");
      webpage += server.arg(i);
      webpage += F("<br><br><button class='buttons' onclick=\"location.href='/doDelete?file=");
      webpage += server.arg(i);
      webpage += F("';\">Yes</button>");

    }
    if (server.argName(i) == "folder") {
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

File UploadFile;
  // upload a new file to the Filing system
void handleFileUpload() {
  HTTPUpload& uploadfile = server.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
  // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
  if (uploadfile.status == UPLOAD_FILE_START)
  {
    String filename = uploadfile.filename;
    if (!filename.startsWith("/")) filename = uploadPath + "/" + filename;
    SD.remove(filename);                         // Remove a previous version, otherwise data is appended the file again
    UploadFile = SD.open(filename, FILE_WRITE); // Open the file for writing in SPIFFS (create it, if doesn't exist)
    filename = String();
  }
  else if (uploadfile.status == UPLOAD_FILE_WRITE)
  {
    if (UploadFile) {
      UploadFile.write(uploadfile.buf, uploadfile.currentSize); // Write the received bytes to the file
    }
  }
  else if (uploadfile.status == UPLOAD_FILE_END)
  {
    if (UploadFile)         // If the file was successfully created
    {
      UploadFile.close();   // Close the file again
      Serial.print("Upload Size: "); Serial.println(uploadfile.totalSize);

      String webpage = "";
      webpage += F(header);
      webpage += F("<hr>File was successfully uploaded<br>");
      webpage += F("Uploaded File Name: ");
      webpage += uploadfile.filename + "<br>";
      webpage += F("File Size: ");
      webpage += file_size(uploadfile.totalSize) + "<br>";
      webpage += "<button class='buttons' onclick='window.history.back();'>OK</button>";
      server.send(200, "text/html", webpage);
    }
    else
    {
      delay(1000);
      String webpage = "";
      webpage += F(header);
      webpage += F("<hr>Could Not Create Uploaded File (write-protected?)<br>");
      webpage += "<button class='buttons' onclick='window.history.back();'>OK</button>";
      server.send(200, "text/html", webpage);
    }
  }
}

String file_size(int bytes) {
  String fsize = "";
  if (bytes < 1024)                 fsize = String(bytes) + " B";
  else if (bytes < (1024 * 1024))      fsize = String(bytes / 1024.0, 3) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
  else                              fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
  return fsize;
}

String urldecode(String str)
{

  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == '+') {
      encodedString += ' ';
    } else if (c == '%') {
      i++;
      code0 = str.charAt(i);
      i++;
      code1 = str.charAt(i);
      c = (h2int(code0) << 4) | h2int(code1);
      encodedString += c;
    } else {

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
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
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
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}

String split(String s, char parser, int index) {
  String rs = "";
  int parserIndex = index;
  int parserCnt = 0;
  int rFromIndex = 0, rToIndex = -1;
  while (index >= parserCnt) {
    rFromIndex = rToIndex + 1;
    rToIndex = s.indexOf(parser, rFromIndex);
    if (index == parserCnt) {
      if (s.substring(rFromIndex, rToIndex) == "")
        break;
    } else parserCnt++;
    rs = s.substring(rFromIndex, rToIndex);
  }
  return rs;
}

void setup(void) {

  Serial.begin(921600UL);
  Serial.setDebugOutput(true);
  Serial.print("Initializing SD card...");


  if (!SD.begin(SS)) {

    Serial.println("Card Mount Failed");

    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {

    Serial.println("No SD card attached");

    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {

    Serial.println("MMC");
  } else if (cardType == CARD_SD) {

    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {

    Serial.println("SDHC");
  } else {

    Serial.println("UNKNOWN");
  }


  Serial.println();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20)
  { //wait 10 seconds
    delay(500);
  }
  if (i == 21)
  {
    Serial.print("Could not connect to");
    Serial.println(ssid);
    while (1)
    {
      delay(500);
    }
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

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

  server.on("/fupload",  HTTP_POST, []() {
    server.send(200);
  }, handleFileUpload);

  server.on("/deleteConfirm", deleteConfirm);
  server.on("/doDelete", doDelete);
  server.begin();
  Serial.println("\r\nHTTP server started");



  Serial.println();

  Serial.println("Initialization done.");

}

void loop(void) {

  server.handleClient();
}
