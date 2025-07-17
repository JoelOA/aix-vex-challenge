/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       Joel Osei-Asamoah                                         */
/*    Created:      6/5/2025, 9:34:21 AM                                      */
/*    Description:  V5 project                                                */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#include "SerialReceiver.h"
#include <string.h>
#include <queue>

// A global instance of vex::brain
brain Brain;

serial_link seriallink_worker_1 = serial_link(PORT6, "link_worker_1", linkType::manager);
serial_link seriallink_worker_2 = serial_link(PORT7, "link_worker_2", linkType::manager);

// thread-safe data structure for communication between threads
struct Message {
    char data[SERIAL_BUFFER_SIZE];
    uint8_t targetRadio;
    uint32_t timestamp;
};

// Message queues for thread communication
std::queue<Message> serialToRadioQueue;
std::queue<Message> radioToSerialQueue;

// mutex for thread-safe operation (VEX v5 uses basic synchronization)
bool serialToRadioQueueLock = false;
bool radioToSerialQueueLock = false;

//buffers
char mainReceivedBuffer[SERIAL_BUFFER_SIZE];
uint8_t serialReceiveBuffer[128];

uint8_t radioReceiveBuffer1[128];
uint8_t radioReceiveBuffer2[128];

bool threadsRunning = true;
uint32_t lastRadio1Contact = 0;
uint32_t lastRadio2Contact = 0;
const uint32_t RADIO_TIMEOUT_MS = 1000;

// radio addressing system
const uint8_t RADIO_1_ID = 1;
const uint8_t RADIO_2_ID = 2;

char RADIO_1_TEAM[64] = "NOT SET";
char RADIO_2_TEAM[64] = "NOT SET";

bool DEBUG = true;

/**
 * @brief This function gets a message, from the serial connection 
 * and thenpushes it to the "Serial to Radio Queue"
 * 
 * @param msg: The message that is to be sent to one of the robots
 * @return None
 */
void pushToSerialToRadioQueue(const Message& msg) {

    while(serialToRadioQueueLock) {
        wait(1, msec);
    }
    serialToRadioQueueLock = true;
    serialToRadioQueue.push(msg);
    serialToRadioQueueLock = false;
}

/**
 * @brief This function takes the most recent message on the SerialToRadioQueue
 * and then returns it to be sent to the appropriate radio
 * 
 * @param msg: message to be sent to radio
 * @return hasMessage: boolean representing whether the SerialToRadioQueue is full
 */
bool popFromSerialToRadioQueue(Message& msg) {
    while(serialToRadioQueueLock) {
        wait(1, msec);
    }
    serialToRadioQueueLock = true;
    bool hasMessage = !serialToRadioQueue.empty();
    if (hasMessage) {
        msg = serialToRadioQueue.front();
        serialToRadioQueue.pop();
    }
    serialToRadioQueueLock = false;
    return hasMessage;
}

/**
 * @brief Thread function to handle incoming serial data and route it to the appropriate radio.
 *
 * This function continuously checks for new serial data, parses the message to extract the team name and message content,
 * determines the appropriate target radio based on the team name, and pushes the message into the appropriate queue.
 * It also prints debug information to the Brain screen if debugging is enabled.
 *
 * @note This function should run in its own thread as it contains a while loop dependent on `threadsRunning`.
 *
 * @param None
 * @return void
 */
