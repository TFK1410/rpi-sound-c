#include "dmxReader.h"
#include <wiringPi.h>
#include <wiringPiI2C.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

int time_stop;
int fd;
char buf[4];
pthread_t dmx_thread;
LedColor *color;
bool stop_thread;

void *dmxLoop(){
  while(!stop_thread){
    int res_len = read(fd, buf, 4);
    if(res_len > 0 && buf[0] > 0){
      color->r = buf[1];
      color->g = buf[2];
      color->b = buf[3];
    }
    if(time_stop > 0)
      delay(time_stop);
  }
  close(fd);
  return NULL;
}

void initDmx(int update_delay, unsigned char slave_address, LedColor *input_dmx){
  time_stop = update_delay;
  fd = wiringPiI2CSetup(slave_address);
  color = input_dmx;
  stop_thread = false;

  pthread_create(&dmx_thread, NULL, dmxLoop, NULL);
}

void stopDmx(){
  stop_thread = true;
  pthread_join(dmx_thread, NULL);
}
