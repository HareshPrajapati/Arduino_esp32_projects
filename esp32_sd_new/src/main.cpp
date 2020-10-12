/* Includes */
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <unity.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include "Arduino_HWSPI.h"
#include "Arduino_ESP32SPI.h"
#include "Arduino_SWSPI.h"
#include "Arduino_GFX.h"     // Core graphics library
#include "Arduino_Display.h" // Various display driver



#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

/* Defines*/
#define BLINKED_LED      (26)
#define SD_CS_PIN        (5)
#define FILE_COUNT_MAX   (2)
#define TFT_DC           (2)
#define SCLK             (14)               // (18)
#define MOSI             (13)              // (23)
#define MISO             (12)             // (19)
#define TFT_CS           (15)
#define TFT_RST          (4)
#define TOUCH_IRQ_PIN    (35)
#define TOUCH_CS_PIN     (33)

#define TFT_BL           (22)
#define TFT_BRIGHTNESS   (128)

#define TFT_NORMAL          1
#define TFT_UPSIDEDOWN      3




/* Constants */
const uint8_t RETRY_COUNT_MAX = 5;
const char * const WIFI_SSDI  = "Mikrotroniks";
const char * const WIFI_PASS  = "51251251";
const uint8_t TFT_ORIENTATION = TFT_NORMAL;
const char * const FILE_ADDRESS[FILE_COUNT_MAX] = {
    "https://upload.wikimedia.org/wikipedia/commons/thumb/3/36/Hopetoun_falls.jpg/220px-Hopetoun_falls.jpg",
    "https://5.imimg.com/data5/OK/OW/MY-37775609/lotus-flower-500x500.jpg",
    // "https://previews.123rf.com/images/jeedkob11/jeedkob111805/jeedkob11180500009/101794142-begonia-flowers-full-blooming-in-flower-garden-for-background-and-wallpaper-beautiful-natural-red-an.jpg",
    // "https://image.freepik.com/free-vector/watercolor-natural-background-with-landscape_23-2148244911.jpg",
    // "https://images.indianexpress.com/2020/09/kolkata-coronavirus-students-1200.jpg",
    // "https://upload.wikimedia.org/wikipedia/en/thumb/4/41/Flag_of_India.svg/125px-Flag_of_India.svg.png",
    // "https://fivethirtyeight.com/wp-content/uploads/2020/05/GettyImages-1161194037-1-e1589840315279.jpg",
    // "https://static.india.com/wp-content/uploads/2020/06/cricket-1.jpg",
    // "https://lattukids.com/wp-content/uploads/2019/08/JJ_PNG_016-1024x932-1024x585.png",
    // "https://pmcvariety.files.wordpress.com/2020/08/1182-the-batman-dc-fandome-teaser-youtube-5.png"
};

