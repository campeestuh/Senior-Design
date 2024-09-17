Circuit Wiring:

Rotary Encoder   --> FREENOVE ESP32 WROOM
> CLK (Output A) --> Pin 4 
> DT  (Output B) --> Pin 5 
> SW  (Switch)   --> Pin 16 
>  +  (VCC)      --> 3.3V 
> GND            --> GND

LCD 
VCC                 --> 5V
SDA (Serial Data)   --> Pin 21
SCL (Serical Clock) --> Pin 22
GND                 --> GND

Button 
GND    --> GND
Signal --> Pin 19

Arduino IDE
Board: ESP32 Wrover Module
How to get ESP32 Board Support:
- Go to File > Preferences
- In "Additional Board Manager URLs" add this URL:
https://dl.espressif.com/dl/package_esp32_index.json
- Click OK and save
  
Install LiquidCrystal Library by Adafruit

