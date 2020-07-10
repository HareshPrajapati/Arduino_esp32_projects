#include <Arduino.h>

const int buttonPin = 4;

int buttonState = 0;     // current state of the button
int lastButtonState = 0; // previous state of the button
int startPressed = 0;    // the moment the button was pressed
int endPressed = 0;      // the moment the button was released
int holdTime = 0;        // how long the button was hold
int idleTime = 0;        // how long the button was idle
int ledPin = 12;
unsigned long previousMillis = 0; // will store last time LED was updated
int ledState = LOW;               // ledState used to set the LED
void updateState();
void setup()
{
  pinMode(buttonPin, INPUT_PULLUP); // initialize the button pin as a input
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200); // initialize serial communication
}

void loop()
{
  buttonState = digitalRead(buttonPin); // read the button input

  if (buttonState != lastButtonState)
  { // button state changed
    updateState();
  }
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= holdTime)
  {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW)
    {
      ledState = HIGH;
    }
    else
    {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
  }

  lastButtonState = buttonState; // save state for next loop
}

void updateState()
{
  // the button has been just pressed
  if (buttonState == LOW)
  {
    startPressed = millis();
    idleTime = startPressed - endPressed;
  }
  else
  {
    endPressed = millis();
    holdTime = endPressed - startPressed;
    unsigned long dellay = holdTime;
    Serial.println("holdtime :");
    Serial.println(holdTime);
  }
}
