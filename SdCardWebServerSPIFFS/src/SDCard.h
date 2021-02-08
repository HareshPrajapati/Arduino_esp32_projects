#ifndef SDCard_h
#define SDCard_h

/******************************  Includes  *******************************************/
#include <SD.h>
#include <FS.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include <debug.h>

/******************************  Defines  *********************************************/
#define FILE_COUNT_MAX (1)
#define SD_CS_PIN (5)

/****************************** Globle Const var *************************************/
const uint8_t RETRY_COUNT_MAX = 5;

class SDcard
{
private:
public:
  uint8_t per = 0;
  long totalLen = 0;
  bool SDPresent = false;
  void printDirectory(const char *directoryName, uint8_t levels);
  void deleteFile(fs::FS &fs, const char *filePath);
  void fileList(const char *startPath = "/");
};
#endif