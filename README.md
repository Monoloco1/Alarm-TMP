# Alarm-TMP, a security alarm
This is the project I made for my Microcontrollers II class I took at AGH UST. It is a device that secures a valuable object by sounding an alarm if the object is moved, or a movement is detected in the vicinity. Program written in C99.
## Hardware
![Image of the security alarm in the "safe mode](https://github.com/Monoloco1/Alarm-TMP/assets/84081091/65f9dbca-7514-4ccc-bfee-a2dac4c589a1)

The alarm is based on the Freescale FRDM-KL05Z board that uses a Kinetis KL05Z Cortex-M0 processor. The IO devices that are connected to the main board are as follows:
- A standard 16x2 LCD display
- A speaker(by Waveshare) for the audio signals
- An ALS PT19 photodiode light sensor
- A 4x4 multiplexed keyboard
- The internal accelerometer of the main board
- The main board's included RGB LEDs.
## Software
The program of the device consists mainly of a main loop that processes IO. Data to/from IO devices is processed with the help of interrupts.

## User interface
The end user interacts with the alarm using the 4x4 keyboard. The device has two main modes:
- Admin mode
- User mode
#### Admin mode
To enter this mode the user has to type the admin password. In this mode it's possible to change the detection parameters and the passwords.
The parameters that can be changed are as follows: 
- Accelerometer sensitivity, 0 to disable
- Light sensor sensitivity, 0 to disable
- Light sensor low-pass filter time constant(for filtering noise and light flickering)

![Example parameter change screen](https://github.com/Monoloco1/Alarm-TMP/assets/84081091/ad142797-07e8-4c17-a20e-f44caaa604cd)

The passwords that can be changed:
- Admin password
- Arming password
- Disarming password
- Alarm disabling password

![Example password changing screen](https://github.com/Monoloco1/Alarm-TMP/assets/84081091/eebc7fbd-053e-4cd6-a088-00ecdb2a6f81)

#### User mode
In this mode the user can enable/disable the armed status of the alarm, as well as disable the alarm if it happens. From this mode it's possible to change to admin mode after typing the admin password.

The project's design document(in polish):
[Projekt alarmu.pdf](https://github.com/Monoloco1/Alarm-TMP/files/14454152/Projekt.alarmu.pdf)