/* Global Variables */
File uploadFile;
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, 14 /* SCK */,13 /* MOSI */, 12 /* MISO */, VSPI /* spi_num */);
Arduino_ILI9341 *tft = new Arduino_ILI9341(bus, TFT_RST, 0 /* rotation */);
// Adafruit_ILI9341 tft = Adafruit_ILI9341( TFT_CS, TFT_DC, TFT_RST );
XPT2046_Touchscreen touch(TOUCH_CS_PIN, TOUCH_IRQ_PIN);
TS_Point rawLocation;
bool SDPresent = false;
bool allDownloaded = false;
bool allRetry = false;
bool downloadResult[FILE_COUNT_MAX] = {false};
int  retryCount, currentFileIndex = 0;
int  ledState = LOW;
unsigned long previousMillis = 0;
const long interval = 500UL;           // interval at which to blink (milliseconds)
TaskHandle_t task1,task2;
enum State{
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
bool downloadFile(const char * const fileURL);
void fileList(const char *startPath = "/");
void feedTheDog();
void fileDownloader(void);
void taskFileDownload(void * parameters );
void taskBlinkLED(void * pvParameters);
static inline __attribute__((always_inline)) float mapFloat( float x, const float in_min, const float in_max, const float out_min, const float out_max);

void setup() {

    Serial.begin(921600UL);
    Serial.setDebugOutput(true);

    pinMode(BLINKED_LED, OUTPUT);

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
    if (!SD.begin(SD_CS_PIN))
    {
        Serial.println(F("Card failed or not present.."));
        SDPresent = false;
    }
    else
    {
        Serial.println(F("Card initialised... file access enabled..."));
        SDPresent = true;
    }
    // SPI.begin(SCLK,MISO,MOSI);
    tft->begin();
    tft->setRotation(TFT_ORIENTATION);
    tft->fillScreen(ILI9341_BLACK);
#ifdef TFT_BL
    ledcAttachPin(TFT_BL, 1);     // assign TFT_BL pin to channel 1
    ledcSetup(1, 12000, 8);       // 12 kHz PWM, 8-bit resolution
    ledcWrite(1, TFT_BRIGHTNESS); // brightness 0 - 255
#endif


    if(!touch.begin()) {
        Serial.println("Touch initialization failed");
    }else {
        Serial.println("Touch screen ready.");
    }

    /* create a tasks taskFileDownload with priority 1 and
       raskBlinkLed with priority 2 that will be executed on core 0 */
    xTaskCreatePinnedToCore(taskFileDownload,"File Downloader",10000,NULL,1,&task1,0); // pin task to core 0
    delay(1);
    xTaskCreatePinnedToCore(taskBlinkLED,"LED Blinking",10000,NULL,2,&task2,1); // pin task to core 1
    delay(1);
}


void loop() {

}

void printDirectory(const char *directoryName, uint8_t levels) {
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

void deleteFile(fs::FS &fs, const char *filePath) {
    Serial.printf("Deleting file: %s\n", filePath);
    if (fs.remove(filePath)) {
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void fileList(const char *startPath) {
    if (SDPresent) {
        File root = SD.open(startPath);
        if (root) {
            root.rewindDirectory();
            printDirectory(startPath, 0);
            root.close();
        }
    }
}

bool downloadFile(const char * const fileURL) {
    bool result = false;
    char *fileName = strrchr(fileURL, '/' );
    deleteFile(SD, fileName);
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
            uploadFile = SD.open(fileName, FILE_WRITE);
            // get lenght of document (is -1 when Server sends no Content-Length header)
            int len = Http.getSize();
            Serial.printf("Payload size [%d] bytes.\r\n", len);
            // create buffer for read
            uint8_t buff[2048] = {0};
            // get tcp stream
            WiFiClient *stream = Http.getStreamPtr();
            // read all data from server
            while (Http.connected() && (len > 0 || len == -1)){
                // get available data size
                size_t size = stream->available();
                if (size){   // read up to 128 byte
                    int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                    // write it to Serial
                    Serial.printf("%d bytes read[c].\r\n", c);
                    Serial.printf("%d bytes available for read \r\n", size);
                    // open file in append mode.
                    uploadFile.write(buff, c);
                    // close file.
                    if (len > 0){
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
    else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", Http.errorToString(httpCode).c_str());
    }
    Http.end();
    uploadFile.close();
    return (result);
}

void feedTheDog() {
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;
}

void taskFileDownload(void *pvParameters)
{
    Serial.print("fileDownloader running on core ");
    Serial.println(xPortGetCoreID());
    for (;;) {
        fileDownloader();
        feedTheDog();
    }
    vTaskDelete(NULL);
}

void taskBlinkLED(void *pParameters) {
    Serial.print("LEDBlinking is running on core ");
    Serial.println(xPortGetCoreID());
    for (;;) {
    if ( touch.touched() )
    {
        rawLocation = touch.getPoint();
        // Serial.print("x = ");
        // Serial.print(rawLocation.x);
        // Serial.print(", y = ");
        // Serial.print(rawLocation.y);
        // Serial.print(", z = ");
        // Serial.println(rawLocation.z);

        if (TFT_ORIENTATION == TFT_UPSIDEDOWN)
        {
            tft->fillCircle(mapFloat(rawLocation.x, 340, 3900, 0, 320),
                        mapFloat(rawLocation.y, 200, 3850, 0, 240),2,
                        ILI9341_GREEN);
                        delay(1);
        }
        if (TFT_ORIENTATION == TFT_NORMAL)
        {
            tft->fillCircle(mapFloat(rawLocation.x, 340, 3900, 320, 0),
                        mapFloat(rawLocation.y, 200, 3850, 240, 0),2,
                        ILI9341_GREEN);
                        delay(1);
        }
    }

        feedTheDog();
    }
    vTaskDelete(NULL);
}


void fileDownloader(void)
{
    switch (currentState) {
    case STATE_START:
        currentState = STATE_DISPLAY_DIRECTORY;
        break;

    case STATE_DISPLAY_DIRECTORY:
        if (SDPresent) {
            uint64_t cardSize = SD.cardSize() / (1024UL * 1024UL);
            Serial.printf("SD Card Size: %lluMB\n", cardSize);
            fileList();
        }
        currentState = STATE_START_DOWNLOAD;
        break;

    case STATE_START_DOWNLOAD:
        if (WiFi.status() == WL_CONNECTED) {
            currentState = STATE_DOWNLOAD_FILES;
        }
        break;
    case STATE_DOWNLOAD_FILES:
        retryCount = 0;
        if (currentFileIndex < FILE_COUNT_MAX) {
            while ((downloadResult[currentFileIndex] == false) && (retryCount++ < RETRY_COUNT_MAX)) {
                downloadResult[currentFileIndex] = downloadFile(FILE_ADDRESS[currentFileIndex]);
                if (downloadResult[currentFileIndex] == false) {
                    Serial.printf("Retrying...%d \r\n",retryCount);
                }

            }
            currentFileIndex++;
        }
        else {
            currentState = STATE_DOWNLOAD_COMPLETE;
        }
        break;

    case STATE_DOWNLOAD_COMPLETE:
        Serial.printf("[#]:[URL] -> [RESULT]\r\n");
        for (size_t index = 0; index < FILE_COUNT_MAX; index++) {
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

static inline __attribute__((always_inline)) float mapFloat( float x, const float in_min, const float in_max, const float out_min, const float out_max)
{
  return ( x - in_min ) * ( out_max - out_min ) / ( in_max - in_min ) + out_min;
}