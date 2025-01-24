// Define the stepper motor 1 for Arduino
// Direction Digital 16 (CW), pulses Digital 15 (CLK)
#include <AccelStepper.h>
AccelStepper stepper(1, 15, 16);

const int encoderCLK = 4;  // Rotary encoder CLK
const int encoderDT = 5;   // Rotary encoder DT
int lastCLKState;          // To store the last state of the encoder CLK
unsigned long lastMoveTime = 0;  // To track the last movement time
int count = 0;             // Initialize the count for rotary encoder steps

// Define button pins
const int buttonPin = 19;    // Button to save position
const int emergencyButton = 26;  // DPDT emergency button
int buttonState = 0;        // Variable to hold the button state
int eButtonState = 0;       // Variable for emergency button state

int setPosition = 0;        // Placeholder for saved position
bool positionSaved = false; // Flag for saved position
bool eButtonPressed = false; // Track emergency button press

void setup() {  
  // Stepper motor setup
  stepper.setMaxSpeed(800);         // Adjust max speed for smooth operation
  stepper.setAcceleration(300);    // Set acceleration for smoother transitions
  stepper.setSpeed(0);             // Initially, set the motor speed to 0

  // Rotary encoder setup
  pinMode(encoderCLK, INPUT);
  pinMode(encoderDT, INPUT);
  lastCLKState = digitalRead(encoderCLK);  // Initialize the encoder's last state

  // Button setup
  pinMode(buttonPin, INPUT_PULLUP);  // Set button pin as input with internal pull-up resistor
  pinMode(emergencyButton, INPUT_PULLUP);
  Serial.begin(115200);  // Initialize serial communication for debugging
}

void loop() {
  // Read the rotary encoder
  int currentCLKState = digitalRead(encoderCLK);

  // Check if the encoder has been rotated (state change on CLK pin)
  if (currentCLKState != lastCLKState) {
    if (digitalRead(encoderDT) == currentCLKState) {
      // Counter Clockwise rotation
      count--;  // Increment the count
      stepper.setSpeed(-250);  // Adjust speed for counterclockwise rotation
    } else {
      // Clockwise rotation
      count++;  // Decrement the count
      stepper.setSpeed(250);  // Adjust speed for clockwise rotation
    }
    lastMoveTime = millis();  // Record the time of the last movement
    Serial.print("Current Count: ");  // Print the current count to the serial monitor
    Serial.println(count);
  }

  // If the encoder has stopped turning, stop the motor after a brief delay
  if (millis() - lastMoveTime > 20) {  // 50ms delay before stopping
    stepper.setSpeed(0);  // Stop the motor if no movement is detected
  }

  // Save the last CLK state
  lastCLKState = currentCLKState;

  // Save Position
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) {
    delay(200);  // Debounce delay
    Serial.println("Position Saved");
    setPosition = stepper.currentPosition();
    positionSaved = true;
  }

  // Emergency Button Logic
  eButtonState = digitalRead(emergencyButton);

  if (eButtonState == LOW && !eButtonPressed) {  // Button pressed
    eButtonPressed = true;  // Mark the button as pressed

    if (positionSaved) {
      Serial.println("Returning to Saved Position");
      stepper.setMaxSpeed(800);  // Ensure smooth speed for return
      stepper.setAcceleration(300);  // Smooth acceleration for return
      stepper.moveTo(setPosition);  // Move to the saved position
      stepper.runToPosition();  // Run the motor to the saved position
    } else {
      Serial.println("Ascending Off Platform");
      int ascendPosition = stepper.currentPosition() - 500;  // Ascend a fixed amount
      stepper.setMaxSpeed(800);  // Ensure smooth speed for ascending
      stepper.setAcceleration(300);  // Smooth acceleration for ascending
      stepper.moveTo(ascendPosition);  // Move to the new position
      stepper.runToPosition();  // Run the motor to the new position
    }
  }

  if (eButtonState == HIGH) {  // Button released
    eButtonPressed = false;  // Reset the button press state
  }

  // Run the stepper motor at the set speed
  stepper.runSpeed();  // Continuously runs the motor at the set speed
}