void serialThread() {
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.print("Serial Thread Started");
    Brain.Screen.newLine();

    while(threadsRunning) {
        // check for incoming serial data
        const char* receivedData = processSerialData();

        if (receivedData != nullptr) {
            // Parse the received data to determine target radio
            Message msg;
            msg.timestamp = Brain.timer(msec);

            // convert received info into a string
            char tempBuffer[SERIAL_BUFFER_SIZE];
            strncpy(tempBuffer, receivedData, sizeof(tempBuffer) - 1);
            tempBuffer[sizeof(tempBuffer) - 1] = '\0';

            // look for colon seperator to separate team name and data
            char* colonPos = strchr(tempBuffer, ':');

            if (colonPos != nullptr) {
                *colonPos = '\0';
                char* teamName = tempBuffer;
                char* messageData = colonPos + 1;

                if (DEBUG == true) {
                    // Debug: Print the exact strings being compared
                    Brain.Screen.setCursor(5, 1);
                    Brain.Screen.clearLine();
                    Brain.Screen.print("TeamName: '");
                    Brain.Screen.print(teamName);
                    Brain.Screen.print("' Len:");
                    Brain.Screen.print(strlen(teamName));
                    
                    Brain.Screen.setCursor(6, 1);
                    Brain.Screen.clearLine();
                    Brain.Screen.print("Radio1Team: '");
                    Brain.Screen.print(RADIO_1_TEAM);
                    Brain.Screen.print("' Len:");
                    Brain.Screen.print(strlen(RADIO_1_TEAM));
                    
                    Brain.Screen.setCursor(7, 1);
                    Brain.Screen.clearLine();
                    Brain.Screen.print("Radio2Team: '");
                    Brain.Screen.print(RADIO_2_TEAM);
                    Brain.Screen.print("' Len:");
                    Brain.Screen.print(strlen(RADIO_2_TEAM));

                    // Debug: Print character codes to see hidden characters
                    Brain.Screen.setCursor(11, 1);
                    Brain.Screen.clearLine();
                    Brain.Screen.print("TeamName bytes: ");
                    for(int i = 0; i < strlen(teamName) && i < 10; i++) {
                        Brain.Screen.print((int)teamName[i]);
                        Brain.Screen.print(",");
                    }

                    // Debug: Print comparison results
                    int cmp1 = strcmp(teamName, RADIO_1_TEAM);
                    int cmp2 = strcmp(teamName, RADIO_2_TEAM);
                    
                    Brain.Screen.setCursor(10, 1);
                    Brain.Screen.clearLine();
                    Brain.Screen.print("Cmp1: ");
                    Brain.Screen.print(cmp1);
                    Brain.Screen.print(" Cmp2: ");
                    Brain.Screen.print(cmp2);
                }

                // check which team this message is for
                if (strcmp(teamName, RADIO_1_TEAM) == 0) {
                    msg.targetRadio = RADIO_1_ID;
                    if (DEBUG) {
                        Brain.Screen.setCursor(17, 1);
                        Brain.Screen.clearLine();
                        Brain.Screen.print("Match: RADIO_1");
                    }
                } else if (strcmp(teamName, RADIO_2_TEAM) == 0) {
                    msg.targetRadio = RADIO_2_ID;
                    if (DEBUG) {
                        Brain.Screen.setCursor(17, 1);
                        Brain.Screen.clearLine();
                        Brain.Screen.print("Match: RADIO_2");
                    }
                }
                strncpy(msg.data, messageData, sizeof(msg.data) - 1);
                msg.data[sizeof(msg.data) - 1] = '\0';
            } 

            pushToSerialToRadioQueue(msg);
            
            Brain.Screen.setCursor(2, 1);
            Brain.Screen.clearLine();
            Brain.Screen.print("Serial RX: ");
            Brain.Screen.print(msg.data);
            Brain.Screen.print("---> Radio ");
            Brain.Screen.print(msg.targetRadio);
        }

        wait(5, msec);
    }
}

/**
 * @brief Thread function responsible for handling communication between serial and radio modules.
 * 
 * This thread continuously runs while `threadsRunning` is true. It performs two main tasks:
 * 1. Sends messages from the serial-to-radio queue to the appropriate radio (or both).
 * 2. Listens for and processes incoming messages from both radios, updating their last contact time 
 *    and extracting associated team names. It also provides real-time feedback on connection status.
 * 
 * @note This function should be run in a separate thread. It assumes the global state including
 *       serial queues, radio links, and debug flag.
 * 
 * @param none
 * @return void
 */
