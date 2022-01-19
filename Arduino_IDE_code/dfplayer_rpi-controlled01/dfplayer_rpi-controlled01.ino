/***************************************************
 DFPlayer - A Mini MP3 Player For Arduino
 ***************************************************
 This test system shows how all the DFPlayer functions can be controlled from a Python program 
 running on a Raspberry Pi and using SMBUS/I2C as the Arduino <--> RPi connection method

 This Arduino 'side' of the system builds upon the GNU Lesser General Public Licensed DFPlayer demo
 created in 2016-12-07 by [Angelo qiao](Angelo.qiao@dfrobot.com)
 
 This code has so far only been tested on an Arduino Uno
 ****************************************************/
 
// this code communicates with the RPi Python script DFPlayer_ArduinoI2c_01.py which assumes the Arduino address is 0x04 ie as 
// set below, and the Python script sends a series of single character commands that the Arduino uses to control the DFPlayer

// RPI connections
// ***************
// RPI SDA connects to the Arduino Uno A4 pin
// RPI SCL connects to the Arduino Uno A5 pin
// RPI GND and Arduino GND are also cross connected
// GPIO#17 used to control an 'indicator' LED on the RPi
// also using the RPi 5V pin to power the Arduino ie connected to the Uno Vin pin

// DFPlayer/Arduino connections
// ****************************
// DFPlayer VCC --> Uno 5V
// DFPlayer GND --> Uno GND
// DFPlayer RX  --> Uno #11
// DFPlayer TX  --> Uno #10
// speaker connections to GND and DFPlayer SPK_1 & SPK_2

// DFPlayer control includes
// *************************
#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// RPi smbus/I2C includes
// **********************
#include <Wire.h>

#define SLAVE_ADDRESS 0x04
#define LED  13    // use the Uno internal board LED as an 'indicator'

// DFPlayer set ups
// ****************
SoftwareSerial mySoftwareSerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

// RPi smbus/I2C set ups
// *********************
volatile boolean receiveFlag = false;
volatile char temp[32];
char received_command[32];
String command;

