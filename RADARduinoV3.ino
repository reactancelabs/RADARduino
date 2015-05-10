  /* SAR Radar Control Progeam for the unnamed Arduino Radar Kit
   REACTANCE LABS
   (c) Tony Long, 2012
   This code is Open Source, licensed under Creative Commons Attribution 3.0 Unported License
   see http://creativecommons.org/licenses/by/3.0 for details
   
   This code was designed to control the motion of a linear rail-type Synthetic Aperture Radar
   kit developed by Reactance Labs.  The carriage is driven by a stepper motor which is connected
   via an EasyDrive Stepper Motor Driver.  See info at http://www.schmalzhaus.com/EasyDriver/index.html
   
 */
   
#include "SPI.h"
unsigned int SyncPeriod = 2; //the period in microseconds of delay between DAC steps
                             
                       
                             
word outputValue = 0; // a word is a 16-bit number
byte data = 0; // and a byte is an 8-bit number


// Limit Switches
int LeftLimitSwitch = 7;  // Left Limit Switch is on pin 7
int RightLimitSwitch = 6; // Right Limit Switch is on pin 6

// Motor Control Lines
int MotorEnable = 5;      // Stepper Motor Driver Enable is on pin 5
int MotorDirection = 4;   // Stepper Motor Driver Direction is on pin 4
int MotorStep = 3;        // Stepper Motor Driver Step is on pin 3
int MotorSleep = 2;       // Stepper Motor Driver Sleep is on pin 2

char firmwareVersion[ ] = "Version 1.0";  // This is the version of this firmware
int StepPeriod = 3;                       // This value equals one half of the stepper motor pulse in msec
int CurrPos = 0;                          // Current Position (in steps relative to home position)
int RightLimit = 0;                       // Right Limit Indicator
int LeftLimit = 0;                        // Left Limit Indicator
int End = 1000000;                        // Position of rail end
String command = "";                      // a string to hold incoming commands
String value = "";                        // a string to hold incoming values
boolean stringComplete = false;  // whether the string is complete
char valueArray[8];
void setup()
{
  
  Serial.begin(115200);     // start serial port at 9600 bps and wait for port to open
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  command.reserve(20);
  value.reserve(20);
  
  pinMode(7, INPUT);   // Left Limit Switch is on pin 7
  pinMode(6, INPUT);   // Right Limit Switch is on pin 6
  pinMode(5, OUTPUT);  // Stepper Motor Driver Enable is on pin 5
  pinMode(4, OUTPUT);  // Stepper Motor Driver Direction is on pin 4
  pinMode(3, OUTPUT);  // Stepper Motor Driver Step is on pin 3
  pinMode(2, OUTPUT);  // Stepper Motor Driver Sleep is on pin 2
  
  pinMode(10, OUTPUT); //DAC CS pin
  pinMode(9, OUTPUT);  //SYNC pin
  SPI.begin(); // wake up the SPI bus.
  SPI.setBitOrder(MSBFIRST);
  digitalWrite(9, LOW); //Ensure SYNC signal is off
  
  PowerDownDriver();   // Clear the Motor Driver to keep it in sleep mode
  establishContact();  // send a byte to establish contact until receiver responds 
}

void loop()
{ 
  if (stringComplete) {  
    PowerUpDriver();
    if (command.equals("moveRight")) {    // This is the moveRight command 
      value.toCharArray(valueArray,7);    // Convert the input String value to a char array
      int a = atoi(valueArray);           // Convert the char array to an integer
      MotorClockwise(a);                  // Move the motor by the specified number of steps
      }
    if (command.equals("moveLeft")) {     // This is the moveLeft command
      value.toCharArray(valueArray,7);    // Convert the input String value to a char array
      int a = atoi(valueArray);           // Convert the char array to an integer
      MotorCounterClockwise(a);           // Move the motor by the specified number of steps
      }
      
    if (command.equals("findHome")) {     // Finds Home position
      FindHome();
      }
      
    if (command.equals("findEnd")) {      // Finds End position
      FindEnd();
      }
      
    if (command.equals("CurrPos"))  {     // Returns the current position
      Serial.println(CurrPos);
      }
      
    if (command.equals("version")) {      // Returns the current version of this firmware
      Serial.println(firmwareVersion);  
    }
    
    if (command.equals("gotoPos")) {      // Go To a specified absolute position on the rail
      value.toCharArray(valueArray,7);    // Converts the input String to a char array
      int a = atoi(valueArray);           // Converts the char array to an integer
      if (a < CurrPos) {                  // These two if statements take care of deciding which 
        MotorCounterClockwise(CurrPos-a); //  way to move the motor, and by how much.
      }
      if (a > CurrPos) {
        MotorClockwise(a-CurrPos);  
      }  
    }
    if (command.equals("modulatorOn")) {      // Turns the modulator on
      Modulator();
    }  
    if (command.equals("setVCO")) {      // set VCO to specified level
      value.toCharArray(valueArray,7);    // Convert the input String value to a char array
      int a = atoi(valueArray);           // Convert the char array to an integer
      setVCO(a);                  // Move the motor by the specified number of steps
    }
  
    if (command.equals("SAR")) {
      CaptureSAR();
    }
    
command = "";                              // Clears the current command since it has been executed
value = "";                                // Clears the current value since it has been used
stringComplete = false;                    // Resets the flag for complete string input

}

}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();     // get the new byte:
    if(isAlpha(inChar)){                   // add it to the command String if it is a letter
     command += inChar;
    }
    
    if(isDigit(inChar)) {                  // add it to the command String if it is a number
     value += inChar;
    }
    
    if (inChar == '%') {                   // if the incoming character is a newline, set a flag
      stringComplete = true;               // so the main loop can do something about it:
    } 

}
}

