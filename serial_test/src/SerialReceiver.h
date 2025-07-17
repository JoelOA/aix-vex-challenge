#ifndef SERIAL_RECEIVER_H
#define SERIAL_RECEIVER_H

#include "vex.h"

using namespace vex;

// Define the maximum size for the serial buffer
const int SERIAL_BUFFER_SIZE = 256;

// Function to initialize the serial receiver
void serialReceiverInit();

const char* processSerialData();

#endif //SERIAL_RECEIVER_H