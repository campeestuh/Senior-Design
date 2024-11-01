#include <AccelStepper.h>

// Define stepper motor connections and interface type
#define dirPin 8      // Direction pin
#define stepPin 9     // Step pulse pin
#define enablePin 7   // Enable pin (optional)
#define motorInterfaceType 1  // 1 for Driver mode (2 pins: Step and Direction)
#define augerInterfaceType 1  // 
#define CLK 4  // Clock pin
#define DT 5   // Data pin
#define augerDirPin 7  // Auger Driver Direction Pin
#define augerStepPin 6 // Auger Driver Pulse Pin 
#define emergencySTOP 2 // Emergency Stop  Button 

// Create a stepper object
AccelStepper treadmill(motorInterfaceType, stepPin, dirPin);
AccelStepper auger(augerInterfaceType, augerStepPin, augerDirPin);

// Variables to store rotary encoder states
int lastCLKState;
int currentCLKState;
int currentDTState;

int augerSpeed = 0;
const int augerMaxSpeed = 4000; 

int treadmillSpeed = 0;     // Motor speed starts at 0
const int maxSpeed = 1337; // Max speed for the motor

// Adjust these values to fine-tune the speed increments
const int treadmillIncrement = 17;  // New finer increment for treadmill speed
const int augerIncrement = 26;      // New finer increment for auger speed

void setup() {
  Serial.begin(9600);
  // Set enable pin as output
  pinMode(enablePin, OUTPUT);
  // Enable the motor driver
  digitalWrite(enablePin, LOW);  // LOW to enable the motor (depends on driver)
 
  // Set rotary encoder pins as inputs
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  
  // Initialize encoder state
  lastCLKState = digitalRead(CLK);
  
  // Set maximum speed and acceleration (Treadmill)
  treadmill.setMaxSpeed(maxSpeed);     // Max speed is 3000 steps per second
  treadmill.setAcceleration(2000);     // High acceleration for quick ramp-up
  // Set maximum speed and acceleration (Auger)
  auger.setMaxSpeed(augerMaxSpeed);    // Max speed is 4000 steps per second
  auger.setAcceleration(2000);         // High acceletation

}

void loop() {
  // Read the current state of the rotary encoder
  currentCLKState = digitalRead(CLK);
  currentDTState = digitalRead(DT);

  // If the rotary encoder has been turned
  if (currentCLKState != lastCLKState) {
    // Clockwise rotation (increase speed)
    if (digitalRead(DT) != currentCLKState) {
      treadmillSpeed += treadmillIncrement; // Increase speed by finer increment
      augerSpeed += augerIncrement;
    } 
    // Counterclockwise rotation (decrease speed)
    else {
      treadmillSpeed -= treadmillIncrement; // Decrease speed by finer increment
      augerSpeed -= augerIncrement;
    }
    
    // Constrain motor speed to be within 0 and maxSpeed
    treadmillSpeed = constrain(treadmillSpeed, 0, maxSpeed);
    augerSpeed = constrain(augerSpeed, 0, augerMaxSpeed);

    // Reverse the direction of the motor
    treadmill.setSpeed(-treadmillSpeed); // Apply the negative of motorSpeed to reverse direction (Clockwise)
    auger.setSpeed(augerSpeed);

    float rpm = (abs(treadmillSpeed) * 60.0) / 400;

    // Calculate the linear speed of the treadmill in meters per second
    float linearSpeed = (rpm * 0.0762 * PI) / 60.0;

    // Display the calculated RPM and linear speed in meters per second
    Serial.println(linearSpeed);

    // Update the last CLK state
    lastCLKState = currentCLKState;
  }

  // Run the motor at the set speed
  treadmill.runSpeed();
  auger.runSpeed();
}
