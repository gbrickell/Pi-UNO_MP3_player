#!/usr/bin/python

# *******************************************************************************************************
# A Python program to send 'instructions' to the DFPlayer - a mini MP3 Player controlled by an Arduino
# *******************************************************************************************************

# command to run: python3 /home/pi/Arduino_connect/DFPlayer_ArduinoI2c_01.py
#  adjust this command depending upon where the code is stored on the RPi

#  This test system shows how all the DFPlayer functions can be controlled from a 
#  Python program running on a Raspberry Pi and using SMBUS/I2C as the connection method

#  This Raspberry Pi 'side' of the system provides a simple CLI user interface to capture an instruction
#   and to then send it to the Arduino on a I2C/SMBUS link
#  Only a limited number of commands have so far been coded just as a prof of concept and a  more extensive
#   user interface, perhaps using a Flask web server could be developed at some time
 
# The Arduino code has so far only been tested on an Arduino Uno
# **************************************************************

# The RPi (master) communicates with an Arduino board (client) using SMBUS which is a subset of I2C
# method adapted from https://dzone.com/articles/arduino-and-raspberry-pi-working-together-part-2-now-with-i2
# this simple version sends/gets acknowledged a series of characters that can each 
#  be used to 'command' the Arduino to configure the DFPlayer module
#
# the RPi I2C interface needs to be enabled
# use sudo i2cdetect -y 1 to show the I2C address of the connected UNO device/client 
#  which should be 0x04 - and the UNO should be 'reset' if the 0x04 address is not shown
# communicates with the Arduino script RPiConnect.ino which sets the address as above
# RPI SDA connects to the Arduino Uno A4 pin
# RPI SCL connects to the Arduino Uno A5 pin
# RPI GND and Arduino GND are also cross connected

# also using the RPi 5V pin to power the Arduino ie connected to the Vin pin

# also N.B. timing can be an issue as the I2C baud rate on the Pi is usually quite high (system set)
# so the Arduino should perhaps be set to (say) 115200 and some delays added between read/writes (see later) 


# *********************************
#      I2C/SMBUS comms functions
# *********************************

def writeData(value, cmd):
    # value is a string of characters
    # cmd is the cmd byte hex value that is in front of the data block e.g. 0x00
    byteValue = StringToBytes(value)    
    bus.write_i2c_block_data(address,cmd,byteValue) #first byte e.g. 0=command byte.. just is.
    return -1


def StringToBytes(val):
    retVal = []
    #for c in val:
    for i in range( len(val) ):
        print ("character in string :", val[i])
        retVal.append(ord(val[i]))
    return retVal


# *******************
#    main code
# *******************

import RPi.GPIO as gpio
import smbus
import time
import sys
bus = smbus.SMBus(1)
address = 0x04

# set the various command characters that are potentially allowed
# the specific meaning of each command used is interpreted in the Arduino IDE code
characters = ['A',   
           'B',     
           'C',
           'D',
           'E',
           'F',
           'G',
           'H',
           'I',
           'J',
           'K',
           'L',
           'M',
           'N',
           'O',
           'P',
           'Q',
           'R',
           'S',
           'T',
           'U',
           'V',
           'W',
           'X',
           'Y',
           'Z',
           '0',
           '1',
           '2',
           '3',
           '4',
           '5',
           '6',
           '7',
           '8',
           '9',
           ' ']

print ("currently defined commands:")
print ("A - play next mp3")
print ("B - play previous mp3")
print ("C - pause the mp3")
print ("D - start the mp3 from the pause")
print ("E - Volume Up")
print ("F - Volume Down")
print ("")

def main():
    gpio.setmode(gpio.BCM)
    gpio.setwarnings(False)
    gpio.setup(17, gpio.OUT)     # GPIO pin used to set the LED on or off
    cmdbyte = 0x00
    input_command = "nothing"  # set an initial default
    cmdbyte = 1                # set the cmdbyte to 1 

    while True:
        # now request an updated command from the user 
        while input_command not in characters or len(input_command) != 1:
            input_command = input(" Enter a DFPlayer command - must be exactly 1 character and a valid command (CTRL C to stop? )")

        gpio.output(17, 0)  # set the RPi LED 'off' until the sent command is acknowledged
        print ("cmdbyte set to: ", cmdbyte)
        print ("command set to: ", input_command)
        writeData(input_command, cmdbyte)   # write a command to the Arduino
        print ("RPi send to Arduino  :", input_command )     # print the sent character
        time.sleep(0.5)  # small delay to wait for a response form the Arduino to avoid IO errors

        Arduino_response = bus.read_byte(address)   # read the byte sent back from the Arduino
        print ("Arduino answer to RPI:", Arduino_response )
        if Arduino_response != 1:
            print ("incorrect response from Arduino")
        else:
            print ("Arduino command accepted")
            gpio.output(17, 1)  # set the RPi LED 'on' to show that the sent command is acknowledged

        time.sleep(0.5)  # general delay in the overall cycle
        input_command = "nothing"  # reset to an initial default so a new command can be entered

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print ('Interrupted')
        gpio.cleanup()
        sys.exit(0)