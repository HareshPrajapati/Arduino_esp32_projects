#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <analogWrite.h>
#include <PID_v1.h>

double Setpoint ; // will be the desired value
double Input; // photo sensor
double Output ; //LED
//PID parameters
double Kp=0, Ki=10, Kd=0; 
 
//create PID instance 
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);
 
void setup() {
  Serial.begin(921600);
  //Hardcode the brigdness value
  Setpoint =75;
  //Turn the PID on
  myPID.SetMode(AUTOMATIC);
  //Adjust PID values
  myPID.SetTunings(Kp, Ki, Kd);
}
 
void loop() {
  Input = map(analogRead(2), 0, 4095, 0, 255);  // photo senor is set on analog pin 5
  //PID calculation
  myPID.Compute();
  //Write the output as calculated by the PID function
  analogWrite(15,Output);
  //Send data by serial for plotting
  Serial.print(Input);
  Serial.print(" ");
  Serial.println(Output);
  Serial.print(" ");
  Serial.println(Setpoint);
}