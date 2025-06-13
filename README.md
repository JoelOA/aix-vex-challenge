# AIX-VEX-Challenge

This repository contains the code for a vision-based line-following robot, developed for the AIX-VEX Challenge. The system uses a **Raspberry Pi** as the primary vision processing unit, equipped with a **Camera Module 3**, to detect and track a line. When the robot deviates from the line, the Raspberry Pi sends corrective commands to the **VEX V5 Brain** to steer the robot back on track.

---
## Overview

The core idea behind this project is to leverage the computational power and vision capabilities of the Raspberry Pi to enhance the autonomous navigation of a VEX V5 robot. The Raspberry Pi acts as the "eyes" of the robot, continuously analyzing the camera feed to determine the robot's position relative to the line. Based on this analysis, it calculates and transmits appropriate motor control signals to the VEX V5 Brain, which then executes the movements.

---
## Features

* **Vision-Based Line Detection:** Utilizes the Raspberry Pi Camera Module 3 for real-time image acquisition and processing.
* **Line Following Algorithm:** Implements a robust algorithm to identify and track the line, even with minor imperfections or turns.
* **Raspberry Pi to VEX V5 Communication:** Establishes a communication link between the Raspberry Pi and the VEX V5 Brain to send corrective commands.
* **Autonomous Correction:** Enables the robot to autonomously adjust its trajectory to stay on the line.

---
## Hardware Requirements

* **VEX V5 Robot:** A fully assembled VEX V5 robot chassis with motors and drive train.
* **VEX V5 Brain:** The main controller for the VEX V5 robot.
* **Raspberry Pi:** (e.g., Raspberry Pi 4 Model B) for vision processing.
* **Raspberry Pi Camera Module 3:** For capturing the visual feed.
* **Appropriate Cables:** USB cables for communication and power.

---
## Software Requirements

* **Raspberry Pi OS:** Operating system for the Raspberry Pi.
* **Python 3:** Programming language for the vision algorithm on the Raspberry Pi.
* **OpenCV:** Library for computer vision tasks on the Raspberry Pi.
* **PROS (or VEXcode Pro V5):** Firmware development environment for the VEX V5 Brain.
* **Pyserial (or similar):** Python library for serial communication between the Raspberry Pi and VEX V5 Brain (if using serial).

---
## Getting Started

### 1. Hardware Setup

* Mount the Raspberry Pi and Camera Module 3 securely on your VEX V5 robot, ensuring the camera has a clear view of the ground in front of the robot.
* Establish a communication link between the Raspberry Pi and the VEX V5 Brain. This can be done via USB-to-serial communication, Wi-Fi, or other suitable methods.

### 2. VEX V5 Brain Programming

* Using PROS or VEXcode Pro V5, program the VEX V5 Brain to receive commands from the Raspberry Pi and control the robot's motors accordingly. This will involve setting up communication protocols and motor control functions.

### 3. Raspberry Pi Setup

* Install Raspberry Pi OS on your Raspberry Pi.
* Install Python 3 and OpenCV. You can typically do this using `pip`:
    ```bash
    pip install opencv-python
    ```
* Install any necessary communication libraries (e.g., `pyserial`):
    ```bash
    pip install pyserial
    ```
* Clone this repository to your Raspberry Pi:
    ```bash
    git clone [https://github.com/JoelOA/aix-vex-challenge.git](https://github.com/JoelOA/aix-vex-challenge.git)
    ```

### 4. Running the Code

* Navigate to the cloned repository on your Raspberry Pi.
* Run the main Python script that handles the vision processing and communication:
    ```bash
    python main_vision_script.py # (or whatever your main script is named)
    ```
* Ensure the VEX V5 Brain program is running and ready to receive commands.

---

---
## License

This project is licensed under the [MIT License](LICENSE).

---
## Contact

For any questions or inquiries, please contact [JoelOA](https://github.com/JoelOA).

---
