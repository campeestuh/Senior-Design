#include <AccelStepper.h>
// Direction Digital 16 (CW), pulses Digital 15 (CLK)
AccelStepper stepper(1, 15, 16);

// Define button pins
const int buttonPin = 19;        // Button to save position
const int emergencyButton = 26;  // DPDT emergency button
const int clockwiseButton = 27;  // Momentary button for clockwise rotation
const int counterclockwiseButton = 28;  // Momentary button for counterclockwise rotation

int buttonState = 0;
int eButtonState = 0;
int setPosition = 0;
bool positionSaved = false;
bool eButtonPressed = false;

void setup() {
  stepper.setMaxSpeed(800);
  stepper.setAcceleration(300);
  stepper.setSpeed(0);

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(emergencyButton, INPUT_PULLUP);
  pinMode(clockwiseButton, INPUT_PULLUP);
  pinMode(counterclockwiseButton, INPUT_PULLUP);

  Serial.begin(115200);
}

void loop() {
  // Save Position
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) {
    delay(200);
    Serial.println("Position Saved");
    setPosition = stepper.currentPosition();
    positionSaved = true;
  }

  // Emergency Button Logic
  eButtonState = digitalRead(emergencyButton);

  if (eButtonState == LOW && !eButtonPressed) {
    eButtonPressed = true;

    if (positionSaved) {
      Serial.println("Returning to Saved Position");
      stepper.setMaxSpeed(800);
      stepper.setAcceleration(300);
      stepper.moveTo(setPosition);
      stepper.runToPosition();
    } else {
      Serial.println("Ascending Off Platform");
      int ascendPosition = stepper.currentPosition() - 500;
      stepper.setMaxSpeed(800);
      stepper.setAcceleration(300);
      stepper.moveTo(ascendPosition);
      stepper.runToPosition();
    }
  }

  if (eButtonState == HIGH) {
    eButtonPressed = false;
  }

  // Momentary Button Logic
  if (digitalRead(clockwiseButton) == LOW) {
    stepper.setSpeed(200);  // Clockwise rotation at 200 pulses
    stepper.runSpeed();
  } else if (digitalRead(counterclockwiseButton) == LOW) {
    stepper.setSpeed(-200);  // Counterclockwise rotation at 200 pulses
    stepper.runSpeed();
  } else {
    stepper.setSpeed(0);  // Stop the motor if no button is pressed
  }

  stepper.runSpeed();
}
