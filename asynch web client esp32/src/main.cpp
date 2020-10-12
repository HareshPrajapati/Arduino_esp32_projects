/* Includes */
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

/* Defines*/
#define SD_CS_PIN (5)
#define FILE_COUNT_MAX (3)

/* Constants */
const uint8_t RETRY_COUNT_MAX = 5;
const char *const WIFI_SSDI = "Mikrotroniks";
const char *const WIFI_PASS = "51251251";

const char *const FILE_ADDRESS[FILE_COUNT_MAX] = {
    "https://upload.wikimedia.org/wikipedia/commons/thumb/3/36/Hopetoun_falls.jpg/220px-Hopetoun_falls.jpg",
    "https://5.imimg.com/data5/OK/OW/MY-37775609/lotus-flower-500x500.jpg",
    "https://previews.123rf.com/images/jeedkob11/jeedkob111805/jeedkob11180500009/101794142-begonia-flowers-full-blooming-in-flower-garden-for-background-and-wallpaper-beautiful-natural-red-an.jpg"};

/* Global Variables */
File uploadFile;
bool SDPresent = false;
bool allDownloaded = false;
bool allRetry = false;
bool downloadResult[FILE_COUNT_MAX] = {false};
int retryCount, currentFileIndex = 0;
int ledState = LOW;
unsigned long previousMillis = 0;
const long interval = 500UL; // interval at which to blink (milliseconds)
TaskHandle_t task1, task2;

enum State
{
    STATE_START,
    STATE_DISPLAY_DIRECTORY,
    STATE_START_DOWNLOAD,
    STATE_DOWNLOAD_FILES,
    STATE_DOWNLOAD_COMPLETE,
    STATE_DISPLAY_DOWNLOAD_INFO,
    STATE_IDLE
};
State currentState;

/* Function Declarations */
void printDirectory(const char *directoryName, uint8_t levels);
void deleteFile(fs::FS &fs, const char *filePath);
bool downloadFile(const char *const fileURL);
void fileList(const char *startPath = "/");
void feedTheDog();
void fileDownloader(void);
void taskFileDownload(void *parameters);
void taskBlinkLED(void *pvParameters);

void setup()
{

    Serial.begin(921600UL);
    Serial.setDebugOutput(true);

    pinMode(LED_BUILTIN, OUTPUT);
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

    if (!SD.begin(SD_CS_PIN, SPI, 8000000UL))
    {
        Serial.println(F("Card failed or not present.."));
        SDPresent = false;
    }
    else
    {
        Serial.println(F("Card initialised... file access enabled..."));
        SDPresent = true;
    }

    static AsyncClient *aClient = NULL;

    if (aClient) //client already exists
        return;

    aClient = new AsyncClient();
    if (!aClient) //could not allocate client
        return;

    aClient->onError([](void *arg, AsyncClient *client, int error) {
        Serial.println("Connect Error");
        aClient = NULL;
        delete client;
    },NULL);

    aClient->onConnect([](void *arg, AsyncClient *client) {
        Serial.println("Connected");
        aClient->onError(NULL, NULL);

        client->onDisconnect([](void *arg, AsyncClient *c) {
            Serial.println("Disconnected");
            aClient = NULL;
            delete c;
        },NULL);

        client->onData([](void *arg, AsyncClient *c, void *data, size_t len) {
            Serial.print("\r\nData: ");
            Serial.println(len);
            uint8_t *d = (uint8_t *)data;

            for (size_t i = 0; i < len; i++) {
                Serial.write(d[i]);
            }


        },NULL);
        //send the request
        client->write("GET / HTTP/1.0\r\nHost: www.google.com\r\n\r\n");
    },NULL);

    if (!aClient->connect("www.google.com", 80))
    {
        Serial.println("Connect Fail");
        AsyncClient *client = aClient;
        aClient = NULL;
        delete client;
    }
}

void loop()
{
}

