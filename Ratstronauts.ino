// Authors: Juan Campista & Alicia Enriquez     
// Date:    4/28/2024
// Summary: This code implements a rotary encoder-controlled user interface for a treadmill system
//          with two modes: speed control for treadmill & both auger motors and directional control for a harness
//          motor. It displays a menu on an I2C LCD, handles rotary input for navigation and selection,
//          and adjusts motor behavior based on user interaction. Emergency stop and directional
//          control buttons are also integrated for safety and manual overrides.
//----------------------------------------------------------------------------------------------------------------------------//
#include <LiquidCrystal_PCF8574.h> // Include library to control an LCD over I2C
#include <AccelStepper.h>           // Include library to control stepper motors
#include <esp_sleep.h>              // Include library to manage deep sleep on ESP32

// Pin Definitions
#define encoderCLK 4                // Rotary encoder CLK signal pin
#define encoderDT 5                 // Rotary encoder DT signal pin
#define rotaryButton 18             // Rotary encoder push button pin

#define treadDirectionPin 25        // Treadmill motor direction control pin
#define treadPulsePin 27            // Treadmill motor pulse (step) pin

#define augerDirectionPin 14        // Auger motor 1 direction control pin
#define augerPulsePin 13            // Auger motor 1 pulse (step) pin

#define auger2DirectionPin 32       // Auger motor 2 direction control pin
#define auger2PulsePin 33           // Auger motor 2 pulse (step) pin

#define harnessPulsePin 15          // Harness motor pulse (step) pin
#define harnessDirectionPin 16      // Harness motor direction control pin

#define Button 29                   // General-purpose button pin
#define emergencyButton 26          // Emergency stop button pin
#define clockwiseButton 17          // Button to move harness clockwise
#define counterclockwiseButton 23   // Button to move harness counterclockwise

// LCD Settings
const int lcdColumns = 16, lcdRows = 2;        // LCD display dimensions
LiquidCrystal_PCF8574 lcd(0x27);               // Create LCD object at I2C address 0x27

// Treadmill Settings
AccelStepper treadmill(1, treadPulsePin, treadDirectionPin); // Create treadmill stepper motor object
int treadmillSpeed = 0;                       // Current treadmill speed
const int maxSpeed = 1337;                     // Maximum treadmill speed
const int treadmillIncrement = 17;             // Increment to adjust treadmill speed

// Auger Settings
AccelStepper auger(1, augerPulsePin, augerDirectionPin); // Create auger 1 stepper motor object
int augerSpeed = 0;                          // Current auger speed
const int augerMaxSpeed = 2044;               // Maximum auger speed
const int augerIncrement = 26;                // Increment to adjust auger speed

// 2nd Auger (mirrors auger1)
AccelStepper auger2(1, auger2PulsePin, auger2DirectionPin); // Create auger 2 stepper motor object

// Harness Motor Settings
AccelStepper harness(1, harnessPulsePin, harnessDirectionPin); // Create harness stepper motor object
bool positionSaved = false;                  // Track if harness position was saved
int setPosition = 0;                          // Position value to save for harness
bool eButtonPressed = false;                  // Track emergency button pressed state
unsigned long lastMoveTime = 0;                // Timestamp of last movement
int count = 0;                                 // General purpose counter

// Menu Variables
enum Mode { MENU, SPEED_MODE, HARNESS_MODE } currentMode = MENU; // Create modes and set default to MENU
const String menuItems[] = {"SPEED", "HARNESS"}; // Menu items for display
int menuIndex = 0;                             // Current index selected in menu
const int numMenuItems = 2;                    // Total number of menu items

// Rotary Encoder State
int lastCLKState = LOW;                        // Last state of rotary encoder CLK signal
unsigned long lastDebounceTime = 0;             // Last time a button was toggled
const unsigned long debounceDelay = 50;         // Debounce delay time in milliseconds

// Button Release Guard
bool buttonReleaseGuard = false;                // Guard flag to block button repeats
unsigned long buttonReleaseTime = 0;             // Last button release timestamp
const unsigned long buttonReleaseDelay = 200;    // Button release delay in milliseconds

int buttonState = 0;                            // State of general-purpose button
int eButtonState = 0;                           // State of emergency button

