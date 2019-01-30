#ifndef DMXREADER_H
#define DMXREADER_H

#include "configuration.h"

void initDmx(int update_delay, unsigned char slave_address, LedColor *input_dmx);

void stopDmx();

#endif
