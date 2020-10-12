#ifndef SDCard_h
#define SDCard_h


#include <SD.h>
#include <FS.h>
#include <HTTPClient.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"






#define FILE_COUNT_MAX   (1)
#define SD_CS_PIN        (5)

const uint8_t RETRY_COUNT_MAX = 5;
// const char * const FILE_ADDRESS[FILE_COUNT_MAX] = {
//     "https://cdn.pixabay.com/photo/2015/04/23/22/00/tree-736885_960_720.jpg",
//     "https://cdn.pixabay.com/photo/2016/01/08/11/57/butterfly-1127666_960_720.jpg",
//     "https://cdn.pixabay.com/photo/2016/11/14/04/45/elephant-1822636_960_720.jpg",
//     "https://image.freepik.com/free-vector/watercolor-natural-background-with-landscape_23-2148244911.jpg",
//     "https://www.w3.org/TR/PNG/iso_8859-1.txt",
//     "https://upload.wikimedia.org/wikipedia/en/thumb/4/41/Flag_of_India.svg/125px-Flag_of_India.svg.png",
//     "https://fivethirtyeight.com/wp-content/uploads/2020/05/GettyImages-1161194037-1-e1589840315279.jpg",
//     "https://static.india.com/wp-content/uploads/2020/06/cricket-1.jpg",
//     "https://lattukids.com/wp-content/uploads/2019/08/JJ_PNG_016-1024x932-1024x585.png",
//     "https://pmcvariety.files.wordpress.com/2020/08/1182-the-batman-dc-fandome-teaser-youtube-5.png"
// };





enum State{
    STATE_START,
    STATE_DISPLAY_DIRECTORY,
    STATE_START_DOWNLOAD,
    STATE_DOWNLOAD_FILES,
    STATE_DOWNLOAD_COMPLETE,
    STATE_DISPLAY_DOWNLOAD_INFO,
    STATE_IDLE
};

class SDcard {
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
    // bool downloadFile(const char *const fileURL);
    void fileList(const char *startPath = "/");
    void feedTheDog();
    bool fileDownloader(String fileArg);
};

#endif