void setup() {
  Serial.begin(115200);                         // Start serial communication

  lcd.begin(lcdColumns, lcdRows);               // Initialize LCD
  lcd.setBacklight(255);                        // Turn on LCD backlight at full brightness
  displayMenu();                                // Display initial menu

  pinMode(encoderCLK, INPUT);                   // Set encoder CLK as input
  pinMode(encoderDT, INPUT);                    // Set encoder DT as input
  pinMode(rotaryButton, INPUT_PULLUP);           // Set rotary button with pull-up resistor
  pinMode(Button, INPUT_PULLUP);                 // Set general button with pull-up resistor
  pinMode(emergencyButton, INPUT_PULLUP);        // Set emergency button with pull-up resistor
  pinMode(clockwiseButton, INPUT_PULLUP);        // Set clockwise button with pull-up resistor
  pinMode(counterclockwiseButton, INPUT_PULLUP); // Set counterclockwise button with pull-up resistor

  treadmill.setMaxSpeed(maxSpeed);               // Set treadmill max speed
  treadmill.setAcceleration(2000);               // Set treadmill acceleration

  auger.setMaxSpeed(augerMaxSpeed);               // Set auger 1 max speed
  auger.setAcceleration(2000);                   // Set auger 1 acceleration

  auger2.setMaxSpeed(augerMaxSpeed);              // Set auger 2 max speed
  auger2.setAcceleration(2000);                  // Set auger 2 acceleration

  harness.setMaxSpeed(800);                      // Set harness motor max speed
  harness.setAcceleration(300);                  // Set harness motor acceleration
  harness.setSpeed(0);                           // Set harness motor speed to 0

  lastCLKState = digitalRead(encoderCLK);         // Read initial state of encoder CLK
}

void loop() {
  static Mode previousMode = MENU;               // Store previous mode

  if (currentMode == MENU) {
    if (buttonReleaseGuard && millis() - buttonReleaseTime > buttonReleaseDelay) {
      buttonReleaseGuard = false;                // Reset guard after delay
    }

    if (!buttonReleaseGuard) {
      if (previousMode != MENU) {
        lcd.clear();                             // Clear LCD if mode changed
        displayMenu();                           // Re-display menu
        previousMode = MENU;                     // Update previous mode
      }
      handleEncoderForMenu();                    // Check rotary encoder for menu
      if (buttonPressed(rotaryButton)) handleMenuSelection(); // Handle selection if button pressed
    }

    if (treadmillSpeed > 0) {
      treadmill.runSpeed();                      // Run treadmill motor
      auger.runSpeed();                           // Run auger motor 1
      auger2.runSpeed();                          // Run auger motor 2
    }
  } else if (currentMode == SPEED_MODE) {
    handleSpeedMode();                           // Handle speed adjustments
    if (treadmillSpeed && augerSpeed == 0) {
      if (buttonPressed(rotaryButton)) returnToMenu(); // Return if button pressed
    }
  } else if (currentMode == HARNESS_MODE) {
    handleHarnessMode();                         // Handle harness motor control
    if (buttonPressed(rotaryButton)) returnToMenu(); // Return if button pressed
  }
}

void handleEncoderForMenu() {
  int currentCLKState = digitalRead(encoderCLK);  // Read current CLK state
  if ((millis() - lastDebounceTime) > debounceDelay && currentCLKState != lastCLKState) {
    if (digitalRead(encoderDT) != currentCLKState) {
      menuIndex++;                               // Scroll menu forward
    } else {
      menuIndex--;                               // Scroll menu backward
    }
    menuIndex = constrain(menuIndex, 0, numMenuItems - 1); // Constrain within bounds
    displayMenu();                               // Update LCD menu display
    lastCLKState = currentCLKState;               // Save current state
    lastDebounceTime = millis();                  // Update debounce timer
  }
}

void handleMenuSelection() {
  lcd.clear();                                   // Clear LCD
  lcd.print(menuItems[menuIndex] + " SELECTED"); // Show selection
  Serial.println(menuItems[menuIndex] + " SELECTED"); // Print to serial
  delay(1000);                                   // Pause

  if (menuIndex == 0) {
    currentMode = SPEED_MODE;                    // Switch to Speed mode
    lcd.clear();
    lcd.print("Speed Mode");                     // Display "Speed Mode"
    treadmill.setSpeed(treadmillSpeed);           // Set treadmill speed
  } else if (menuIndex == 1) {
    currentMode = HARNESS_MODE;                  // Switch to Harness mode
    lcd.clear();
    lcd.print("Harness Mode");                   // Display "Harness Mode"
  }
}

