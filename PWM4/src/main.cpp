
#include <Arduino.h>
byte channelPins[] = {16, 17, 18, 19};
byte howManyChannels = sizeof(channelPins) / sizeof(channelPins[0]);

//timing
unsigned long previousMicros;
int highTime = 250; //micro seconds

unsigned long currentMicros;
void doChannels()
{
    static byte thisChannel = -1;

    if (currentMicros - previousMicros >= (unsigned long)highTime)
    {
        previousMicros = currentMicros;
        digitalWrite(channelPins[thisChannel], LOW);
        thisChannel++;
        if (thisChannel == howManyChannels)
            thisChannel = 0;
        //turn on "this" channel
        digitalWrite(channelPins[thisChannel], HIGH);
    }
} //doChannels()

void setup()
{
    Serial.begin(115200);
    Serial.println("250us PWM delay()-less");
    pinMode(16, OUTPUT);
    pinMode(17, OUTPUT);
    pinMode(18, OUTPUT);
    pinMode(19, OUTPUT);
} //setup

void loop()
{
    currentMicros = micros();
    doChannels();

} //loop