void establishContact() {
  while (Serial.available() <= 0) {
    Serial.println("Ready");   // send an initial string
    delay(300);
  }
}

void PowerDownDriver() {
  digitalWrite(MotorEnable, HIGH);   // Set Motor Driver Enable pin to high (this disables the outputs) 
  digitalWrite(MotorSleep, LOW);    // Set Motor Driver Sleep pin to low (this puts the part to sleep)
}

void PowerUpDriver() {
  digitalWrite(MotorEnable, LOW);    // Set Motor Driver Enable pin to low (this enaables the outputs)
  digitalWrite(MotorSleep, HIGH);    // Set Motor Driver Sleep pin to high (this wakes up the part)
  delay(5);                          // Pause for 5 msec to let the Motor Driver circuitry wake up
}

void MotorClockwise(int steps) {
  RightLimit = digitalRead(RightLimitSwitch);

    digitalWrite(MotorDirection, LOW);  // Set Motor Direction 
    while(steps > 0 && RightLimit == 0) {
      RightLimit = digitalRead(RightLimitSwitch);
      steps = --steps;                    // decrement the counter
      digitalWrite(MotorStep, LOW);       // Start pulse
      delay(StepPeriod);                  // Hold value for half the cycle
      digitalWrite(MotorStep, HIGH);      // Set to high value of pulse
      delay(StepPeriod);                  // Hold value for second half of the cycle
      CurrPos = ++CurrPos;                // Increment Current Position 
      RightLimit = digitalRead(RightLimitSwitch);
      } 
    digitalWrite(MotorStep, LOW);       // Return the motor pulse to zero
    Serial.print("CurrentPosition: ");   // Send Current Position to host
    Serial.println(CurrPos, DEC);        // Send Current Position to host     
}

void MotorCounterClockwise(int steps) {
  LeftLimit = digitalRead(LeftLimitSwitch);
    digitalWrite(MotorDirection, HIGH);    // Set Motor Direction 
    while(steps > 0 && LeftLimit == 0) {
      LeftLimit = digitalRead(LeftLimitSwitch);
      steps = --steps;                    // decrement the counter
      digitalWrite(MotorStep, LOW);       // Start pulse
      delay(StepPeriod);                  // Hold value for half the cycle
      digitalWrite(MotorStep, HIGH);      // Set to high value of pulse
      delay(StepPeriod);                  // Hold value for second half of the cycle
      CurrPos = --CurrPos;                // Decrement Current Position
      LeftLimit = digitalRead(LeftLimitSwitch);
  }
    digitalWrite(MotorStep, LOW);       // Return the motor pulse to zero
    Serial.print("CurrentPosition: ");  // Send Current Position to host
    Serial.println(CurrPos, DEC);       // Send Current Position to host
  
}

void FindHome() {
  LeftLimit = digitalRead(LeftLimitSwitch);
  while (LeftLimit == 0) {
    LeftLimit = digitalRead(LeftLimitSwitch);
    MotorCounterClockwise(1);  // move carriage to the left one step
    
  }
  CurrPos = 0;
  Serial.println("Done");
}

void FindEnd() {
  RightLimit = digitalRead(RightLimitSwitch);
  while (RightLimit == 0) {
    MotorClockwise(1); // move carriage to the right one step
    RightLimit = digitalRead(RightLimitSwitch);
  }
  End = CurrPos;
  Serial.print("End position is: ");
  Serial.println(CurrPos);
}

void Pulse(){
    digitalWrite(9, LOW); // SYNC Low
    for (int i=600; i<3000; i+=2){
        if(i==1800){
            digitalWrite(9, HIGH); //SYNC Low
        }
        outputValue = i;
        digitalWrite(10, LOW);
        data = highByte(outputValue);
        data = 0b00001111 & data; //clear configuration bits
        data = 0b00010000 | data; //set configuration bits
        SPI.transfer(data); //sends first byte of data word
        data = lowByte(outputValue);
        SPI.transfer(data); //sends second byte of data word
        digitalWrite(10, HIGH);
        delayMicroseconds(SyncPeriod);
    } 
}


void Modulator()
{
    Serial.print("Sawtooth Wave Transmitting...");

    while(1)
    {
        Pulse();
       
    }
}

void setVCO(int level){
  Serial.println("VCO set");

        digitalWrite(10, LOW);
        data = highByte(level);
        data = 0b00001111 & data; //clear configuration bits
        data = 0b00010000 | data; //set configuration bits
        SPI.transfer(data); //sends first byte of data word
        data = lowByte(level);
        SPI.transfer(data); //sends second byte of data word
        digitalWrite(10, HIGH);
}
  
  
void CaptureSAR(){
    Serial.println("Finding Home");
    FindHome();
    Serial.println("Home Found!");
    Serial.println("Capturing SAR Image Now");
    
    RightLimit = digitalRead(RightLimitSwitch);
    while (RightLimit == 0) {
        pinMode(9, OUTPUT);
        for (int j=0; j<50; j++){
            Pulse();
        }
        pinMode(9, INPUT);
        for (int x=0; x<77; x++) {    // Move the carriage 2 inches
        MotorClockwise(1); // move carriage to the right one step
        RightLimit = digitalRead(RightLimitSwitch);
        }
        delay(2000);   
      }
    
  
}  
