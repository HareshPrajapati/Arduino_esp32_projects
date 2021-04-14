#ifndef HexParser_h
#define HexParser_h

/***********************************  Includes *********************************************/
#include <Arduino.h>
#include <Update.h>
#include <SD.h>
#include <FS.h>
#include <debug.h>


/********************************* Function Declaration ***************************************/

void startUpdate(Stream &updateSource, size_t updateSize);
void firmwareUpdateFromFS(fs::FS &fs);
String cutcheckSum (String s);
int hexToInt(char *s);
String decToHex(int n);
bool checkChecksumForHex(String readLine);
bool checkChecksumForS19(String readLine);
void writeFile(fs::FS &fs, const char * path, const char * data);

#endif