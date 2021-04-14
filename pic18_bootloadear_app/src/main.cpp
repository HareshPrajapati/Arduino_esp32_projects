#include <Arduino.h>
#include <SPIFFS.h>

uint8_t versionRead[9] = {0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};
uint8_t flashErase[9] = {0x0F, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};
uint8_t readFlash[9] = {0x0F, 0x01, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x04};
uint8_t runApp[9] = {0x0F, 0x08, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x04};
uint8_t wrt[8] = {0x0F, 0x02, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40};

uint32_t startingAddress = 0x4000, finalAddress = 0, lastAddress = 0;
int count = 0;
long int tempByte = 0;
char *pEnd;
uint32_t start = 0, stop = 128, stopHex = 2, inc = 1;
String finalData = "", addressOneData = "";
File file;
String NAME_OF_FILE = "/500blink.hex";
void fileWriter(uint32_t tempAddress, String packet);
uint16_t FLASH_crc16(uint8_t *data_p, uint8_t length);
bool inturruptFile = false;

void setup()
{
  delay(1000);
  Serial.begin(9600UL);
  Serial2.begin(9600UL);
  if (!SPIFFS.begin())
  {
    Serial.println("SPIFFS failed for open");
  }
  else
  {
    Serial.println("Successfully open SPIFFS");
  }
  file = SPIFFS.open(NAME_OF_FILE, "r");

  bool startAddressTrue = true, secondAddressTrue = true;
  uint32_t firstAddress = 0, secoundAddress = 0;

  while (file.available())
  {
    String readLine = file.readStringUntil('\n');
    String Address = readLine.substring(3, 7);
    unsigned int tempAddress = strtol((char *)Address.c_str(), &pEnd, 16);
    if (tempAddress == startingAddress)
    {
      if (startAddressTrue)
      {
        firstAddress = tempAddress;
        Serial.printf("firstAddress = 0x%X \r\n", firstAddress);
        String recordType = readLine.substring(7, 9);
        String neededRecordType = "00";
        String tempData = readLine.substring(9);
        String data = tempData.substring(0, (tempData.length() - 3));
        addressOneData = data;
        startAddressTrue = false;
      }
    }
    if (tempAddress > firstAddress && tempAddress < (firstAddress + 64))
    {
      if (secondAddressTrue)
      {
        secoundAddress = tempAddress;
        Serial.printf("secound Address = 0x%X \r\n", secoundAddress);
        secondAddressTrue = false;
        inturruptFile = true;
      }
    }
  }
  file.close();
  if (inturruptFile)
  { 
    startAddressTrue = true, secondAddressTrue = true;
    firstAddress = 0, secoundAddress = 0;

    for (size_t i = 0; i < 9; i++)
    {
      Serial2.write(versionRead[i]);
      delay(5);
    }
    while (Serial2.read() != 0xEE)
      ;
    for (size_t i = 0; i < 9; i++)
    {
      Serial2.write(flashErase[i]);
      delay(5);
    }
    while (Serial2.read() != 0xEE);
    file = SPIFFS.open(NAME_OF_FILE, "r");
    while (file.available())
    {
      String readLine = file.readStringUntil('\n');
      String Address = readLine.substring(3, 7);
      unsigned int tempAddress = strtol((char *)Address.c_str(), &pEnd, 16);
      if (tempAddress == startingAddress)
      {
        if (startAddressTrue)
        {
          firstAddress = tempAddress;
          Serial.printf("firstAddress = 0x%X \r\n", firstAddress);
          String recordType = readLine.substring(7, 9);
          String neededRecordType = "00";
          String tempData = readLine.substring(9);
          String data = tempData.substring(0, (tempData.length() - 3));
          addressOneData = data;
          startAddressTrue = false;
        }
      }
      if (tempAddress > firstAddress && tempAddress < (firstAddress + 64))
      {
        if (secondAddressTrue)
        {
          secoundAddress = tempAddress;
          Serial.printf("secound Address = 0x%X \r\n", secoundAddress);
          secondAddressTrue = false;
        }
      }

      if (tempAddress > firstAddress)
      {
        String recordType = readLine.substring(7, 9);
        String neededRecordType = "00";
        String tempData = readLine.substring(9);
        String data = tempData.substring(0, (tempData.length() - 3));
        finalData += data;
      }
    }
    file.close();
    if (secoundAddress < (firstAddress + 64))
    {
      for (size_t i = 0; i < (secoundAddress - firstAddress); i++)
      {
        addressOneData += "F";
      }
    }
    addressOneData += finalData;
    uint16_t pp = ((addressOneData.length() / 128) + 1);
    uint16_t neededLen = (128 * pp);
    if (addressOneData.length() < neededLen)
    {
      uint16_t ad = (neededLen - addressOneData.length());
      for (size_t i = 0; i < ad; i++)
      {
        addressOneData += "F";
      }
    }
    uint32_t STARTadd = 0x4000;
    for (size_t i = 0; i < (pp); i++)
    {
      fileWriter(STARTadd, addressOneData.substring(start, stop));
      STARTadd += 64;
      start += 128;
      stop += 128;
      Serial.printf("\r\n ############################## writed ##################################\r\n");
    }
    start = 0;
    stop = 0;

    for (size_t i = 0; i < 9; i++)
    {
      Serial2.write(readFlash[i]);
      delay(5);
    }
    while (Serial2.read() != 0xEE)
      ;
    for (size_t i = 0; i < 9; i++)
    {
      Serial2.write(runApp[i]);
      delay(5);
    }
  } else {
    file.close();
    if (!SPIFFS.begin())
    {
      Serial.println("SPIFFS failed for open");
    }
    else
    {
      Serial.println("Successfully open SPIFFS");
    }
    for (size_t i = 0; i < 9; i++)
    {
      Serial2.write(versionRead[i]);
      delay(5);
    }
    while (Serial2.read() != 0xEE);
    for (size_t i = 0; i < 9; i++)
    {
      Serial2.write(flashErase[i]);
      delay(5);
    }
    while (Serial2.read() != 0xEE);
    file = SPIFFS.open(NAME_OF_FILE, "r");
    while (file.available())
    {
      String readLine = file.readStringUntil('\n');
      String Address = readLine.substring(3, 7);
      unsigned int tempAddress = strtol((char *)Address.c_str(), &pEnd, 16);
      String recordType = readLine.substring(7, 9);
      String neededRecordType = "00";
      if (tempAddress == startingAddress && recordType == neededRecordType)
      {
        Serial2.write(0x0F);
        delay(5);
        Serial2.write(0x02);
        delay(5);
        unsigned char a, b, c, d;
        a = (tempAddress & 0xFF);         //extract first byte
        b = ((tempAddress >> 8) & 0xFF);  //extract second byte
        c = ((tempAddress >> 16) & 0xFF); //extract third byte
        d = ((tempAddress >> 24) & 0xFF); //extract fourth byte
        Serial2.write(a);
        delay(5);
        Serial2.write(b);
        delay(5);
        Serial2.write(c);
        delay(5);
        Serial2.write(d);
        delay(5);
        Serial2.write(0x00);
        delay(5);
        String tempData = readLine.substring(9);
        String data = tempData.substring(0, (tempData.length() - 3));
        if (data.length() < 64)
        {
          uint32_t newLen = (64 + (data.length() / 2));
          for (size_t i = data.length(); i < newLen; i++)
          {
            data += "FF";
          }
        }
        delay(5);
        Serial2.write(0x40);
        delay(5);
        uint8_t temDataBuff[64] = {};
        memset(temDataBuff, 0, sizeof(temDataBuff));
        int p = 0;
        for (size_t index = 0; index < data.length(); index += 2)
        {
          String oneHex = data.substring(index, stopHex);
          tempByte = strtol((char *)oneHex.c_str(), &pEnd, 16);
          temDataBuff[p] = tempByte;
          p++;
          stopHex += 2;
        }
        stopHex = 2;
        for (size_t i = 0; i < 64; i++)
        {
          Serial2.write(temDataBuff[i]);
          delay(5);
        }
        uint16_t cr = FLASH_crc16(temDataBuff, sizeof(temDataBuff));
        Serial.printf("\r\ncrc = 0x%02X \r\n", cr);
        uint16_t crcLSB = 0, crcMSB = 0;
        crcLSB = (cr & 0xFF);
        crcMSB = ((cr >> 8) & 0xFF);
        delay(5);
        Serial.printf("crcLSB = 0x%02X \r\n", crcLSB);
        Serial2.write(crcLSB);
        delay(5);
        Serial.printf("crcMSB = 0x%02X \r\n", crcMSB);
        Serial2.write(crcMSB);
        delay(5);
        Serial2.write(0x04);
        delay(5);
        while (Serial2.read() != 0xEE);
      } else if (tempAddress > 0 && tempAddress != startingAddress) {
        if (recordType == neededRecordType)
        {
          uint32_t tempararyAddress = tempAddress & 0xFFC0;
          count++;
          if (count == 1)
          {
            finalAddress = tempararyAddress;
            String tempData = readLine.substring(9);
            String data = tempData.substring(0, (tempData.length() - 3));
            uint32_t unUsedLen = (tempAddress - tempararyAddress);
            String tempDataArray = "";
            for (size_t i = 0; i < unUsedLen; i++)
            {
              tempDataArray += "FF";
            }
            finalData = (tempDataArray + data);
          }
          else
          {
            lastAddress = tempararyAddress;
            String tempData = readLine.substring(9);
            String data = tempData.substring(0, (tempData.length() - 3));
            finalData += data;
          }
        }
      }
    }
    file.close();
    delay(5);
    count = (finalData.length() / 128);
    Serial.printf("count = %d \r\n", count);
    uint16_t startPoint = 0, stopPoint = 128;
    for (size_t i = 0; i < count; i++)
    {
      String gg = finalData.substring(startPoint, stopPoint);
      fileWriter(finalAddress, gg);
      finalAddress +=64;
      startPoint += 128;
      stopPoint += 128;
    }
    finalAddress = 0;
    for (size_t i = 0; i < 9; i++)
    {
      Serial2.write(readFlash[i]);
      delay(5);
    }
    while (Serial2.read() != 0xEE);
    for (size_t i = 0; i < 9; i++)
    {
      Serial2.write(runApp[i]);
      delay(5);
    }
  }
}