void setup()
{
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);

  Serial.println();
  Serial.println(F("RPi managed DFPlayer Mini Demo"));
  
  // DFPlayer set up
  // ***************
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  if (!myDFPlayer.begin(mySoftwareSerial)) {    // Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to start DFPlayer:"));
    Serial.println(F("1.Please recheck the DFPlayer connections!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  Serial.println(F("DFPlayer Mini online."));

  myDFPlayer.setTimeOut(500); //Set DFPlayer serial communication time out 500ms

  //----Set volume----
  myDFPlayer.volume(30);   //Set volume value (0~30).
  myDFPlayer.volumeUp();   //Volume Up
  myDFPlayer.volumeDown(); //Volume Down

  //----Set default EQ and list other options----
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
//  myDFPlayer.EQ(DFPLAYER_EQ_POP);
//  myDFPlayer.EQ(DFPLAYER_EQ_ROCK);
//  myDFPlayer.EQ(DFPLAYER_EQ_JAZZ);
//  myDFPlayer.EQ(DFPLAYER_EQ_CLASSIC);
//  myDFPlayer.EQ(DFPLAYER_EQ_BASS);

  //----Set default output device as SD card and list other options----
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
//  myDFPlayer.outputDevice(DFPLAYER_DEVICE_U_DISK);
//  myDFPlayer.outputDevice(DFPLAYER_DEVICE_AUX);
//  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SLEEP);
//  myDFPlayer.outputDevice(DFPLAYER_DEVICE_FLASH);

  //----various MP3 control options----
//  myDFPlayer.sleep();       //sleep
//  myDFPlayer.reset();       //Reset the module
//  myDFPlayer.enableDAC();   //Enable On-chip DAC
//  myDFPlayer.disableDAC();  //Disable On-chip DAC
//  myDFPlayer.outputSetting(true, 15); //output setting, enable the output and set the gain to 15


  // Just a listing of all the various DFPlayer 'play' command options
  // -----------------------------------------------------------------
  //myDFPlayer.next();  //Play next mp3
  //myDFPlayer.previous();  //Play previous mp3
  //myDFPlayer.play(1);  //Play the first mp3
  //myDFPlayer.loop(1);  //Loop the first mp3
  //myDFPlayer.pause();  //pause the mp3
  //myDFPlayer.start();  //start the mp3 from the pause
  //myDFPlayer.playFolder(15, 4);  //play specific mp3 in SD:/15/004.mp3; Folder Name(1~99); File Name(1~255)
  //myDFPlayer.enableLoopAll(); //loop all mp3 files.
  //myDFPlayer.disableLoopAll(); //stop loop all mp3 files.
  //myDFPlayer.playMp3Folder(4); //play specific mp3 in SD:/MP3/0004.mp3; File Name(0~65535)
  //myDFPlayer.advertise(3); //advertise specific mp3 in SD:/ADVERT/0003.mp3; File Name(0~65535)
  //myDFPlayer.stopAdvertise(); //stop advertise
  //myDFPlayer.playLargeFolder(2, 999); //play specific mp3 in SD:/02/004.mp3; Folder Name(1~10); File Name(1~1000)
  //myDFPlayer.loopFolder(5); //loop all mp3 files in folder SD:/05.
  //myDFPlayer.randomAll(); //Random play all the mp3.
  //myDFPlayer.enableLoop(); //enable loop.
  //myDFPlayer.disableLoop(); //disable loop.

  //----Read and display current DFPlayer 'state' information----
  Serial.println(myDFPlayer.readState());      //read and display general mp3 state
  Serial.println(myDFPlayer.readVolume());     //read and display current volume
  Serial.println(myDFPlayer.readEQ());         //read and display EQ setting
  Serial.println(myDFPlayer.readFileCounts()); //read and display all file counts in SD card
  Serial.println(myDFPlayer.readCurrentFileNumber());   //read and display current play file number
  Serial.println(myDFPlayer.readFileCountsInFolder(3)); //read and display fill counts in folder SD:/03


  // RPi comms set up
  // ****************
  pinMode(LED, OUTPUT);

  // initialize i2c as slave
  Wire.begin(SLAVE_ADDRESS);

  // define callbacks for i2c communication
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

  Serial.println(" LED  switched OFF as an initial start position");
  digitalWrite(LED, LOW);
  Serial.println("RPi to Arduino UNO links ready and waiting!");

}

void loop()
{
  if (receiveFlag == true) {
    Serial.print("received command: ");
    Serial.println(received_command);
    if (myDFPlayer.available()) {
      printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
    }
    receiveFlag = false;
    Serial.println(" - LED  switched OFF again to show that the Arduino is waiting for a new command");
    digitalWrite(LED, LOW);

    run_command(received_command[0]);
   
  }
}

void run_command(char type){
  Serial.println("Checking which command to execute ...");
  switch (type) {
    case 'A':
	  Serial.println("Executing command A: play next MP3");
      myDFPlayer.next();  //Play next mp3
      break;

    case 'B':
	  Serial.println("Executing command B: play previous MP3");
      myDFPlayer.previous();  //Play previous mp3
      break;

    case 'C':
	  Serial.println("Executing command C: pause MP3");
      myDFPlayer.pause();  //pause the mp3
      break;

    case 'D':
	  Serial.println("Executing command D: (re)start MP3");
      myDFPlayer.start();  //start the mp3 from the pause
      break;	  

    case 'E':
	  Serial.println("Executing command E: increase sound volume");
      myDFPlayer.volumeUp();   //Volume Up
      break;

    case 'F':
	  Serial.println("Executing command F: decrease sound volume");
      myDFPlayer.volumeDown(); //Volume Down
      break;

    default:
      break;
  }
}

void receiveData(int howMany) {
  Serial.println("receiveData happening");

  // initial just grab the first byte which is alays a 'cmd' character sent by the RPi
  int cmd=Wire.read();
  Serial.print("cmd character: ");
  Serial.println(cmd);

  // now grab the howMany bytes command into the temp array
  // and take a copy into received_command
  for (int i = 0; i < howMany; i++) {
    temp[i] = Wire.read();
    received_command[i] = temp[i];
    temp[i + 1] = '\0'; //add null after each char
    received_command[i +1] = temp[i + 1];
  }

  Serial.print("command string received from RPi: ");
  // the cmd byte should always be 1 so the LED should always be put 'on'
  if (cmd == 1) {
    Serial.println(" LED ON");
    digitalWrite(LED, HIGH);
  } else {
    Serial.println(" LED OFF");
    digitalWrite(LED, LOW);
  }

  receiveFlag = true;
 
}


void sendData() {
  Wire.write(1);
  Serial.println("1 sent back to RPi to acknowledge command receipt: ");

}


void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}
