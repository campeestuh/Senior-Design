// Define the stepper motor 1 for Arduino
// Direction Digital 9 (CW), pulses Digital 8 (CLK)
#include <AccelStepper.h>
AccelStepper stepper(1, 8, 9);
// For Project need to create more AccelStepper objects. Auger and Harness
// Work on controlling the speed of the motors using the rotary encoder from speed of 0 to maxSpeed
// While having a smooth acceleration safe enough for rat 
// Implement on gapping the speeds of treadmill(Slower) and Auger(Faster)
// Define rotary encoder pins
const int encoderCLK = 4;  // Rotary encoder CLK
const int encoderDT = 5;   // Rotary encoder DT
int lastCLKState;          // To store the last state of the encoder CLK
unsigned long lastMoveTime = 0;  // To track the last movement time
int count = 0;             // Initialize the count for rotary encoder steps

// Define button pin
const int buttonPin = 6;    // Button to be added, but no action yet
int buttonState = 0;        // Variable to hold the button state

int setPosition = 0;  // This is a placer for when the harnessreturn point is set 
bool positionSaved = false;

int notSavedPosition = 0;

const int emergencyButton = 2;
int eButtonState = 0;


void setup() {  
  // Stepper motor setup
  stepper.setMaxSpeed(1000);  // Max speed of the stepper motor
  stepper.setSpeed(0);        // Initially, set the motor speed to 0

  // Rotary encoder setup
  pinMode(encoderCLK, INPUT);
  pinMode(encoderDT, INPUT);
  lastCLKState = digitalRead(encoderCLK);  // Initialize the encoder's last state

  // Button setup
  pinMode(buttonPin, INPUT_PULLUP);  // Set button pin as input with internal pull-up resistor
  pinMode(emergencyButton, INPUT_PULLUP);
  Serial.begin(9600);  // Initialize serial communication for debugging
}

void loop() {
  // Read the rotary encoder
  int currentCLKState = digitalRead(encoderCLK);

  // Check if the encoder has been rotated (state change on CLK pin)
  if (currentCLKState != lastCLKState) {
    // Check the direction of the rotation
    if (digitalRead(encoderDT) == currentCLKState) {
      // Counter Clockwise rotation
      stepper.setSpeed(400);  // Set speed for clockwise rotation
      count--;  // Increment the count
    } else {
      // Clockwise rotation
      stepper.setSpeed(-400);  // Set speed for counterclockwise rotation
      count++;  // Decrement the count
    }
    lastMoveTime = millis();  // Record the time of the last movement
    Serial.print("Current Count: ");  // Print the current count to the serial monitor
    Serial.println(count);
  }

  // If the encoder has stopped turning, stop the motor after a brief delay
  if (millis() - lastMoveTime > 50) {  // 50ms delay before stopping
    stepper.setSpeed(0);  // Stop the motor if no movement is detected
  }

  // Save the last CLK state
  lastCLKState = currentCLKState;

  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) {
    Serial.print("Position Saved");
    setPosition = stepper.currentPosition();
    positionSaved = true;
  }

  // Check if the button is pressed (no action yet)
 eButtonState = digitalRead(emergencyButton);
  if (eButtonState == LOW) {
     if (positionSaved){
    stepper.setMaxSpeed(1000);  // Increase the max speed
    stepper.setAcceleration(400);  // Optional: Set acceleration
    stepper.moveTo(setPosition);  // Move to the where set 
    stepper.runToNewPosition(setPosition);  // Run the motor to the new position
     }
 else{
    notSavedPosition = stepper.currentPosition();
    notSavedPosition += 10;
        stepper.setMaxSpeed(1000);  // Increase the max speed
    stepper.setAcceleration(400);  // Optional: Set acceleration
    stepper.moveTo(notSavedPosition);  // Move to the where set 
    stepper.runToNewPosition(notSavedPosition);  // Run the motor to the new position


    }
 
  }

  // Run the stepper motor at the set speed
  stepper.runSpeed();  // Continuously runs the motor at the set speed
}