void displayMenu() {
  lcd.clear();                                   // Clear LCD
  lcd.setCursor(0, 0);
  lcd.print("MENU");                             // Print "MENU"
  lcd.setCursor(0, 1);
  lcd.print(menuIndex == 0 ? "-> SPEED" : "-> HARNESS"); // Show selected menu item
}

bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW && (millis() - lastDebounceTime) > debounceDelay) {
    lastDebounceTime = millis();                  // Update debounce
    return true;                                  // Return true if button pressed
  }
  return false;                                   // Else return false
}

void handleSpeedMode() {
  static unsigned long lastEncoderMoveTime = 0;   // Last encoder move time
  static bool speedDisplayed = false;             // Track if speed shown
  const unsigned long displayDelay = 1000;        // Delay before showing speed
  float linearSpeed;                              // Store calculated linear speed
  int currentCLKState = digitalRead(encoderCLK);  // Read current CLK state

  if (currentCLKState != lastCLKState) {
    if (digitalRead(encoderDT) != currentCLKState) {
      treadmillSpeed += treadmillIncrement;       // Increase treadmill speed
      augerSpeed += augerIncrement;                // Increase auger speed
    } else {
      treadmillSpeed -= treadmillIncrement;       // Decrease treadmill speed
      augerSpeed -= augerIncrement;                // Decrease auger speed
    }

    treadmillSpeed = constrain(treadmillSpeed, 0, maxSpeed); // Constrain speed
    augerSpeed = constrain(augerSpeed, 0, augerMaxSpeed);     // Constrain speed

    treadmill.setSpeed(-treadmillSpeed);           // Update treadmill speed (negative direction)
    auger.setSpeed(augerSpeed);                    // Update auger 1 speed
    auger2.setSpeed(augerSpeed);                   // Update auger 2 speed

    // Calculate RPM and linear speed
    float rpm = (abs(treadmillSpeed) * 60.0) / 400.0;
    linearSpeed = (rpm * 0.0762 * PI) / 60.0;      // Convert to m/s
    Serial.println(linearSpeed);                   // Print speed

    lastCLKState = currentCLKState;
    lastEncoderMoveTime = millis();
    speedDisplayed = false;
  }

  treadmill.runSpeed();                            // Run treadmill motor
  auger.runSpeed();                                // Run auger 1 motor
  auger2.runSpeed();                               // Run auger 2 motor

  eButtonState = digitalRead(emergencyButton);     // Read emergency button

  if (eButtonState == HIGH) {
    eButtonPressed = false;                        // Reset emergency pressed flag
  }
  if (eButtonState == HIGH) {
    eStopProcedure();                              // Start emergency procedure
  }

  if ((millis() - lastEncoderMoveTime > displayDelay) && !speedDisplayed) {
    lcd.setCursor(0, 1);
    lcd.print("Speed: ");
    lcd.print(linearSpeed, 2);                     // Display speed on LCD
    lcd.print(" m/s     ");
    speedDisplayed = true;
  }
  if (treadmillSpeed == 0 && augerSpeed == 0) {
    if (buttonPressed(rotaryButton)) returnToMenu();
  }
}

void handleHarnessMode() {
  eButtonState = digitalRead(emergencyButton); // Read emergency button state
  if (eButtonState == HIGH) eButtonPressed = false; // Reset emergency button flag if pressed

  if (digitalRead(clockwiseButton) == LOW) { // If clockwise button is pressed
    harness.setSpeed(200); // Set harness motor speed positive
    harness.runSpeed(); // Run harness motor
  } else if (digitalRead(counterclockwiseButton) == LOW) { // If counterclockwise button is pressed
    harness.setSpeed(-200); // Set harness motor speed negative
    harness.runSpeed(); // Run harness motor
  } else {
    harness.setSpeed(0); // Stop harness motor if no button is pressed
  }

  harness.runSpeed(); // Continue running harness motor
}

