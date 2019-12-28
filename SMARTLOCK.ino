//CPE123 Noon-3 Group 3 
//Nayana Tiwari, Stefan Patch, Comal Virdi, Samay Nathani
//Quarter Project - Smart Lock
//Code for Main Arduino Board
#include <CPutil.h>
#include <Servo.h>

const int doorStatusButtonPin = 12;
const int userButtonPin = 11;
const int buzzerPin = 10;
const int trigPin = 9;
const int echoPin = 8;

Servo doorServo;
Servo lockServo;


Button doorStatusButton(doorStatusButtonPin);
Button userButton(userButtonPin);

//enum statement for changing the LEDs
enum{STARTUP, UNLOCKED, LOCKED};

//Global Variables for RFID
int ValidRFIDTag[14] = {2,48,56,48,48,52,53,68,54,54,49,70,65,3};
int ScannedRFIDTag[14] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int RFIDDecision = -1;
int RFIDReadVariable = 0;

void setup()
{
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  
  doorServo.attach(7);
  lockServo.attach(6);

  Serial1.begin(9600);
  Serial2.begin(9600); //This is the RFID Scanner
  delay(500);
}

void loop()
{
  //control();
  closed();
  delay(1000);
  opened();
  delay(1000);
  
}

void control()
{
  if(checkRFID())
  {
    doorOpen();
    verification2();
  }
  else if(userButton.wasPushed())
  {
    doorOpen();
    verification1();
  }
}
 // verification 1 code
void verification1()
{
  static MSTimer timer;
  enum {START, YES_RFID, NO_RFID, NO_RFID_CHECK /*DOOR_CLOSE, MOTION_SENSOR, ,*/ };
  static int state = START;
  
  switch(state)
  {
    case START:
      if(verifyRFID())
      {
        state = YES_RFID;
      }
      else
      {
        state = NO_RFID;  
      }
    break;
      
    case YES_RFID:
      doorClose(true);
      state = START;
    break;
    
    case NO_RFID:
      timer.set(300000);
      doorClose(false);
      tone(buzzerPin, 1088);
      Serial.println("User has left WITHOUT KEY!");
      state = NO_RFID_CHECK;
    break;
    
    case NO_RFID_CHECK:
      if (verifyRFID() || timer.done())
      {
        noTone(buzzerPin);
        state = YES_RFID;
      }
    break;
  }
}

// verification 2 code
void verification2()
{
  static MSTimer timer;
  enum {START, CHECKBUTTON, DOORCLOSE};
  static int state = START;
 
  switch(state)
  {
    case START:
      timer.set(3000000);
      if(!timer.done())
      {      
        state = CHECKBUTTON;
      }
    break;

    case CHECKBUTTON:
      if (userButton.wasPushed())
      {
        doorClose(true);
      }
      if (timer.done())
      {
       timer.set(3000);
       tone(buzzerPin, 1088);
       state = DOORCLOSE;
      }
    break;

    case DOORCLOSE:
      if (timer.done())
      {
        noTone(buzzerPin);
        doorClose(true);
      }
    break; 
  }
}

void ultrasonicSensor(int passedLockFlag)
{
  if(!checkDoorway())
  {
    noTone(buzzerPin);
    doorClose(passedLockFlag);
  }
  else if(checkDoorway())
  {
    opened();
    tone(buzzerPin,1088);
    
  }
}

void doorClose(int lockFlag)
{
  closed();
  while(!doorStatusButton.wasPushed())
  {
    ultrasonicSensor(lockFlag);
  }
  if(lockFlag)
  {
    lock();
    changeLeds(LOCKED);
    ultrasonicSensor(lockFlag);
  }
}

void doorOpen()
{
  unlock();
  changeLeds(UNLOCKED);
  delay(250);
  opened();
}


int checkRFID()
{
  int returnValue = false;
  int i = 0;
  RFIDDecision = -1;

  if(Serial2.available() > 0)
  {
    delay(100);

    for(i = 0;i < 14;i++)
    {
      RFIDReadVariable = Serial2.read();
      ScannedRFIDTag[i] = RFIDReadVariable;
    }

    Serial2.flush();

    checkTag();
  }

  if(RFIDDecision > 0)
  {
    returnValue = true;
    RFIDDecision = -1;
  }
  else if(RFIDDecision == 0)
  {
    returnValue = false;
    RFIDDecision = -1;
  }
  
  return returnValue;
}

int verifyRFID()
{
  int returnValue = false;
  int i = 0;
  RFIDDecision = -1;
  
  static MSTimer timer;
  timer.set(60000);
  
  while(!timer.done() && RFIDDecision == -1)
  {
    if(Serial2.available() > 0)
    {
      delay(100);

      for(i = 0;i < 14;i++)
      {
        RFIDReadVariable = Serial2.read();
        ScannedRFIDTag[i] = RFIDReadVariable;
      }
      
      Serial2.flush();
  
      checkTag();
    }
  }

  if(RFIDDecision > 0)
  {
    returnValue = true;
    RFIDDecision = -1;
  }
  else if(RFIDDecision == 0)
  {
    returnValue = false;
    RFIDDecision = -1;
  }
  
  return returnValue;
}

void checkTag()
{
  //Note: -1 is No Read Attempt, 0 is Read but Not Validated, 1 is Valid
  RFIDDecision = 0;

  if(compareTag(ScannedRFIDTag,ValidRFIDTag) == true)
  {
    RFIDDecision++;
  }
}

int compareTag(int readDigit[14], int validDigit[14])
{
  int returnFlag = false;
  int digit = 0;
  int i = 0;

  for(i = 0; i < 14; i++)
  {
    if(readDigit[i] == validDigit[i])
    {
      digit++;
    }
  }

  if (digit == 14)
  {
    returnFlag = true;
  }

  return returnFlag;
}

//Code to communicate to the second board to change the LED States
void changeLeds(int newState)
{
  if(newState == STARTUP)
  {
    Serial1.print(STARTUP);
  }
  else if(newState == UNLOCKED)
  {
    Serial1.print(UNLOCKED);
  }
  else if(newState == LOCKED)
  {
    Serial1.print(LOCKED);
  }
}

//proximity sensor code
int checkDoorway()
{
  int value = 0;
  
  trigger();
  value = readEcho();
  if (value > 8 && value < 76)
  {
    return true;
  }
  else
  {
    return false;
  }
}

//supporting proximity sensor code not written by us but we organized it into separate functions
int readEcho()
{
 long duration = pulseIn(echoPin, HIGH);
 long inches = (duration/2) /74;
 Serial.print(inches);
 delay (250);
 return inches;
}

void trigger()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
}

// Servo lock and unlock code for lock servo + close and open for the door servo
void unlock()
{
  lockServo.write(90);
}

void lock()
{
  lockServo.write(0);
}
void closed()
{
  doorServo.write(0);
}
void opened()
{
  doorServo.write(90);
}