void radioThread() {
    Brain.Screen.setCursor(4, 1);
    Brain.Screen.print("Radio Thread Started");
    Brain.Screen.newLine();

    while(threadsRunning) {
        uint32_t currentTime = Brain.timer(msec);

        Message outMsg;
        if (popFromSerialToRadioQueue(outMsg)) {
            size_t dataLength = strlen(outMsg.data);
            // Prefix message with radio ID for identification
            char prefixedMsg[SERIAL_BUFFER_SIZE];

            if (outMsg.targetRadio == RADIO_1_ID) {
                snprintf(prefixedMsg, sizeof(prefixedMsg), "%s", outMsg.data);
                seriallink_worker_1.send((uint8_t*) prefixedMsg, strlen(prefixedMsg));
                lastRadio1Contact = currentTime;
            } else if (outMsg.targetRadio == RADIO_2_ID) {
                snprintf(prefixedMsg, sizeof(prefixedMsg), "%s", outMsg.data);
                seriallink_worker_2.send((uint8_t*) prefixedMsg, strlen(prefixedMsg));
                lastRadio2Contact = currentTime;
            } else {
                snprintf(prefixedMsg, sizeof(prefixedMsg), "%s", outMsg.data);
                seriallink_worker_1.send((uint8_t*) prefixedMsg, strlen(prefixedMsg));

                snprintf(prefixedMsg, sizeof(prefixedMsg), "%s", outMsg.data);
                seriallink_worker_2.send((uint8_t*) prefixedMsg, strlen(prefixedMsg));
                lastRadio1Contact = currentTime;
                lastRadio2Contact = currentTime;
            }

            Brain.Screen.setCursor(5, 1);
            Brain.Screen.clearLine();
            Brain.Screen.print("Radio TX to ");
            Brain.Screen.print(outMsg.targetRadio);
            Brain.Screen.print(": ");
            Brain.Screen.print(outMsg.data);
        }

        // check foor incoming radio messages from radio 1
        int buffer1 = seriallink_worker_1.receive(radioReceiveBuffer1, 1000);
        if (buffer1 != 0) {
            char* receivedStr = (char *) radioReceiveBuffer1;
            receivedStr[buffer1] = '\0';
            
            char cleanedStr[64];
            strncpy(cleanedStr, receivedStr, sizeof(cleanedStr) - 1);
            cleanedStr[sizeof(cleanedStr) - 1] = '\0';

            // remove trailing whitespace / newlines
            int len = strlen(cleanedStr);
            while (len > 0 && (cleanedStr[len - 1] == '\n' || cleanedStr[len - 1] == '\r' || 
                            cleanedStr[len - 1] == ' ' || cleanedStr[len - 1] == '\t')) {
                cleanedStr[--len] = '\0';
            }

            if (len > 0) {
                strncpy(RADIO_1_TEAM, cleanedStr, sizeof(RADIO_1_TEAM) - 1);
                RADIO_1_TEAM[sizeof(RADIO_1_TEAM) - 1] = '\0';
                
                if (DEBUG) {
                    Brain.Screen.setCursor(6, 1);
                    Brain.Screen.clearLine();
                    Brain.Screen.print("Radio 1 Team: ");
                    Brain.Screen.print(RADIO_1_TEAM);
                }
            }            
            lastRadio1Contact = currentTime;
        }

        // Check for incoming radio messages from radio 2
        int buffer2 = seriallink_worker_2.receive(radioReceiveBuffer2, 50);
        if (buffer2 != 0) {
            char* receivedStr = (char *) radioReceiveBuffer2;
            receivedStr[buffer2] = '\0';
            
            // Clean the received string
            char cleanedStr[64];
            strncpy(cleanedStr, receivedStr, sizeof(cleanedStr) - 1);
            cleanedStr[sizeof(cleanedStr) - 1] = '\0';
            
            // Remove any trailing whitespace/newlines
            int len = strlen(cleanedStr);
            while (len > 0 && (cleanedStr[len-1] == '\n' || cleanedStr[len-1] == '\r' || 
                            cleanedStr[len-1] == ' ' || cleanedStr[len-1] == '\t')) {
                cleanedStr[--len] = '\0';
            }
            
            // Only update if we got a valid team name
            if (len > 0) {
                strncpy(RADIO_2_TEAM, cleanedStr, sizeof(RADIO_2_TEAM) - 1);
                RADIO_2_TEAM[sizeof(RADIO_2_TEAM) - 1] = '\0';
                
                if (DEBUG) {
                    Brain.Screen.setCursor(7, 1);
                    Brain.Screen.clearLine();
                    Brain.Screen.print("Radio 2 Team: ");
                    Brain.Screen.print(RADIO_2_TEAM);
                }
            }
            
            lastRadio2Contact = currentTime;
        }
        // Display Connection Status
        Brain.Screen.setCursor(8, 1);
        Brain.Screen.clearLine();
        Brain.Screen.print("R1: ");
        Brain.Screen.print((currentTime - lastRadio1Contact < RADIO_TIMEOUT_MS) ? "OK" : "TIMEOUT");
        Brain.Screen.print("R2: ");
        Brain.Screen.print((currentTime - lastRadio2Contact < RADIO_TIMEOUT_MS) ? "OK" : "TIMEOUT");

        wait(10, msec);
    }
}

/**
 * @brief Thread function to display system time regularly on the Brain screen.
 *
 * This thread updates the Brain screen every second with the current time (in milliseconds) since the program started.
 * It helps visualize the passage of time and confirm that the system is actively running.
 *
 * @note Should be run in a separate thread. Relies on `threadsRunning` flag for control.
 *
 * @param None
 * @return void
 */
void statusThread() {
    while(threadsRunning) {
        Brain.Screen.setCursor(9, 1);
        Brain.Screen.clearLine();
        Brain.Screen.print("Time: ");
        Brain.Screen.print(Brain.timer(msec));
        Brain.Screen.print("ms");

        wait(1000, msec);
    }
}

int main() {
    // Initialize serial receiver
    serialReceiverInit();

    Brain.Screen.clearScreen();
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.print("Multi-threaded Communication System");
    Brain.Screen.newLine();

    // Create and start threads
    thread serialThreadObj = thread(serialThread);
    thread radioThreadObj = thread(radioThread);
    thread statusThreadObj = thread(statusThread);

    // priority from highest to lowest
    radioThreadObj.setPriority(25);
    serialThreadObj.setPriority(20);
    statusThreadObj.setPriority(10);

    Brain.Screen.print("All threads started successfully!");
    Brain.Screen.newLine();

    while(true) {
        if (Brain.Screen.pressing()) {
            Brain.Screen.setCursor(11, 1);
            Brain.Screen.print("Shutting down threads... ");
            threadsRunning = false;

            // wait for threads to finish
            serialThreadObj.join();
            radioThreadObj.join();
            statusThreadObj.join();

            Brain.Screen.print("All threads stopped.");
            break;
        }

        wait(100, msec);
    }

    return 0;
}