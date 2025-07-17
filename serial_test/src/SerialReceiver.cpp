// SerialReceiver.cpp

#include "SerialReceiver.h"
#include <string.h> 

extern brain Brain;

// creating buffer to store incoming data

static char serialBuffer[SERIAL_BUFFER_SIZE];
static int bufferIndex = 0;

static bool prevMessageReturned = false;

void serialReceiverInit() {
    Brain.Screen.clearScreen();
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.print("USB Serial Receiver");
    Brain.Screen.newLine();
    Brain.Screen.print("Waiting for data...");
    Brain.Screen.newLine();
    printf("Serial Receiver Initialized\n");
}

const char* processSerialData() {
    if (prevMessageReturned) {
        memset(serialBuffer, 0, sizeof(serialBuffer));
        bufferIndex = 0;
        prevMessageReturned = false;
    }
    // read from standard input and return value
    int c = getchar();

    // --- DEBUGGING PRINTS ---
    if (c == EOF) {
        // printf("getchar() returned EOF (no data)\n"); // Don't print this every time, it floods
    } else if (c == '\0') {
        // printf("getchar() returned NUL (0x00)\n"); // Also don't print every time
    } else {
        // Print character and its ASCII value if it's actual data
        printf("DEBUG: Char read: '%c' (ASCII: %d)\n", (char)c, c);
        // Optionally, print to screen (but console is better for fast output)
        Brain.Screen.setCursor(6,1);
        Brain.Screen.print("Read char: ");
        Brain.Screen.print((char)c);
        Brain.Screen.newLine();
    }
    // --- END DEBUGGING PRINTS ---

    if (c != EOF && c != '\0') {
        if (c == '\n' || c == '\r') {
            serialBuffer[bufferIndex] = '\0';

            Brain.Screen.setCursor(7,1);
            Brain.Screen.print("Instruction: ");
            Brain.Screen.clearLine();
            Brain.Screen.print(serialBuffer);
            Brain.Screen.newLine();

            prevMessageReturned = true;
            return serialBuffer;
        } else if (bufferIndex < SERIAL_BUFFER_SIZE - 1) {
            serialBuffer[bufferIndex++] = c;
        } else {
            Brain.Screen.setCursor(8,1);
            Brain.Screen.print("Buffer full. Char discarded");
            Brain.Screen.newLine();
        }
    }

    return nullptr;
}