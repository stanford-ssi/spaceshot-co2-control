# Spaceshot CO2 Control Software

This is the code for an initial ground test of the spaceshot stability ground test CO2 spinup mechanism.

An ESP8266 is the main microcontroller. It reads data from an MMA8451 accelerometer, logs it to an SD card, and triggers a solenoid valve from a WiFi command.
