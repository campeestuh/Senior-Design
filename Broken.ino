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
const int treadmillIncrement = 17;  // Smaller increment for treadmill speed
const int augerIncrement = 15;      // Smaller increment for auger speed

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
  treadmill.setMaxSpeed(maxSpeed);
  treadmill.setAcceleration(100);    // Lower acceleration for a smoother start and stop

  // Set maximum speed and acceleration (Auger)
  auger.setMaxSpeed(augerMaxSpeed);
  auger.setAcceleration(100);        // Lower acceleration
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

    float rpm = (abs(treadmillSpeed) * 60.0) / 400;

    // Calculate the linear speed of the treadmill in meters per second
    float linearSpeed = (rpm * 0.0762 * PI) / 60.0;

    // Display the calculated RPM and linear speed in meters per second
    Serial.println(linearSpeed);
    // Update the last CLK state
    lastCLKState = currentCLKState;
  }

  // Run the motors at their set speeds
  treadmill.runSpeed();
  auger.runSpeed();
}

 void slowDown(int &treadmillSpeed) {
  treadmill.setSpeed(treadmillSpeed);
  treadmill.runSpeed();
  Serial.println("Emergency stop activated - slowing down.");

  while (treadmillSpeed > 0) {
    // Gradually reduce speed with a dynamic acceleration rate
  
    treadmill.setAcceleration(treadmillSpeed / 6.5);  // Gradual deceleration

    // Reduce treadmill speed and constrain it
    treadmillSpeed -= max(1, treadmillSpeed * 0.05);  // Reduce by 5% of current speed
    treadmillSpeed = constrain(treadmillSpeed, 0, maxSpeed);

    treadmill.setSpeed(-treadmillSpeed); // Negative for reverse direction
    Serial.println(treadmillSpeed);
    
    treadmill.runSpeed();
    delay(10);  // Frequent updates for smoother slowdown
  }

  treadmill.setSpeed(0);
  treadmill.runSpeed();
  Serial.println("Treadmill stopped.");
}

