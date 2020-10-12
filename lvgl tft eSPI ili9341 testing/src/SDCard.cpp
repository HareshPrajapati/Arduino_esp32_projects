#include "SDCard.h"







void SDcard::printDirectory(const char *directoryName, uint8_t levels) {
    File root = SD.open(directoryName);
    if (!root) {
        return;
    }
    if (!root.isDirectory()) {
        return;
    }
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.println(String(file.isDirectory() ? "Dir " : "File ") + String(file.name()));
            printDirectory(file.name(), levels - 1);
        }
        else {
            Serial.print(String(file.isDirectory() ? "Dir " : "File ") + String(file.name()) + "\t");
            int bytes = file.size();
            String fsize = "";
            if (bytes < 1024)
                fsize = String(bytes) + " B";
            else if (bytes < (1024 * 1024))
                fsize = String(bytes / 1024.0, 3) + " KB";
            else if (bytes < (1024 * 1024 * 1024))
                fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
            else
                fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
            Serial.println(String(fsize));
        }
        file = root.openNextFile();
    }
    file.close();
}

void SDcard::deleteFile(fs::FS &fs, const char *filePath) {
    Serial.printf("Deleting file: %s\n", filePath);
    if (fs.remove(filePath)) {
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}


void SDcard::fileList(const char *startPath) {
    if (SDPresent) {
        File root = SD.open(startPath);
        if (root) {
            root.rewindDirectory();
            printDirectory(startPath, 0);
            root.close();
        }
    }
}


// bool SDcard::downloadFile(const char * const fileURL) {
//     bool result = false;
//     char *fileName = strrchr(fileURL, '/' );
//     deleteFile(SD, fileName);
//     HTTPClient Http;
//     Serial.print("[HTTP] begin...\n");
//     // configure server and url
//     Http.begin(fileURL);
//     // http.begin("media.gettyimages.com", 80, "/photos/red-heart-shape-balloon-flying-above-buildings-in-city-picture-id608954473");
//     Serial.print("[HTTP] GET...\n");
//     // start connection and send HTTP header
//     int httpCode = Http.GET();
//     if (httpCode > 0) {
//         // HTTP header has been send and Server response header has been handled
//         Serial.printf("[HTTP] GET... code: %d\n", httpCode);
//         // file found at server
//         if (httpCode == HTTP_CODE_OK) {
//             uploadFile = SD.open(fileName, FILE_WRITE);
//             // get lenght of document (is -1 when Server sends no Content-Length header)
//             long len = Http.getSize();
//             totalLen = len;
//             Serial.printf("Payload size [%d] bytes.\r\n", len);
//             // create buffer for read
//             uint8_t buff[2048] = {0};
//             // get tcp stream
//             WiFiClient *stream = Http.getStreamPtr();
//             long wb = 0;
//             // read all data from server
//             while (Http.connected() && (len > 0 || len == -1)){
//                 // get available data size
//                 size_t size = stream->available();
//                 if (size){   // read up to 2048 byte
//                     int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
//                     // write it to Serial
//                     Serial.printf("%d bytes read[c].\r\n", c);
//                     Serial.printf("%d bytes available for read \r\n", size);
//                     // Serial.printf("len = %d\r\n",len);
//                     // open file in append mode.
//                     uploadFile.write(buff, c);
//                     wb += c;
//                     per = map(wb,0,totalLen,0,100);
//                     currentPer = per;
//                     server.on("/showPer", handlePer);
//                     server.handleClient();
//                     // close file.
//                     if (len > 0){
//                         len -= c;
//                     }
//                 }
//                 delayMicroseconds(1);
//             }
//             Serial.println();
//             Serial.print("[HTTP] connection closed or file end.\n");
//             result = true;
//         }
//     }
//     else {
//         Serial.printf("[HTTP] GET... failed, error: %s\n", Http.errorToString(httpCode).c_str());
//     }
//     Http.end();
//     uploadFile.close();
//     return (result);
// }

void SDcard::feedTheDog() {
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;
}

bool SDcard::fileDownloader(String fileUrl)
{

    switch (currentState)
    {
    case STATE_START:
    {
        Serial.println("STATE_START");
        currentState = STATE_DISPLAY_DIRECTORY;
    }
    break;
    case STATE_DISPLAY_DIRECTORY:
    {   Serial.println("STATE_DISPLAY_DIRECTORY");
        if (SDPresent)
        {
            uint64_t cardSize = SD.cardSize() / (1024UL * 1024UL);
            Serial.printf("SD Card Size: %lluMB\n", cardSize);
            fileList();
        }
        currentState = STATE_START_DOWNLOAD;

    }
    break;
    case STATE_START_DOWNLOAD:
    {    Serial.println(" STATE_START_DOWNLOAD");
        if (WiFi.status() == WL_CONNECTED)
        {
            currentState = STATE_DOWNLOAD_FILES;
        }
    }
    break;
    case STATE_DOWNLOAD_FILES:
    {    Serial.println("STATE_DOWNLOAD_FILES");
        retryCount = 0;
        if (currentFileIndex < FILE_COUNT_MAX)
        {
            while ((downloadResult[currentFileIndex] == false) && (retryCount++ < RETRY_COUNT_MAX))
            {
                // downloadResult[currentFileIndex] = downloadFile((const char *)fileUrl.c_str());
                if (downloadResult[currentFileIndex] == false)
                {
                    Serial.printf("Retrying...%d \r\n", retryCount);
                }
            }
            currentFileIndex++;
        }
        else
        {
            currentState = STATE_DOWNLOAD_COMPLETE;
        }
    }
    break;
    case STATE_DOWNLOAD_COMPLETE:
    {     Serial.println("STATE_DOWNLOAD_COMPLETE");
        Serial.printf("[#]:[URL] -> [RESULT]\r\n");
        for (size_t index = 0; index < FILE_COUNT_MAX; index++)
        {
            Serial.printf("[%d]:[%s] -> [%s]\r\n", index, fileUrl, downloadResult[index] ? "SUCCESS" : "FAILED");
        }
        currentState = STATE_DISPLAY_DOWNLOAD_INFO;
    }
    break;

    case STATE_DISPLAY_DOWNLOAD_INFO:
    {    Serial.println("STATE_DISPLAY_DOWNLOAD_INFO");
        fileList("/");
        currentState = STATE_IDLE;
    }
    break;
    case STATE_IDLE:
    {    Serial.println(" STATE_IDLE");
        return true;
    }
    break;
    }
}
