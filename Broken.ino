#include <AccelStepper.h>

// Define stepper motor connections and interface type
#define dirPin 8      // Direction pin
#define stepPin 9     // Step pulse pin
#define enablePin 7   // Enable pin (optional)
#define motorInterfaceType 1  // 1 for Driver mode (2 pins: Step and Direction)
#define augerInterfaceType 1  // Auger motor interface type
#define CLK 4  // Rotary encoder Clock pin
#define DT 5   // Rotary encoder Data pin
#define augerDirPin 7  // Auger Driver Direction Pin
#define augerStepPin 6 // Auger Driver Pulse Pin
#define Button 2       // Emergency Stop Button

// Create stepper objects
AccelStepper treadmill(motorInterfaceType, stepPin, dirPin);
AccelStepper auger(augerInterfaceType, augerStepPin, augerDirPin);

// Variables to store rotary encoder states
int lastCLKState;
int currentCLKState;
int currentDTState;

int augerSpeed = 0;
const int augerMaxSpeed = 4000;

int treadmillSpeed = 0;      // Motor speed starts at 0
const int maxSpeed = 1337;   // Max speed for the treadmill motor

// Adjust these values to fine-tune the speed increments
const int treadmillIncrement = 17;  // Increment for treadmill speed
const int augerIncrement = 26;      // Increment for auger speed

void setup() {
  Serial.begin(9600);

  // Set enable pin as output
  pinMode(enablePin, OUTPUT);
  pinMode(Button, INPUT_PULLUP); // Use internal pull-up resistor

  // Enable the motor driver
  digitalWrite(enablePin, LOW);  // LOW to enable the motor (depends on driver)

  // Set rotary encoder pins as inputs
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);

  // Initialize encoder state
  lastCLKState = digitalRead(CLK);

  // Set maximum speed and acceleration (Treadmill)
  treadmill.setMaxSpeed(maxSpeed);     // Max speed is 1337 steps per second
  treadmill.setAcceleration(2000);    // High acceleration for quick ramp-up

  // Set maximum speed and acceleration (Auger)
  auger.setMaxSpeed(augerMaxSpeed);   // Max speed is 4000 steps per second
  auger.setAcceleration(2000);       // High acceleration
}

void loop() {
  // Check if the button is pressed
  while (digitalRead(Button) == LOW) {
    slowDown(treadmillSpeed); // Slow down the treadmill
  }

  // Read the current state of the rotary encoder
  currentCLKState = digitalRead(CLK);
  currentDTState = digitalRead(DT);

  // If the rotary encoder has been turned
  if (currentCLKState != lastCLKState) {
    // Clockwise rotation (increase speed)
    if (digitalRead(DT) != currentCLKState) {
      treadmillSpeed += treadmillIncrement; // Increase treadmill speed
      augerSpeed += augerIncrement;         // Increase auger speed
    } 
    // Counterclockwise rotation (decrease speed)
    else {
      treadmillSpeed -= treadmillIncrement; // Decrease treadmill speed
      augerSpeed -= augerIncrement;         // Decrease auger speed
    }

    // Constrain speeds within valid range
    treadmillSpeed = constrain(treadmillSpeed, 0, maxSpeed);
    augerSpeed = constrain(augerSpeed, 0, augerMaxSpeed);

    // Update motor speeds
    treadmill.setSpeed(-treadmillSpeed); // Negative speed for reverse direction
    auger.setSpeed(augerSpeed);

    // Display the current speed
    Serial.print("Treadmill Speed: ");
    Serial.println(treadmillSpeed);

    // Update the last CLK state
    lastCLKState = currentCLKState;
  }

  // Run the motors at their set speeds
  treadmill.runSpeed();
  auger.runSpeed();
}

// Function to slow down the treadmill gradually
void slowDown(int &treadmillSpeed) { // Pass by reference
 treadmill.setSpeed(treadmillSpeed);
  treadmill.runSpeed();
  Serial.println("Emergency stop activated - slowing down.");
  while (treadmillSpeed > 0) {
    // Decrease speed dynamically based on the current speed
    delay(1000);
    treadmillSpeed -= 17;

    // Constrain to ensure speed doesn't go below 0
    treadmillSpeed = constrain(treadmillSpeed, 0, 1377);

    // Apply the updated speed
    treadmill.setSpeed(-treadmillSpeed); // Negative for reverse direction

    Serial.println(treadmillSpeed);

    // Run the motor at the updated speed
    treadmill.runSpeed();

    // Delay to allow for smoother processing
    delay(200); // Shorter delay for finer granularity
  }

  // Ensure the motor is fully stopped

 
  Serial.println("Treadmill stopped.");
}
