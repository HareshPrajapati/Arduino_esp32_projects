
#include <HexParser.h>

void startUpdate(Stream &updateSource, size_t updateSize)
{
  if (Update.begin(updateSize)) {
    size_t written = Update.writeStream(updateSource);
    if (written == updateSize) {
      DEBUG_HEXPARSER_NL("Written : %s  successfully.", String(written));
    } else {
      DEBUG_HEXPARSER_NL("written only : %s / %s . Retry ?", String(written), String(updateSize));
    }
    if (Update.end()) {
      DEBUG_HEXPARSER_NL("OTA Flash Done!!!!!!!!!!");
      if (Update.isFinished()) {
        DEBUG_GUI_NL("Update Successfully completed . rebooting..");
      } else {
        DEBUG_HEXPARSER_NL("Update Not Finished ?? Something went wrong!!");
      }
    } else {
      DEBUG_HEXPARSER_NL("Error Occurred. Error #: %s ", String(Update.getError()));
    }
  } else {
    DEBUG_HEXPARSER_NL("Not enough space to begin OTA");
  }
}

void firmwareUpdateFromFS(fs::FS &fs) {
  File binUpdate = fs.open("/firmware.bin");
  if (binUpdate) {
    if (binUpdate.isDirectory()) {
      DEBUG_HEXPARSER_NL("Error, firmware.bin is not a file ");
      binUpdate.close();
      return;
    }
    size_t updateSize = binUpdate.size();
    if (updateSize > 0) {
      DEBUG_HEXPARSER_NL("try to start up update..");
      startUpdate(binUpdate, updateSize);
    } else {
      DEBUG_HEXPARSER_NL("Error, file is empty ");
    }
    binUpdate.close();
    fs.remove("/firmware.bin"); // when update  finished remove the binary from sd card to indicate end of the process
  } else {
    DEBUG_HEXPARSER_NL("Could not load update.bin from sd root");
  }
}

String cutcheckSum(String s) {
  int num;
  int index;
  char *New;
  for (index = 0; s[index] != '\0'; index++)
    ;
  // lenght of the new string
  num = index - 2 + 1;
  if (num < 1)
    return "";
  New = (char *)malloc(num * sizeof(char));
  for (index = 0; index < num - 2; index++)
    New[index] = s[index];
  New[index] = '\0';
  return New;
}

int hexToInt(char *temp) {
  int index = 0;
  for (;;) {
    char c = *temp;
    if (c >= '0' && c <= '9') {
      index *= 16;
      index += c - '0';
    } else if (c >= 'A' && c <= 'F') {
      index *= 16;
      index += (c - 'A') + 10;
    } else {
      break;
    }
    temp++;
  }
  return index;
}

String decToHex(int num) {
  String finalValue = "";
  char hexaDeciNumber[100];
  int index = 0;
  while (num != 0) {
    int temp = 0;
    temp = num % 16;
    // check if temp < 10
    if (temp < 10) {
      hexaDeciNumber[index] = (temp + 48);
      index++;
    } else {
      hexaDeciNumber[index] = (temp + 55);
      index++;
    }
    num = num / 16;
  }
  // printing hexadecimal number array in reverse order
  for (int j = index - 1; j >= 0; j--) {
    finalValue = finalValue + hexaDeciNumber[j];
  }
  return finalValue;
}

bool checkChecksumForHex(String readLine) {
  bool result = false;
  int len = readLine.length();
  String checkSum = readLine.substring((len - 3), len);
  String dataWithoutChecksum = cutcheckSum(readLine);
  int hexSum = 0;
  for (size_t index = 0; index < readLine.length(); index += 2) {
    String tempString = dataWithoutChecksum.substring(index, index + 2);
    int tempValue = hexToInt((char *)tempString.c_str());
    hexSum = hexSum + tempValue;
  }
  int tempSum = (65535UL - hexSum);
  int finalSum = (tempSum + 0x01);
  String finalResult = decToHex(finalSum);
  String finalCalculateChecksum = finalResult.substring((finalResult.length() - 2), finalResult.length()).c_str();
  if (strcpy((char *)finalCalculateChecksum.c_str(), (char *)checkSum.c_str())) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

bool checkChecksumForS19(String readLine) {
  bool result = false;
  int length = readLine.length();
  String checkSum = readLine.substring((length-3),length);
  String tempdataWithoutChecksum = cutcheckSum(readLine);
  String dataWithoutChecksum = tempdataWithoutChecksum.substring(2,(tempdataWithoutChecksum.length()));
  int hexSum = 0;
  for (size_t index = 0; index < readLine.length(); index += 2) {
    String tempString = dataWithoutChecksum.substring(index, index + 2);
    int tempValue = hexToInt((char *)tempString.c_str());
    hexSum = hexSum + tempValue;
  }
  int finalSum = (65535UL - hexSum);
  String finalResult = decToHex(finalSum);
  String finalCalculateChecksum = finalResult.substring((finalResult.length() - 2), finalResult.length()).c_str();
  if (strcpy((char *)finalCalculateChecksum.c_str(), (char *)checkSum.c_str())) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

// Write to the SD card
void writeFile(fs::FS &fs, const char * path, const char * data) {
  DEBUG_HEXPARSER("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    DEBUG_HEXPARSER_NL("Failed to open file for writing");
    return;
  }
  if(file.print(data)) {
    DEBUG_HEXPARSER("File written");
  } else {
    DEBUG_HEXPARSER("Write failed");
  }
  file.close();
}