void loop()
{
}




void fileWriter(uint32_t tempAddress, String packet)
{
  Serial2.write(0x0F);
  delay(5);
  Serial2.write(0x02);
  delay(5);
  unsigned char a = 0, b = 0, c = 0, d = 0;
  a = (tempAddress & 0xFF);         //extract first byte
  b = ((tempAddress >> 8) & 0xFF);  //extract second byte
  c = ((tempAddress >> 16) & 0xFF); //extract third byte
  d = ((tempAddress >> 24) & 0xFF); //extract fourth byte
  Serial2.write(a);
  delay(5);
  Serial2.write(b);
  delay(5);
  Serial2.write(c);
  delay(5);
  Serial2.write(d);
  delay(5);
  Serial2.write(0x00);
  delay(5);
  Serial2.write(0x40);
  delay(5);
  uint8_t temDataBuff[64] = {};
  memset(temDataBuff, 0, sizeof(temDataBuff));
  int p = 0;
  for (size_t i = 0; i < packet.length(); i += 2)
  {
    String oneHex = packet.substring(i, stopHex);
    tempByte = strtol((char *)oneHex.c_str(), &pEnd, 16);
    temDataBuff[p] = tempByte;
    p++;
    stopHex += 2;
  }
  stopHex = 2;
  for (size_t i = 0; i < 64; i++)
  {
    Serial2.write(temDataBuff[i]);
    // Serial.printf("0x%X  ",temDataBuff[i]);
    delay(5);
  }
  uint16_t cr = FLASH_crc16(temDataBuff, sizeof(temDataBuff));
  Serial.printf("\r\ncrc = 0x%02X \r\n", cr);
  uint16_t crcLSB = 0, crcMSB = 0;
  crcLSB = (cr & 0xFF);
  crcMSB = ((cr >> 8) & 0xFF);
  delay(5);
  // Serial.printf("crcLSB = 0x%02X \r\n", crcLSB);
  Serial2.write(crcLSB);
  delay(5);
  // Serial.printf("crcMSB = 0x%02X \r\n", crcMSB);
  Serial2.write(crcMSB);
  delay(5);
  Serial2.write(0x04);
  delay(5);
  while (Serial2.read() != 0xEE)
    ;
}

uint16_t FLASH_crc16(uint8_t *data_p, uint8_t length)
{
  uint8_t x;
  /* Initial Polynomial */
  uint16_t crc = 0xFFFF;
  while (length--)
  {
    x = crc >> 8 ^ *data_p++;
    x ^= x >> 4;
    crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
  }
  return crc;
}