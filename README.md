# VLC submarine communication

This is a project to build an end-to-end communication protocol, implemented between two [Arduinos](https://www.arduino.cc/), supported by micro-computers (mainly for data storage), to use a very simple and down-scalable (think [Pico](https://www.kickstarter.com/projects/melbel/pico-the-worlds-smallest-arduino-board) and [Pi Zero](https://www.raspberrypi.org/products/raspberry-pi-zero/)) setup to: 
1) Request data to be uploaded from the emitter side
2) Receive it using a blue diode and a visible-light photodiode.
3) Acknowledge the good transmission of packets

The purpose of this would be to have an underwater, battery-powered device that can log data from different sensors. Then, an ROV from a nearby boat could periodically approach the fixed device and instruct it to upload the data, so that the fixed device can empty its memory.

## Code and structure

There are four different project folders that each contain the code to run on one of the four involved devices.
**Processing Fixed Station** refers to the computer that holds the data initially responsible for breaking it down into chunks, and send them using a Serial communication to the first Arduino. The program was written in Processing since that was the most documented way to communicate with an arduino.
**Arduino Fixed Station** refers to the program to request packets from the computer, and to send them, along with some overhead, to the other Arduino.

