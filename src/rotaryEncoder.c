#include "rotaryEncoder.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <wiringPi.h>

//DT GPIO 36
//CLK GPIO 38
//SW GPIO 32
//vcc pin 1
//ground pin 20

unsigned char Last_RoB_Status;
unsigned char Current_RoB_Status;
unsigned char RoAPin;
unsigned char RoBPin;
unsigned char RoSPin;

void callDeal(void){
  if (digitalRead(RoAPin) == 0){
    Last_RoB_Status = digitalRead(RoBPin);
  } else {
    Current_RoB_Status = digitalRead(RoBPin);

    if ((Last_RoB_Status == 0) && (Current_RoB_Status == 1))
      encoderState++;
    if ((Last_RoB_Status == 1) && (Current_RoB_Status == 0))
      encoderState--;
  }
}

void callClear(void){
  encoderPush = 1;
}

void initEncoder(int DTpin, int CLKpin, int SWpin){
  if(wiringPiSetup() < 0){
    fprintf(stderr, "Unable to setup wiringPi:%s\n",strerror(errno));
    return;
  }

  RoAPin = DTpin;
  RoBPin = CLKpin;
  RoSPin = SWpin;

  pinMode(RoAPin, INPUT);
  pinMode(RoBPin, INPUT);
  pinMode(RoSPin, INPUT);

  pullUpDnControl(RoSPin, PUD_UP);

	encoderPush = 0;
	encoderState = 0;

  wiringPiISR(RoSPin, INT_EDGE_FALLING, callClear);
  wiringPiISR(RoAPin, INT_EDGE_BOTH, callDeal);
}
