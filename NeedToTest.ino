#include <LiquidCrystal_PCF8574.h>
#include <AccelStepper.h>

// Pin Definitions
#define encoderCLK 4
#define encoderDT 5
#define rotaryButton 18
//#define enablePin 12   // Enable pin  FOR TREADMILL (optional)
#define treadDirectionPin 25
#define treadPulsePin 27
#define augerDirectionPin 14
#define augerPulsePin 13
#define harnessDirectionPin 16
#define harnessPulsePin 15
#define Button 19
#define emergencyButton 26
#define clockwiseButton 27
#define counterclockwiseButton 28

// LCD Settings
const int lcdColumns = 16, lcdRows = 2;
LiquidCrystal_PCF8574 lcd(0x27);

// Treadmill Settings
AccelStepper treadmill(1, treadPulsePin, treadDirectionPin);
int treadmillSpeed = 0;                   // Initial Treadmill Speed
const int maxSpeed = 1337;                // Max Treadmill Speed
const int treadmillIncrement = 17;        // Speed Adjustment Step

// Auger Settings
AccelStepper auger(1, augerPulsePin, augerDirectionPin);
int augerSpeed = 0;                       // Initial Auger Speed
const int augerMaxSpeed = 2044;           // Max Auger Speed
const int augerIncrement = 26;            // Speed Adjustment Step

// Harness Motor Settings
AccelStepper stepper(1, harnessPulsePin, harnessDirectionPin); // Harness motor
bool positionSaved = false;
int setPosition = 0; // Saved position for harness adjustments
bool eButtonPressed = false;
unsigned long lastMoveTime = 0;  // To track the last movement time
int count = 0;             // Initialize the count for rotary encoder steps

// Menu Variables
enum Mode { MENU, SPEED_MODE, HARNESS_MODE } currentMode = MENU;
const String menuItems[] = {"SPEED", "HARNESS"};
int menuIndex = 0;
const int numMenuItems = 2;

// Rotary Encoder State
int lastCLKState = LOW;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Button Release Guard
bool buttonReleaseGuard = false;
unsigned long buttonReleaseTime = 0;
const unsigned long buttonReleaseDelay = 200; // Ignore button presses for 200ms

void setup() {
  Serial.begin(115200);
  // // set enablePin as output
  // pinMode(enablePin, OUTPUT);
  // // Enable the motor driver
  // digitalWrite(enablePin, LOW);  // LOW to enable the motor (depends on driver)
 
  // Initialize LCD
  lcd.begin(lcdColumns, lcdRows);
  lcd.setBacklight(255);
  displayMenu();

  // Initialize Rotary Encoder Pins
  pinMode(encoderCLK, INPUT);
  pinMode(encoderDT, INPUT);
  pinMode(rotaryButton, INPUT_PULLUP);
  pinMode(Button, INPUT_PULLUP);
  pinMode(emergencyButton, INPUT_PULLUP);
  pinMode(clockwiseButton, INPUT_PULLUP);
  pinMode(counterclockwiseButton, INPUT_PULLUP);

  // Treadmill Setup
  treadmill.setMaxSpeed(maxSpeed);
  treadmill.setAcceleration(2000);

  // Auger Setup
  auger.setMaxSpeed(augerMaxSpeed);
  auger.setAcceleration(2000);

  // Harness Motor Setup
  stepper.setMaxSpeed(800);         // Adjust max speed for smooth operation
  stepper.setAcceleration(300);    // Set acceleration for smoother transitions
  stepper.setSpeed(0);             // Initially, set the motor speed to 0

  lastCLKState = digitalRead(encoderCLK);
}

void loop() {
  static Mode previousMode = MENU; // Keep track of the previous mode

  if (currentMode == MENU) {
    // Guard against lingering button presses after returning to MENU
    if (buttonReleaseGuard && millis() - buttonReleaseTime > buttonReleaseDelay) {
      buttonReleaseGuard = false; // Clear the guard after the delay
    }

    if (!buttonReleaseGuard) {
      // Refresh menu display when transitioning to MENU mode
      if (previousMode != MENU) {
        lcd.clear();
        displayMenu();
        previousMode = MENU; // Update the mode tracker
      }
      handleEncoderForMenu(); // Handle rotary encoder navigation
      if (buttonPressed(rotaryButton)) handleMenuSelection(); // Select menu item
    }

    // Keep the treadmill running at the set speed
    if (treadmillSpeed > 0) {
      treadmill.runSpeed(); // Ensure treadmill continues to run
      auger.runSpeed();
    }
  } else if (currentMode == SPEED_MODE) {
    handleSpeedMode();
    if (buttonPressed(rotaryButton)) returnToMenu(); // Return to menu if button is pressed
  } else if (currentMode == HARNESS_MODE) {
    handleHarnessMode();
    if (buttonPressed(rotaryButton)) returnToMenu(); // Return to menu if button is pressed
  }
}

