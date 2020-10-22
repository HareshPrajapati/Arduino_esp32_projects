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


void SDcard::feedTheDog() {
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;
}

bool SDcard::fileDownloader(String fileUrl) {

    switch (currentState) {
    case STATE_START: {
        Serial.println("STATE_START");
        currentState = STATE_DISPLAY_DIRECTORY;
    }
    break;
    case STATE_DISPLAY_DIRECTORY:  {   
        Serial.println("STATE_DISPLAY_DIRECTORY");
        if (SDPresent) {
            uint64_t cardSize = SD.cardSize() / (1024UL * 1024UL);
            Serial.printf("SD Card Size: %lluMB\n", cardSize);
            fileList();
        }
        currentState = STATE_START_DOWNLOAD;

    }
    break;
    case STATE_START_DOWNLOAD:{   
        Serial.println(" STATE_START_DOWNLOAD");
        if (WiFi.status() == WL_CONNECTED) {
            currentState = STATE_DOWNLOAD_FILES;
        }
    }
    break;
    case STATE_DOWNLOAD_FILES:{    
        Serial.println("STATE_DOWNLOAD_FILES");
        retryCount = 0;
        if (currentFileIndex < FILE_COUNT_MAX) {
            while ((downloadResult[currentFileIndex] == false) && (retryCount++ < RETRY_COUNT_MAX)) {
                // downloadResult[currentFileIndex] = downloadFile((const char *)fileUrl.c_str());
                if (downloadResult[currentFileIndex] == false) {
                    Serial.printf("Retrying...%d \r\n", retryCount);
                }
            }
            currentFileIndex++;
        } else {
            currentState = STATE_DOWNLOAD_COMPLETE;
        }
    }
    break;
    case STATE_DOWNLOAD_COMPLETE: {     
        Serial.println("STATE_DOWNLOAD_COMPLETE");
        Serial.printf("[#]:[URL] -> [RESULT]\r\n");
        for (size_t index = 0; index < FILE_COUNT_MAX; index++) {
            Serial.printf("[%d]:[%s] -> [%s]\r\n", index, fileUrl.c_str(), downloadResult[index] ? "SUCCESS" : "FAILED");
        }
        currentState = STATE_DISPLAY_DOWNLOAD_INFO;
    }
    break;

    case STATE_DISPLAY_DOWNLOAD_INFO: {    
        Serial.println("STATE_DISPLAY_DOWNLOAD_INFO");
        fileList("/");
        currentState = STATE_IDLE;
    }
    break;
    case STATE_IDLE:{     
        Serial.println(" STATE_IDLE");
        return true;
    }
    break;
    }
}
