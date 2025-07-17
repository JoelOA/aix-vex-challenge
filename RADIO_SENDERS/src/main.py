# ---------------------------------------------------------------------------- #
#                                                                              #
# 	Module:       main.py                                                      #
# 	Author:       Joel Osei-Asamoah                                            #
# 	Created:      7/14/2025, 9:55:26 PM                                        #
# 	Description:  V5 project                                                   #
#                                                                              #
# ---------------------------------------------------------------------------- #

# Library imports
from vex import *

# Brain should be defined by default
brain=Brain()

brain.screen.print("Hello V5")

serial_link = SerialLink(Ports.PORT6, "link_worker_1", VexlinkType.WORKER)

while(brain.timer.time() < 6999):
    serial_link.send("ALPHA")
        
while(True):
    brain.screen.clear_screen()
    brain.screen.set_cursor(1, 1)
        
    buffer = serial_link.receive(128, 1000)
    if buffer:
        try:
            # Convert buffer to string and clean it up
            received_string = buffer.decode('utf-8', errors='ignore')
            received_string = received_string.rstrip('\x00').strip()  # Remove null terminators and whitespace
            
            if received_string:
                brain.screen.print("RX: " + received_string)
            else:
                brain.screen.print("Empty message")
                
        except Exception as e:
            brain.screen.print("Decode error")
        wait(20, MSEC)