// Function to Handle Harness Mode Logic
void handleHarnessMode() {
  // Save Position
  if (buttonPressed(Button)) {
    Serial.println("Position Saved");
    setPosition = stepper.currentPosition();
    positionSaved = true;
  }

  // Emergency Button Logic
  if (digitalRead(emergencyButton) == LOW && !eButtonPressed) {
    eButtonPressed = true;
    eStopProcedure();
  }

  if (digitalRead(emergencyButton) == HIGH) {
    eButtonPressed = false; // Reset emergency button state
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

// Function to Handle Returning to Saved Position or Ascending Off Platform
void eStopProcedure() {
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

  // Gradually decrease treadmill and auger speed during emergency stop
  while (treadmillSpeed > 0 || augerSpeed > 0) {
    treadmillSpeed = max(0, treadmillSpeed - treadmillIncrement);
    augerSpeed = max(0, augerSpeed - augerIncrement);

    treadmill.setSpeed(-treadmillSpeed);
    auger.setSpeed(augerSpeed);

    treadmill.runSpeed();
    auger.runSpeed();
    delay(200);  // Debounce delay to avoid multiple updates
  }
}

// Function to Handle Rotary Encoder for Menu Navigation
void handleEncoderForMenu() {
  int currentCLKState = digitalRead(encoderCLK);
  if ((millis() - lastDebounceTime) > debounceDelay && currentCLKState != lastCLKState) {
    if (digitalRead(encoderDT) != currentCLKState) {
      menuIndex++; // Clockwise rotation
    } else {
      menuIndex--; // Counter-clockwise rotation
    }
    menuIndex = constrain(menuIndex, 0, numMenuItems - 1); // Ensure menuIndex stays within bounds
    displayMenu(); // Update LCD display
    lastCLKState = currentCLKState;
    lastDebounceTime = millis();
  }
}

// Function to Handle Menu Item Selection
void handleMenuSelection() {
  lcd.clear();
  lcd.print(menuItems[menuIndex] + " SELECTED");
  Serial.println(menuItems[menuIndex] + " SELECTED");
  delay(1000); // Simple delay to display selection

  if (menuIndex == 0) {
    currentMode = SPEED_MODE; // Switch to SPEED_MODE
    lcd.clear();
    lcd.print("Speed Mode");
    treadmill.setSpeed(treadmillSpeed); // Maintain the current treadmill speed
  } else if (menuIndex == 1) {
    currentMode = HARNESS_MODE; // Switch to HARNESS_MODE
    lcd.clear();
    lcd.print("Harness Mode");
  }
}

// Function to Display Menu on LCD
void displayMenu() {
  lcd.clear();             // Ensure the display is fully cleared
  lcd.setCursor(0, 0);     // Reset the cursor position
  lcd.print("MENU");       // Display the menu header
  lcd.setCursor(0, 1);     // Move to the second line
  lcd.print(menuIndex == 0 ? "-> SPEED" : "-> HARNESS"); // Display the menu item
}

// Function to Handle Button Press Debouncing
bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW && (millis() - lastDebounceTime) > debounceDelay) {
    lastDebounceTime = millis();
    return true;
  }
  return false;
}

// Speed Mode Logic
void handleSpeedMode() {
  int currentCLKState = digitalRead(encoderCLK);
  int currentDTState = digitalRead(encoderDT);
  // If the rotary encoder has been turned
  if (currentCLKState != lastCLKState) {
    // Clockwise rotation (increase speed)
    if (digitalRead(encoderDT) != currentCLKState) {
      treadmillSpeed += treadmillIncrement; // Increase speed by finer increment
      augerSpeed += augerIncrement;
    } 
    // Counterclockwise rotation (decrease speed)
    else {
      treadmillSpeed -= treadmillIncrement; // Decrease speed by finer increment
      augerSpeed -= augerIncrement;
    }
    // Constrain the speed within valid range
    treadmillSpeed = constrain(treadmillSpeed, 0, maxSpeed);
    augerSpeed = constrain(augerSpeed, 0, augerMaxSpeed);

    // Update the motor speed
    treadmill.setSpeed(-treadmillSpeed);
    auger.setSpeed(augerSpeed);

    // Calculate RPM and linear speed
    float rpm = (abs(treadmillSpeed) * 60.0) / 400.0; // Assuming 400 steps per revolution
    float linearSpeed = (rpm * 0.0762 * PI) / 60.0;   // Convert RPM to m/s

    // Update the last CLK state
    lastCLKState = currentCLKState;
  }

  // Run the treadmill at the updated speed
  treadmill.runSpeed();
  auger.runSpeed();
}

// Function to Return to Menu
void returnToMenu() {
  currentMode = MENU;                // Switch back to MENU mode
  buttonReleaseGuard = true;         // Set guard flag to prevent immediate selection
  buttonReleaseTime = millis();      // Record the current time

  // Keep treadmill running at the current speed
  treadmill.setSpeed(-treadmillSpeed);

  positionSaved = false;             // Reset saved position flag
  eButtonPressed = false;            // Reset emergency button state

  lcd.clear();                       // Clear the LCD before refreshing the menu
  displayMenu();                     // Refresh the menu display
  Serial.println("Returned to MENU");
}
