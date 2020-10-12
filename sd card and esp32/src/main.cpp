#include <Arduino.h>
#include <WiFi.h>           // Built-in
#include <WiFiMulti.h>      // Built-in
#include <ESP32WebServer.h> // https://github.com/Pedroalbuquerque/ESP32WebServer download and place in your Libraries folder
#include <ESPmDNS.h>
#include "HTTPClient.h"
#include "FS.h"
#include "CSS.h"
#include <SD.h>
#include <SPI.h>


#define servername "192.168.29.253" //  address is http://192.168.29.253/
#define SD_CS_pin 5

WiFiMulti wifiMulti;
const char *host = "192.168.29.253 ";
ESP32WebServer server(80);
File uploadFile;
const char ssid_1[] = "Mikrotroniks";
const char password_1[] = "51251251";
bool SD_present = false;


  File root;
void returnOK();
void handleFileUpload();
void HomePage();
void File_Download();
void SD_file_download(String filename);
void File_Upload();
void handleFileUpload();
void SD_dir();
void printDirectory(const char *dirname, uint8_t levels);
void File_Stream();
void SD_file_stream(String filename);
void File_Delete();
void SD_file_delete(String filename);
void SendHTML_Header();
void SendHTML_Content();
void SendHTML_Stop();
void SelectInput(String heading1, String command, String arg_calling_name);
void ReportSDNotPresent();
void ReportFileNotPresent(String target);
void ReportCouldNotCreateFile(String target);
String file_size(int bytes);
void deleteRecursive(String path);
void handleDelete();


