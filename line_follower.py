import cv2
import numpy as np
import time
import subprocess
import threading
from queue import Queue, Empty
import signal
import sys

import serial
import serial.tools.list_ports

class LineFollowingRobot:
    def __init__(self):
        self.frame_queue = Queue(maxsize=2)
        self.serial_command_queue = Queue(maxsize=5)
        self.running = False
        self.process = None
        self.camera_thread = None
        self.serial_thread = None
        self.serial_port_name = None
        self.ser = None
        self.serial_stop_event = threading.Event()
        
        # Line detection parameters
        self.lower_black = np.array([0,0,0])
        self.upper_black = np.array([70,70,70])
        
        self.center_x_threshold_left = 640 * (62.5 /160)
        self.center_x_threshold_right = 640 * (97.5 / 160)


        
        # Start camera
        self.start_libcamera()
        # initialize serial communication port
        self.serial_port_name = self.find_vex_port()
    
    # Camera Functions
    def start_libcamera(self):
        """Start libcamera-vid process"""
        print("Starting libcamera...")
        
        # Command to start libcamera-vid
        cmd = [
            'libcamera-vid',
            '--timeout', '0',
            '--width', '640', 
            '--height', '480',
            '--framerate', '15',
            '--codec', 'mjpeg',
            '--output', '-',
            '--nopreview',
            '--inline',
            '--segment', '1'
        ]
        
        try:
            # Start the process
            self.process = subprocess.Popen(
                cmd, 
                stdout=subprocess.PIPE, 
                stderr=subprocess.PIPE,
                bufsize=0
            )
            
            # Start frame capture thread
            self.running = True
            self.camera_thread = threading.Thread(target=self._capture_frames)
            self.camera_thread.daemon = True
            self.camera_thread.start()
            
            # Wait a bit for camera to initialize
            time.sleep(3)
            print("libcamera started successfully!")
            
        except Exception as e:
            print(f"Error starting libcamera: {e}")
            raise
    
    def _capture_frames(self):
        """Capture frames from libcamera subprocess"""
        buffer = b''
        
        while self.running and self.process and self.process.poll() is None:
            try:
                # Read data from subprocess
                data = self.process.stdout.read(4096)
                if not data:
                    if self.process.poll() is not None:
                        break
                    time.sleep(0.01)
                    continue
                    
                buffer += data
                
                # Look for JPEG markers
                while True:
                    # Find JPEG start marker (FF D8)
                    start_marker = buffer.find(b'\xff\xd8')
                    if start_marker == -1:
                        break
                    
                    # Find JPEG end marker (FF D9)
                    end_marker = buffer.find(b'\xff\xd9', start_marker)
                    if end_marker == -1:
                        break
                    
                    # Extract JPEG frame
                    jpeg_data = buffer[start_marker:end_marker + 2]
                    buffer = buffer[end_marker + 2:]
                    
                    # Decode JPEG to OpenCV frame
                    frame = cv2.imdecode(np.frombuffer(jpeg_data, dtype=np.uint8), cv2.IMREAD_COLOR)
                    
                    if frame is not None:
                        # Add to queue, remove old frame if queue is full
                        if not self.frame_queue.full():
                            self.frame_queue.put(frame)
                        else:
                            try:
                                self.frame_queue.get_nowait()
                                self.frame_queue.put(frame)
                            except:
                                pass
                    
            except Exception as e:
                if self.running:
                    print(f"Frame capture error: {e}")
                break
        print("Camera capture thread stopped.")

    def get_frame(self):
        """Get latest frame from queue"""
        try:
            return self.frame_queue.get(timeout=1.0)
        except Empty:
            return None
    
    def detect_line_and_position(self, frame):
        """Detects the line using colour masking and contour centroid.
        Returns the (cx, cy) of the line centroid or None if no line is found"""

        mask = cv2.inRange(frame, self.lower_black, self.upper_black)

        contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)

        cx = None
        cy = None

        largest_contour = None

        if len(contours) > 0:

            largest_contour = max(contours, key=cv2.contourArea)
            M = cv2.moments(largest_contour)

            if M["m00"] != 0:
                cx = int(M['m10'] / M['m00'])
                cy = int(M['m10'] / M['m00'])

        return cx, cy, mask, largest_contour
    
    def decide_action(self, cx):
        """Decide action based on centroid position"""
        action = "STOP" # Default action

        if cx is None:
            action = "STOP"
        else:
            if cx >= self.center_x_threshold_right:
                action = "RIGHT"
            elif cx < self.center_x_threshold_right and cx > self.center_x_threshold_left:
                action = "FORWARD"
            elif cx <= self.center_x_threshold_left:
                action = "LEFT"
        

        print(f"Decision: {action}")
        # put the action into the queue for the serial thread to send
        try:
            self.serial_command_queue.put_nowait(action)
        except Exception as e:
            print(f"Warning: could not add action to serial queue: {e}")
        return action
        
    def draw_debug_info(self, frame, mask, cx, cy, contour, action):
        """Draw debug overlays"""
        height, width = frame.shape[:2]
        
        # Draw the target regions
        cv2.line(frame, (int(self.center_x_threshold_left), 0), (int(self.center_x_threshold_left), height), (255, 0, 0), 2)
        cv2.line(frame, (int(self.center_x_threshold_right), 0), (int(self.center_x_threshold_right), height), (255, 0, 0), 2)

        # Draw detected contour and centroid
        if contour is not None:
            cv2.drawContours(frame, [contour], -1, (0, 255, 0), 2) # Draw the largest contour
            if cx is not None and cy is not None:
                cv2.circle(frame, (cx, cy), 5, (0, 0, 255), -1) # Draw the centroid (red dot)
        
        # Text overlays
        cv2.putText(frame, f"Action: {action}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
        if cx is not None:
            cv2.putText(frame, f"CX: {cx}", (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        cv2.putText(frame, "Press 'q' to quit", (10, height - 20), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        
        return frame, mask
    
    # Serial Functions
    def find_vex_port(self):
        """
        Find the VEX V5 Brain USB port automatically
        """
        print("Searching for VEX V5 Brain port...")

        ports = serial.tools.list_ports.comports()
        vex_port = None

        for port in ports:
            # look for VEX-related identifiers
            if any(keyword in port.description.upper() for keyword in ['VEX', 'V5', 'BRAIN']):
                vex_port = port.device
                break
            # On some systems, VEX shows up with different identifiers
            if any(keyword in str(port.hwid).upper() for keyword in ['VEX', 'V5']):
                vex_port = port.device
                break
            
        if vex_port:
            print(f"Found VEX V5 Brain on port: {vex_port}")
            return vex_port
        else:
            print("Could not automatically find VEX V5 port.")
            print("Available ports: ")
            for i, p in enumerate(ports):
                print(f" {i + 1}: {p.device} - {p.desccription}")

            port_name = input("Enter the port name manually (e.g., COM3, /dev/ttyACM0): ")
            return port_name
        
    def _serial_worker(self):
        """
        Dedicated thread function for sending commands to vex V5
        This thread establishes and maintains the serial connection
        """

        if not self.serial_port_name:
            print("Serial worker: No VEX V5 port identified. Exiting serial thread.")
            return
        
        try:
            print(f"Serial worker: Connecting to VEX V5 on port {self.serial_port_name}...")
            self.ser = serial.Serial(
                port = self.serial_port_name,
                baudrate=115200,
                timeout=0.1,
                write_timeout=0.1
            )
            print(f"Serial worker: Connected to VEX V5 on {self.serial_port_name}")

            while not self.serial_stop_event.is_set():
                try:
                    action_to_send = self.serial_command_queue.get(timeout=0.05)
                    message_bytes = (action_to_send + '\n').encode('utf-8')
                    self.ser.write(message_bytes)
                    print(f"Serial worker: Sent: {action_to_send}")

                except Empty:
                    pass
                except serial.SerialTimeoutError:
                    print("Serial worker: Timeout sending/receiving data.")
                except Exception as e:
                    print(f"Serial worker: Error during communication: {e}")
                    self.serial_stop_event.set()

            print("Serial worker: Stop event received. Exiting loop.")

        except serial.SerialException as e:
            print(f"Serial worker: Could not connect to VEX V5: {e}")
            print("Serial worker: Make sure:")
            print("1. VEX V5 Brain is connected via USB")
            print("2. No other programs are using the port")
            print("3. The correct port is specified")
        except Exception as e:
            print(f"Serial worker: An unexpected error occurred in serial thread: {e}")
        finally:
            if self.ser and self.ser.is_open:
                self.ser.close()
                print("Serial worker: Serial port closed.")
            self.serial_command_queue.task_done()

    def run(self, show_video=True):
        """Main detection loop"""
        print("Starting line detection...")
        print("Commands: MOVE FORWARD, TURN LEFT, TURN RIGHT, STOP")
        print("Press 'q' to quit")
        
        # Setup signal handler for clean exit
        def signal_handler(sig, frame):
            print("\nShutting down...")
            self.cleanup()
            sys.exit(0)
        
        signal.signal(signal.SIGINT, signal_handler)
        
        # start the serial communication thread
        if self.serial_port_name:
            self.serial_thread = threading.Thread(target=self._serial_worker)
            self.serial_thread.daemon = True
            self.serial_thread.start()
            print("Serial communication thread started.")
        else:
            print("Skipping serial communication thread: VEX V5 port not available.")

        try:
            no_frame_count = 0
            while self.running:
                frame = self.get_frame()
                
                if frame is None:
                    no_frame_count += 1
                    if no_frame_count > 30:  # 30 failed attempts
                        print("Too many failed frame reads, exiting...")
                        break
                    time.sleep(0.1)
                    continue
                
                no_frame_count = 0  # Reset counter
                
                # Process frame
                cx, cy, mask, contour = self.detect_line_and_position(frame)

                # Make decision
                action = self.decide_action(cx)
                
                # Display
                if show_video:
                    debug_frame, debug_mask = self.draw_debug_info(frame.copy(), mask, cx, cy, contour, action)
                    cv2.imshow('Line Detection', debug_frame)
                    cv2.imshow('Line Detection Mask', debug_mask)
                    #cv2.imshow('Binary', binary)
                    #cv2.imshow('ROI', roi)
                    
                    key = cv2.waitKey(1) & 0xFF
                    if key == ord('q') or key == 27:
                        break
                
                time.sleep(0.01)
                
        except Exception as e:
            print(f"Error in main loop: {e}")
        finally:
            self.cleanup()
    
    def cleanup(self):
        """Clean up resources"""
        print("Cleaning up...")
        self.running = False
        self.serial_stop_event.set()
        
        # Stop camera thread
        if self.camera_thread and self.camera_thread.is_alive():
            print("Waiting for camera thread to finish...")
            self.camera_thread.join(timeout=2)
            if self.camera_thread.is_alive():
                print("Camera thread did not terminate gracefully.")
        
        # Stop serial thread
        if self.serial_thread and self.serial_thread.is_alive():
            print("Waiting for serial thread to finish...")
            self.serial_thread.join(timeout=5)
            if self.serial_thread.is_alive():
                print("Serial thread did not terminate gracefully.")

        # stop libcamera process
        if self.process:
            print("Terminating libcamera process...")
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                print("libcamera process did not terminate gracefull, killing it.")
                self.process.kill()
        
        # close serial port if it was opened
        if self.ser and self.ser.is_open:
            self.ser.close()

        cv2.destroyAllWindows()
        print("Cleanup completed")

if __name__ == "__main__":
    print("Raspberry Pi Camera Module 3 Line Detector with Threaded VEX Communication")
    print("Make sure your camera is connected and enabled!")
    
    try:
        detector = LineFollowingRobot()
        
        # Adjust parameters for your setup
        # detector.line_threshold = 50  # Adjust for line darkness
        # detector.center_tolerance = 30  # Tolerance for "centered" 
        # detector.roi_height = 100  # ROI height
        
        detector.run(show_video=True)
        
    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"Error: {e}")
        print("\nTroubleshooting:")
        print("1. Check camera connection")
        print("2. Test with: libcamera-hello --preview")
        print("3. Make sure camera is enabled in raspi-config")
    finally:
        if 'detector' in locals():
            detector.cleanup()