void printDirectory(const char *directoryName, uint8_t levels)
{
    File root = SD.open(directoryName);
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
        if (file.isDirectory())
        {
            Serial.println(String(file.isDirectory() ? "Dir " : "File ") + String(file.name()));
            printDirectory(file.name(), levels - 1);
        }
        else
        {
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

void deleteFile(fs::FS &fs, const char *filePath)
{
    Serial.printf("Deleting file: %s\n", filePath);
    if (fs.remove(filePath))
    {
        Serial.println("File deleted");
    }
    else
    {
        Serial.println("Delete failed");
    }
}

void fileList(const char *startPath)
{
    if (SDPresent)
    {
        File root = SD.open(startPath);
        if (root)
        {
            root.rewindDirectory();
            printDirectory(startPath, 0);
            root.close();
        }
    }
}

bool downloadFile(const char *const fileURL)
{
    bool result = false;
    char *fileName = strrchr(fileURL, '/');
    deleteFile(SD, fileName);
    HTTPClient Http;
    Serial.print("[HTTP] begin...\n");
    // configure server and url
    Http.begin(fileURL);
    // http.begin("media.gettyimages.com", 80, "/photos/red-heart-shape-balloon-flying-above-buildings-in-city-picture-id608954473");
    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = Http.GET();
    if (httpCode > 0)
    {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
        // file found at server
        if (httpCode == HTTP_CODE_OK)
        {
            uploadFile = SD.open(fileName, FILE_WRITE);
            // get lenght of document (is -1 when Server sends no Content-Length header)
            int len = Http.getSize();
            Serial.printf("Payload size [%d] bytes.\r\n", len);
            // create buffer for read
            uint8_t buff[2048] = {0};
            // get tcp stream
            WiFiClient *stream = Http.getStreamPtr();
            // read all data from server
            while (Http.connected() && (len > 0 || len == -1))
            {
                // get available data size
                size_t size = stream->available();
                if (size)
                { // read up to 128 byte
                    int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                    // write it to Serial
                    Serial.printf("%d bytes read[c].\r\n", c);
                    Serial.printf("%d bytes available for read \r\n", size);
                    // open file in append mode.
                    uploadFile.write(buff, c);
                    // close file.
                    if (len > 0)
                    {
                        len -= c;
                    }
                }
                delayMicroseconds(1);
            }
            Serial.println();
            Serial.print("[HTTP] connection closed or file end.\n");
            result = true;
        }
    }
    else
    {
        Serial.printf("[HTTP] GET... failed, error: %s\n", Http.errorToString(httpCode).c_str());
    }
    Http.end();
    uploadFile.close();
    return (result);
}

void feedTheDog()
{
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;
}

void taskFileDownload(void *pvParameters)
{
    Serial.print("fileDownloader running on core ");
    Serial.println(xPortGetCoreID());
    for (;;)
    {
        fileDownloader();
        feedTheDog();
    }
    vTaskDelete(NULL);
}

void taskBlinkLED(void *pParameters)
{
    Serial.print("LEDBlinking is running on core ");
    Serial.println(xPortGetCoreID());
    for (;;)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        vTaskDelay(500UL / portTICK_PERIOD_MS);
        digitalWrite(LED_BUILTIN, LOW);
        vTaskDelay(500UL / portTICK_PERIOD_MS);
        feedTheDog();
    }
    vTaskDelete(NULL);
}

void fileDownloader(void)
{
    switch (currentState)
    {
    case STATE_START:
        currentState = STATE_DISPLAY_DIRECTORY;
        break;

    case STATE_DISPLAY_DIRECTORY:
        if (SDPresent)
        {
            uint64_t cardSize = SD.cardSize() / (1024UL * 1024UL);
            Serial.printf("SD Card Size: %lluMB\n", cardSize);
            fileList();
        }
        currentState = STATE_START_DOWNLOAD;
        break;

    case STATE_START_DOWNLOAD:
        if (WiFi.status() == WL_CONNECTED)
        {
            currentState = STATE_DOWNLOAD_FILES;
        }
        break;
    case STATE_DOWNLOAD_FILES:
        retryCount = 0;
        if (currentFileIndex < FILE_COUNT_MAX)
        {
            while ((downloadResult[currentFileIndex] == false) && (retryCount++ < RETRY_COUNT_MAX))
            {
                downloadResult[currentFileIndex] = downloadFile(FILE_ADDRESS[currentFileIndex]);
            }
            currentFileIndex++;
        }
        else
        {
            currentState = STATE_DOWNLOAD_COMPLETE;
        }
        break;

    case STATE_DOWNLOAD_COMPLETE:
        Serial.printf("[#]:[URL] -> [RESULT]\r\n");
        for (size_t index = 0; index < FILE_COUNT_MAX; index++)
        {
            Serial.printf("[%d]:[%s] -> [%s]\r\n", index, FILE_ADDRESS[index], downloadResult[index] ? "SUCCESS" : "FAILED");
        }
        currentState = STATE_DISPLAY_DOWNLOAD_INFO;
        break;

    case STATE_DISPLAY_DOWNLOAD_INFO:
        fileList("/");
        currentState = STATE_IDLE;
        break;

    case STATE_IDLE:
        break;
    }
}