// All supporting functions from here...
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void HomePage()
{
  // SendHTML_Header();
  // webpage += F("<a href='/download'><button>Download</button></a>");
  // webpage += F("<a href='/upload'><button>Upload</button></a>");
  // webpage += F("<a href='/stream'><button>Stream</button></a>");
  // webpage += F("<a href='/delete'><button>Delete</button></a>");
  // webpage += F("<a href='/dir'><button>Directory</button></a>");
  SD_dir();
  // append_page_footer();
  // SendHTML_Content();
  // SendHTML_Stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Download()
{ // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
  if (server.args() > 0)
  { // Arguments were received

    if (server.hasArg("download"))
      SD_file_download(server.arg(0));
  }
  else
    SelectInput("Enter filename to download", "download", "download");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void SD_file_download(String filename)
{
  if (SD_present)
  {
    File download = SD.open("/" + filename);
    if (download)
    {
      server.sendHeader("Content-Type", "text/text");
      server.sendHeader("Content-Disposition", "attachment; filename=" + filename);
      server.sendHeader("Connection", "close");
      server.streamFile(download, "application/octet-stream");
      download.close();
    }
    else
      ReportFileNotPresent("download");
  }
  else
    ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Upload()
{
  Serial.println("File upload stage-1");
  append_page_header();
  webpage += F("<h3>Select File to Upload</h3>");
  webpage += F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>");
  webpage += F("<input class='url' style='width:40%'  name='fupload' id = 'fupload' value=''><br>");
  webpage += F("<br><button class='buttons' style='width:10%' type='submit'>Upload File</button><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  Serial.println("File upload stage-2");
  server.send(200, "text/html", webpage);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
File UploadFile;
void handleFileUpload()
{ // upload a new file to the Filing system

  Serial.println("File upload stage-3");
  HTTPUpload &uploadfile = server.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
                                            // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
  if (uploadfile.status == UPLOAD_FILE_START)
  {
    Serial.println("File upload stage-4");
    String filename = uploadfile.filename;
    if (!filename.startsWith("/"))
      filename = "/" + filename;
    Serial.print("Upload File Name: ");
    Serial.println(filename);
    SD.remove(filename);                        // Remove a previous version, otherwise data is appended the file again
    UploadFile = SD.open(filename, FILE_WRITE); // Open the file for writing in SPIFFS (create it, if doesn't exist)
    filename = String();
  }
  else if (uploadfile.status == UPLOAD_FILE_WRITE)
  {
    Serial.println("File upload stage-5");
    if (UploadFile)
      UploadFile.write(uploadfile.buf, uploadfile.currentSize); // Write the received bytes to the file
  }
  else if (uploadfile.status == UPLOAD_FILE_END)
  {
    if (UploadFile) // If the file was successfully created
    {
      UploadFile.close(); // Close the file again
      Serial.print("Upload Size: ");
      Serial.println(uploadfile.totalSize);
      webpage = "";
      append_page_header();
      webpage += F("<h3>File was successfully uploaded</h3>");
      webpage += F("<h2>Uploaded File Name: ");
      webpage += uploadfile.filename + "</h2>";
      webpage += F("<h2>File Size: ");
      webpage += file_size(uploadfile.totalSize) + "</h2><br>";
      append_page_footer();
      server.send(200, "text/html", webpage);
    }
    else
    {
      ReportCouldNotCreateFile("upload");
    }
  }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_dir()
{
  if (SD_present)
  {
    File root = SD.open("/");
    if (root)
    {
      // root.rewindDirectory();
      SendHTML_Header();
      webpage += F("<table align='center'>");
      webpage += F("<tr><th>Name/Type</th>");
      printDirectory("/", 0);
      webpage += F("</table>");
      SendHTML_Content();
      root.close();
    }
    else
    {
      SendHTML_Header();
      webpage += F("<h3>No Files Found</h3>");
    }
    SendHTML_Content();
    SendHTML_Stop();   // Stop is needed because no content length was sent
  }
  else
    ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void printDirectory(const char *dirname, uint8_t levels)
{
  File root = SD.open(dirname);
#ifdef ESP8266
  root.rewindDirectory(); //Only needed for ESP8266
#endif
  if (!root)
  {
    return;
  }
  if (!root.isDirectory())
  {
    return;
  }
  File file = root.openNextFile();
  while (file)
  {
    if (webpage.length() > 1000)
    {
      SendHTML_Content();
    }
    if (file.isDirectory()) {
      // Serial.println(String(file.isDirectory() ? "Dir " : "File ") + String(file.name()));
      webpage +="<tr><td><a href='/dir'><text>" + String(file.name()) +
                "</text></a><td><a href='/delete'><button class='buttons' style='width:100%'>Delete</button></a>" +
                "</td></td></tr>";
      // webpage += F("<a href='/dir'><button>Directory</button></a>");
      printDirectory(file.name(), levels - 1);
    } else {
      webpage += "<tr><td>" + String(file.name()) + "<td><a href='/delete'><button class='buttons' style='width:100%'>Delete</button></a></td></td></tr>";
      // Serial.print(String(file.isDirectory() ? "Dir " : "File ") + String(file.name()) + "\t");
      // webpage += "<td>" + String(file.isDirectory() ? "Dir" : "File") + "</td>";
      // int bytes = file.size();
      // String fsize = "";
      // if (bytes < 1024)
      //   fsize = String(bytes) + " B";
      // else if (bytes < (1024 * 1024))
      //   fsize = String(bytes / 1024.0, 3) + " KB";
      // else if (bytes < (1024 * 1024 * 1024))
      //   fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
      // else
      //   fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
      // webpage += "<td>" + fsize + "</td></tr>";
      // Serial.println(String(fsize));
    }
    file = root.openNextFile();
  }
  file.close();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Stream()
{
  if (server.args() > 0)
  { // Arguments were received
    if (server.hasArg("stream"))
      SD_file_stream(server.arg(0));
  }
  else
    SelectInput("Enter a File to Stream", "stream", "stream");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_file_stream(String filename)
{
  if (SD_present)
  {
    File dataFile = SD.open("/" + filename, FILE_READ); // Now read data from SD Card
    Serial.print("Streaming file: ");
    Serial.println(filename);
    if (dataFile)
    {
      if (dataFile.available())
      { // If data is available and present
        String dataType = "application/octet-stream";
        if (server.streamFile(dataFile, dataType) != dataFile.size())
        {
          Serial.print(F("Sent less data than expected!"));
        }
      }
      dataFile.close(); // close the file:
    }
    else
      ReportFileNotPresent("Cstream");
  }
  else
    ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Delete()
{
  if (server.args() > 0)
  { // Arguments were received
    if (server.hasArg("delete"))
      SD_file_delete(server.arg(0));
  }
  else
    // SelectInput("Select a File to Delete", "delete", "delete");
    deleteRecursive(server.arg(0));
    // Serial.printf("not deleted /r/n");
}

void deleteRecursive(String path){
  File file = SD.open((char *)path.c_str());
  if(!file.isDirectory()){
    file.close();
    SD.remove((char *)path.c_str());
    return;
  }

  file.rewindDirectory();
  while(true) {
    File entry = file.openNextFile();
    if (!entry) break;
    String entryPath = path + "/" +entry.name();
    if(entry.isDirectory()){
      entry.close();
      deleteRecursive(entryPath);
    } else {
      entry.close();
      SD.remove((char *)entryPath.c_str());
    }
    yield();
}

  SD.rmdir((char *)path.c_str());
  file.close();
}

void returnFail(String msg) {
  server.send(500, "text/plain", msg + "\r\n");
}



void handleDelete(){
  if(server.args() == 0) return returnFail("BAD ARGS");
  String path = server.arg(0);
  if(path == "/" || !SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }
  deleteRecursive(path);
  returnOK();
}


void returnOK() {
  server.send(200, "text/plain", "");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_file_delete(String filename)
{ // Delete the file
  if (SD_present)
  {
    SendHTML_Header();
    File dataFile = SD.open("/" + filename, FILE_READ); // Now read data from SD Card
    Serial.print("Deleting file: ");
    Serial.println(filename);
    if (dataFile)
    {
      if (SD.remove("/" + filename))
      {
        Serial.println(F("File deleted successfully"));
        webpage += "<h3>File '" + filename + "' has been erased</h3>";
        webpage += F("<a href='/delete'>[Back]</a><br><br>");
      }
      else
      {
        webpage += F("<h3>File was not deleted - error</h3>");
        webpage += F("<a href='delete'>[Back]</a><br><br>");
      }
    }
    else
    ReportFileNotPresent("delete");
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();
  }
  else
    ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Header()
{
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  append_page_header();
  server.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Content()
{
  server.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Stop()
{
  server.sendContent("");
  server.client().stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SelectInput(String heading1, String command, String arg_calling_name)
{
  SendHTML_Header();
  webpage += F("<h3>");
  webpage += heading1 + "</h3>";
  webpage += F("<FORM action='/");
  webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
  webpage += F("<input type='text' name='");
  webpage += arg_calling_name;
  webpage += F("' value=''><br>");
  webpage += F("<type='submit' name='");
  webpage += arg_calling_name;
  webpage += F("' value=''><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportSDNotPresent()
{
  SendHTML_Header();
  webpage += F("<h3>No SD Card present</h3>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportFileNotPresent(String target)
{
  SendHTML_Header();
  webpage += F("<h3>File does not exist</h3>");
  webpage += F("<a href='/");
  webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportCouldNotCreateFile(String target)
{
  SendHTML_Header();
  webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>");
  webpage += F("<a href='/");
  webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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

void setup()
{
  Serial.begin(921600UL);
  Serial.setDebugOutput(true);
  Serial.print("\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_1, password_1);
  Serial.print("Connecting to ");
  Serial.println(ssid_1);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20)
  { //wait 10 seconds
    delay(500);
  }
  if (i == 21)
  {
    Serial.print("Could not connect to");
    Serial.println(ssid_1);
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
    // Serial.println(".local");
  }
  server.on("/", HomePage);
  // server.on("/delete", File_Delete);
  server.on("/delete",handleDelete);
  server.on("/dir", SD_dir);

  server.begin();
  Serial.println("\r\nHTTP server started");

  Serial.print(F("Initializing SD card..."));

  if (!SD.begin(SD_CS_pin, SPI, 8000000))
  { // see if the card is present and can be initialised. Wemos SD-Card CS uses D8
    Serial.println(F("Card failed or not present, no SD Card data logging possible..."));
    SD_present = false;
  }
  else
  {
    Serial.println(F("Card initialised... file access enabled..."));
    SD_present = true;
  }
}

void loop()
{
   server.handleClient(); // Listen for client connections
}