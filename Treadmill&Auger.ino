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
const int maxSpeed = 3000; // Max speed for the motor

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
      treadmillSpeed += 100; // Increase speed
      augerSpeed += 133;
    } 
    // Counterclockwise rotation (decrease speed)
    else {
      treadmillSpeed -= 100; // Decrease speed
      augerSpeed -= 133;
    }
    
    // Constrain motor speed to be within 0 and maxSpeed
    treadmillSpeed = constrain(treadmillSpeed, 0, maxSpeed);
    augerSpeed = constrain(augerSpeed, 0, augerMaxSpeed);

    // Reverse the direction of the motor
    treadmill.setSpeed(-treadmillSpeed); // Apply the negative of motorSpeed to reverse direction (Clockwise)
    auger.setSpeed(augerSpeed);


    // Update the last CLK state
    lastCLKState = currentCLKState;
  }

  // Run the motor at the set speed
  treadmill.runSpeed();
  auger.runSpeed();
}
