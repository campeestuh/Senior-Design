Circuit Wiring:

Rotary Encoder   --> FREENOVE ESP32 WROOM
> CLK (Output A) --> Pin 4 
> DT  (Output B) --> Pin 5 
> SW  (Switch)   --> Pin 16 
>  +  (VCC)      --> 5V 
> GND            --> GND

LCD 
VCC                 --> 5V
SDA (Serial Data)   --> Pin 21
SCL (Serial Clock)  --> Pin 22
GND                 --> GND

Button 
GND    --> GND
Signal --> Pin 19

Harness
STEP/PUL    --> Pin 15
DIRECTION   --> Pin 16
ENABLE      --> 5V

AUGER 
STEP/PUL    --> Pin 13
DIRECTION   --> Pin 14
ENABLE      --> 5V

TREADMILL
STEP/PUL    --> Pin 12
DIRECTION   --> Pin 25
ENABLE      --> 5V

Arduino IDE
Board: ESP32 Wrover Module
How to get ESP32 Board Support:
- Go to File > Preferences
- In "Additional Board Manager URLs" add this URL:
https://dl.espressif.com/dl/package_esp32_index.json
- Click OK and save
  
Install LiquidCrystal Library by Adafruit

