#include <X113647Stepper.h>

//STATUS CODES -----------------------------------------------
#define INTRUDER_DETECTED 0xff
#define OK 0xf0
#define MALFUNCTION 0x0f
#define SOMEONE_AT_THE_DOOR 0xaa

#define LOCK 0x00
#define UNLOCK 0x11

//PIN LAYOUT -------------------------------------------------
#define IR_SENSOR_PIN A1

#define ULTRASONIC_TRIG 9
#define ULTRASONIC_ECHO 10

#define IR_BYPASS A2
#define ULTRASONIC_BYPASS A3

//FUNC DEFINITIONS -------------------------------------------
bool serialAvailable();
void sendSerial(byte code);
byte readSerial();

void lockDoor();
void unlockDoor();
bool IRDetector();
bool isSomeoneAtTheDoor();

int distance;
long duration;

X113647Stepper motor(2048, 4, 5, 6, 7);

/*
  while (Serial.available() == 0){}

  int incomingByte = Serial.read();
  Serial.write(incomingByte);
*/

void setup() {
  motor.setSpeed(150);

  Serial.begin(9600);
  pinMode(IR_SENSOR_PIN, INPUT);
  
  pinMode(ULTRASONIC_ECHO, INPUT);
  pinMode(ULTRASONIC_TRIG, OUTPUT);

  pinMode(IR_BYPASS, INPUT);
  pinMode(ULTRASONIC_BYPASS, INPUT);
  

}

void loop() {
  while(serialAvailable() == false){
    if (IRDetector() && digitalRead(IR_BYPASS) == LOW){sendSerial(INTRUDER_DETECTED); delay(1000);return;}
    if (isSomeoneAtTheDoor() && digitalRead(ULTRASONIC_BYPASS) == LOW){sendSerial(SOMEONE_AT_THE_DOOR); delay(1000);return;}
    if (IRDetector() == false && IRDetector() == false){sendSerial(OK); delay(1000);return;}
    else {sendSerial(OK); delay(1000);return;}
  }

  switch(readSerial()){
    case 0x00:
      lockDoor();
      delay(10);
      sendSerial(OK);
      break;
    case 0x11:
      unlockDoor();
      delay(10);
      sendSerial(OK);
      break;
  }
}

bool serialAvailable(){
  return Serial.available();
}

void sendSerial(byte code){
  Serial.write(code);
}

byte readSerial(){
  return Serial.read();
}

void lockDoor(){
  motor.step(1024);
}

void unlockDoor(){
  motor.step(-1024);
}

bool IRDetector(){
  return !digitalRead(IR_SENSOR_PIN) == HIGH;
}


bool isSomeoneAtTheDoor(){
  digitalWrite(ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  
  digitalWrite(ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  
  digitalWrite(ULTRASONIC_TRIG, LOW);
  
  duration = pulseIn(ULTRASONIC_ECHO, HIGH);
  distance = duration * 0.034 / 2;

  return distance < 1100;
}