void eStopProcedure() {
    eButtonPressed = true; // Set emergency button flag true

    // Start harness movement without blocking execution
    harness.setMaxSpeed(800); // Set harness max speed
    harness.setAcceleration(300); // Set harness acceleration

    if (positionSaved) { // If a saved position exists
        harness.moveTo(setPosition); // Move harness to saved position
        harness.runToPosition(); // Block until position reached
    } else {
        harness.moveTo(harness.currentPosition() - 200); // Move harness backwards by 200 steps
    }

    // Keep treadmill & augers running during harness movement
    treadmill.setAcceleration(300); // Set treadmill acceleration
    auger.setAcceleration(300); // Set auger 1 acceleration
    auger2.setAcceleration(300); // Set auger 2 acceleration

    int currentTreadmillSpeed = treadmillSpeed; // Save current treadmill speed
    int currentAugerSpeed = augerSpeed; // Save current auger 1 speed
    int currentAuger2Speed = augerSpeed; // Save current auger 2 speed

    treadmill.setSpeed(-currentTreadmillSpeed); // Set treadmill speed negative
    auger.setSpeed(currentAugerSpeed); // Set auger 1 speed
    auger2.setSpeed(currentAuger2Speed); // Set auger 2 speed

    // Timing-based gradual deceleration while stepper is moving
    unsigned long lastUpdate = millis(); // Record current time
    int decelStep = 10; // Treadmill deceleration step size
    int updateInterval = 30; // Time interval between speed updates
    int augDec = 20; // Auger deceleration step size

    while ((currentTreadmillSpeed > 0 || currentAugerSpeed > 0 || currentAuger2Speed > 0) || harness.distanceToGo() != 0) {
        if (millis() - lastUpdate >= updateInterval) { // Check if time to update
            lastUpdate = millis(); // Update last time marker
            
            if (currentTreadmillSpeed > 0) { // If treadmill still moving
                currentTreadmillSpeed = max(0, currentTreadmillSpeed - decelStep); // Decrease treadmill speed
                treadmill.setSpeed(-currentTreadmillSpeed); // Update treadmill speed
            }
            if (currentAugerSpeed > 0) { // If auger 1 still moving
                currentAugerSpeed = max(0, currentAugerSpeed - augDec); // Decrease auger 1 speed
                auger.setSpeed(currentAugerSpeed); // Update auger 1 speed
            }
            if (currentAuger2Speed > 0) { // If auger 2 still moving
                currentAuger2Speed = max(0, currentAuger2Speed - augDec); // Decrease auger 2 speed
                auger2.setSpeed(currentAuger2Speed); // Update auger 2 speed
            }
        }

        // Run all motors while decelerating
        treadmill.runSpeed(); // Run treadmill
        auger.runSpeed(); // Run auger 1
        auger2.runSpeed(); // Run auger 2
        harness.run(); // Run harness motor (non-blocking)
    }

    // Stop treadmill and augers completely
    treadmill.setSpeed(0); // Set treadmill speed to 0
    auger.setSpeed(0); // Set auger 1 speed to 0
    auger2.setSpeed(0); // Set auger 2 speed to 0
    treadmill.runSpeed(); // Run treadmill
    auger.runSpeed(); // Run auger 1
    auger2.runSpeed(); // Run auger 2

    harness.moveTo(harness.currentPosition() + 200); // Move harness forward 200 steps
    while (harness.distanceToGo() != 0) { // Wait until movement done
        harness.run(); // Run harness
    }

    // System shutdown message
    Serial.println("EMERGENCY STOP COMPLETE - ENTERING LOW POWER MODE"); // Print shutdown message
    lcd.clear(); // Clear LCD
    lcd.setCursor(0, 0); // Set cursor at first row
    lcd.print("SYSTEM SHUTDOWN"); // Display "System Shutdown"
    lcd.setCursor(0, 1); // Set cursor at second row
    lcd.print("RESTART REQUIRED"); // Display "Restart Required"
    
    delay(5000); // Wait 5 seconds
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // Configure ESP32 to wake up on GPIO0 LOW
    esp_deep_sleep_start(); // Enter deep sleep mode
}

void returnToMenu() {
  currentMode = MENU; // Switch mode back to MENU
  buttonReleaseGuard = true; // Activate button release guard
  buttonReleaseTime = millis(); // Save current time for guard
  positionSaved = false; // Clear saved position flag
  eButtonPressed = false; // Clear emergency button flag
  lcd.clear(); // Clear LCD
  displayMenu(); // Display main menu
  Serial.println("Returned to MENU"); // Print return message
}

