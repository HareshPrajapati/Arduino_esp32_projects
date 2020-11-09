#ifndef SDCard_h
#define SDCard_h

#include <SD.h>
#include <FS.h>
#include <HTTPClient.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#define FILE_COUNT_MAX (1)
#define SD_CS_PIN (5)

const uint8_t RETRY_COUNT_MAX = 5;

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

class SDcard
{
private:
    // File uploadFile;
    bool allDownloaded = false;
    bool allRetry = false;
    bool downloadResult[FILE_COUNT_MAX] = {false};
    int retryCount, currentFileIndex = 0;
    State currentState;

public:
    uint8_t per = 0;
    long totalLen = 0;
    bool SDPresent = false;
    void printDirectory(const char *directoryName, uint8_t levels);
    void deleteFile(fs::FS &fs, const char *filePath);
    void fileList(const char *startPath = "/");
    void feedTheDog();
    bool fileDownloader(String fileArg);
};

#endif