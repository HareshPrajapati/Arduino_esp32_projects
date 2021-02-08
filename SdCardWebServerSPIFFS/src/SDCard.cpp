#include "SDCard.h"

void SDcard::printDirectory(const char *directoryName, uint8_t levels)
{
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
      DEBUG_SD_CARD_NL("Directory : %s", file.name());
      printDirectory(file.name(), levels - 1);
    } else {
      int bytes = file.size();
      String fsize = "";
      if ((bytes < 1024)) {
        fsize = String(bytes) + " B";
      } else if ((bytes < (1024 * 1024))) {
        fsize = String(bytes / 1024.0, 3) + " KB";
      } else if ((bytes < (1024 * 1024 * 1024))) {
        fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
      } else {
        fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
      }
      DEBUG_SD_CARD_NL("File : %s Size : %s", file.name(), fsize.c_str());
    }
    file = root.openNextFile();
  }
  file.close();
}

void SDcard::deleteFile(fs::FS &fs, const char *filePath) {
  DEBUG_SD_CARD_NL("Deleting file: %s", filePath);
  if (fs.remove(filePath)) {
    DEBUG_SD_CARD_NL("File deleted");
  } else {
    DEBUG_SD_CARD_NL("Delete failed");
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
