// Authors: Juan Campista & Alicia Enriquez     
// Date:    4/23/2024
// Summary: This code implements a rotary encoder-controlled user interface for a treadmill system
//          with two modes: speed control for a treadmill motor and directional control for a harness
//          motor. It displays a menu on an I2C LCD, handles rotary input for navigation and selection,
//          and adjusts motor behavior based on user interaction. Emergency stop and directional
//          control buttons are also integrated for safety and manual overrides.
//---------------------------------------------------------------------------------------------------------//
#include <LiquidCrystal_PCF8574.h>          // Include the library for controlling an I2C LCD
#include <AccelStepper.h>                   // Include the library for controlling stepper motors
#include <esp_sleep.h>                      // Include the library for ESP32 sleep functions

// Pin Definitions
#define encoderCLK 4                        // Rotary encoder CLK pin
#define encoderDT 5                         // Rotary encoder DT pin
#define rotaryButton 18                     // Rotary encoder push button pin

#define treadDirectionPin 25                // Direction control pin for treadmill motor
#define treadPulsePin 27                    // Step (pulse) control pin for treadmill motor

#define harnessPulsePin 15                  // Step (pulse) control pin for harness motor
#define harnessDirectionPin 16             // Direction control pin for harness motor

#define Button 29                           // General-purpose button (not yet used)
#define emergencyButton 26                  // Emergency stop button pin
#define clockwiseButton 17                  // Button to move harness clockwise
#define counterclockwiseButton 23          // Button to move harness counterclockwise

// LCD Settings
const int lcdColumns = 16, lcdRows = 2;     // LCD dimensions (16 columns, 2 rows)
LiquidCrystal_PCF8574 lcd(0x27);           // Create LCD object with I2C address 0x27

// Treadmill Settings
AccelStepper treadmill(1, treadPulsePin, treadDirectionPin); // Create treadmill stepper object
int treadmillSpeed = 0;                     // Current speed of treadmill
const int maxSpeed = 1337;                  // Maximum speed for treadmill
const int treadmillIncrement = 17;          // Increment/decrement value for treadmill speed

// Harness Motor Settings
AccelStepper harness(1, harnessPulsePin, harnessDirectionPin); // Create harness stepper object
bool positionSaved = false;                // Flag for saved harness position
int setPosition = 0;                        // Target position for harness (not used in this version)
bool eButtonPressed = false;               // Flag for emergency button state
unsigned long lastMoveTime = 0;            // Last time harness moved (not used in this version)
int count = 0;                              // Counter for rotary changes (not used in this version)

// Menu Variables
enum Mode { MENU, SPEED_MODE, HARNESS_MODE } currentMode = MENU; // Mode state
const String menuItems[] = {"SPEED", "HARNESS"}; // Menu options
int menuIndex = 0;                          // Current index in menu
const int numMenuItems = 2;                // Total number of menu items

// Rotary Encoder State
int lastCLKState = LOW;                    // Last known state of encoder CLK
unsigned long lastDebounceTime = 0;        // Last debounce time
const unsigned long debounceDelay = 50;    // Debounce delay in milliseconds

// Button Release Guard
bool buttonReleaseGuard = false;           // Prevents button re-triggering
unsigned long buttonReleaseTime = 0;       // Time when button was released
const unsigned long buttonReleaseDelay = 200; // Delay before button can trigger again

int buttonState = 0;                        // Current button state (not used in this version)
int eButtonState = 0;                       // Current emergency button state

void setup() {
  Serial.begin(115200);                     // Initialize serial communication

  lcd.begin(lcdColumns, lcdRows);          // Initialize the LCD
  lcd.setBacklight(255);                   // Set LCD backlight to max
  displayMenu();                            // Show the main menu on LCD

  pinMode(encoderCLK, INPUT);              // Set encoder CLK pin as input
  pinMode(encoderDT, INPUT);               // Set encoder DT pin as input
  pinMode(rotaryButton, INPUT_PULLUP);     // Set rotary button pin as input with pull-up
  pinMode(Button, INPUT_PULLUP);           // Set general button pin as input with pull-up
  pinMode(emergencyButton, INPUT_PULLUP);  // Set emergency button as input with pull-up
  pinMode(clockwiseButton, INPUT_PULLUP);  // Set clockwise button as input with pull-up
  pinMode(counterclockwiseButton, INPUT_PULLUP); // Set counterclockwise button as input with pull-up

  treadmill.setMaxSpeed(maxSpeed);         // Set maximum treadmill speed
  treadmill.setAcceleration(2000);         // Set treadmill acceleration

  harness.setMaxSpeed(800);                // Set maximum harness speed
  harness.setAcceleration(300);            // Set harness acceleration
  harness.setSpeed(0);                     // Initialize harness speed to 0

  lastCLKState = digitalRead(encoderCLK);  // Save initial encoder CLK state
}

void loop() {
  static Mode previousMode = MENU;         // Store the previous mode for LCD updates

  if (currentMode == MENU) {               // If in menu mode
    if (buttonReleaseGuard && millis() - buttonReleaseTime > buttonReleaseDelay) {
      buttonReleaseGuard = false;          // Reset button guard after delay
    }

    if (!buttonReleaseGuard) {
      if (previousMode != MENU) {          // If mode changed to menu
        lcd.clear();                       // Clear the LCD
        displayMenu();                     // Display the menu
        previousMode = MENU;               // Update previous mode
      }
      handleEncoderForMenu();              // Handle rotary input for menu
      if (buttonPressed(rotaryButton)) handleMenuSelection(); // If button pressed, select menu item
    }

    if (treadmillSpeed > 0) {              // If treadmill is running
      treadmill.runSpeed();                // Continue running treadmill
    }
  } else if (currentMode == SPEED_MODE) {  // If in speed adjustment mode
    handleSpeedMode();                     // Adjust speed based on encoder
    if (treadmillSpeed == 0) {
      if (buttonPressed(rotaryButton)) returnToMenu(); // Return to menu if stopped
    }
  } else if (currentMode == HARNESS_MODE) {// If in harness control mode
    handleHarnessMode();                   // Run harness motor
    if (buttonPressed(rotaryButton)) returnToMenu(); // Return to menu on button press
  }
}

void handleEncoderForMenu() {
  int currentCLKState = digitalRead(encoderCLK); // Read encoder CLK
  if ((millis() - lastDebounceTime) > debounceDelay && currentCLKState != lastCLKState) {
    if (digitalRead(encoderDT) != currentCLKState) {
      menuIndex++;                         // Rotate clockwise: increment index
    } else {
      menuIndex--;                         // Rotate counterclockwise: decrement index
    }
    menuIndex = constrain(menuIndex, 0, numMenuItems - 1); // Keep index in bounds
    displayMenu();                         // Update menu display
    lastCLKState = currentCLKState;        // Update last CLK state
    lastDebounceTime = millis();           // Update debounce time
  }
}

void handleMenuSelection() {
  lcd.clear();                             // Clear LCD for new mode
  lcd.print(menuItems[menuIndex] + " SELECTED"); // Show selected item
  Serial.println(menuItems[menuIndex] + " SELECTED"); // Print to serial
  delay(1000);                             // Pause for 1 second

  if (menuIndex == 0) {                    // If SPEED selected
    currentMode = SPEED_MODE;              // Change mode to speed
    lcd.clear(); lcd.print("Speed Mode");  // Display mode
    treadmill.setSpeed(treadmillSpeed);    // Set treadmill speed
  } else if (menuIndex == 1) {             // If HARNESS selected
    currentMode = HARNESS_MODE;            // Change mode to harness
    lcd.clear(); lcd.print("Harness Mode");// Display mode
  }
}

void displayMenu() {
  lcd.clear();                             // Clear LCD
  lcd.setCursor(0, 0);                     // Set cursor to top left
  lcd.print("MENU");                       // Print "MENU" header
  lcd.setCursor(0, 1);                     // Move to second line
  lcd.print(menuIndex == 0 ? "-> SPEED" : "-> HARNESS"); // Show selected menu item
}

bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW && (millis() - lastDebounceTime) > debounceDelay) {
    lastDebounceTime = millis();           // Update debounce timer
    return true;                           // Return true if button is pressed
  }
  return false;                            // Otherwise return false
}

void handleSpeedMode() {
  static unsigned long lastEncoderMoveTime = 0;       // Time of last encoder input
  static bool speedDisplayed = false;                 // Tracks if speed was already shown
  const unsigned long displayDelay = 1000;            // Wait time before showing speed
  const float speedStep = 0.05;
  float linearSpeed;
  int currentCLKState = digitalRead(encoderCLK); // Read encoder CLK

  if (currentCLKState != lastCLKState) {
    if (digitalRead(encoderDT) != currentCLKState) {
      treadmillSpeed += treadmillIncrement; // Increase speed
      // displaySpeed += speedStep;
    } else {
      treadmillSpeed -= treadmillIncrement; // Decrease speed
      // displaySpeed -= speedStep;
    }

    treadmillSpeed = constrain(treadmillSpeed, 0, maxSpeed); // Gauge speed value
    treadmill.setSpeed(-treadmillSpeed);    // Set treadmill speed (negative for direction)
    // displaySpeed = constrain(displaySpeed, 0, 0.80);

    // Calculate RPM and linear speed
     float rpm = (abs(treadmillSpeed) * 60.0) / 400.0; // Assuming 400 steps per revolution
     linearSpeed = (rpm * 0.0762 * PI) / 60.0;   // Convert RPM to m/s
    Serial.println(linearSpeed);

    lastCLKState = currentCLKState;         // Update CLK state
    lastEncoderMoveTime = millis();                   // Reset last movement time
    speedDisplayed = false;                           // Reset display flag
  }

  treadmill.runSpeed();                     // Run treadmill

    eButtonState = digitalRead(emergencyButton);

  if (eButtonState == HIGH) {
    eButtonPressed = false;
  }
    if (eButtonState == HIGH ) {
    eStopProcedure();
    }

  if ((millis() - lastEncoderMoveTime > displayDelay) && !speedDisplayed) {
    lcd.setCursor(0, 1);                              // Second row of LCD
    lcd.print("Speed: ");
    lcd.print(linearSpeed, 2);            // Display speed in user units
    lcd.print(" m/s     ");                           // Fill/clear line
    speedDisplayed = true;                            // Prevent re-print
  }
  if (treadmillSpeed == 0) {
    if (buttonPressed(rotaryButton)) returnToMenu(); // Return to menu if button pressed
  }
}

void eStopProcedure() {
    eButtonPressed = true; // Set emergency stop flag

    // Start harness movement *without blocking execution*
    harness.setMaxSpeed(800);               // Set maximum speed for harness motor
    harness.setAcceleration(300);           // Set acceleration for harness motor

    harness.moveTo(harness.currentPosition() - 200); // Move harness 200 steps backward

    // Keep treadmill & auger running during stepper movement
    treadmill.setAcceleration(300);         // Set treadmill acceleration

    int currentTreadmillSpeed = treadmillSpeed; // Store current treadmill speed locally

    treadmill.setSpeed(-currentTreadmillSpeed); // Start treadmill in reverse at current speed

    // Timing-based gradual deceleration while stepper is moving
    unsigned long lastUpdate = millis();    // Record the current time
    int decelStep = 10;                     // Amount to decrease speed per step
    int updateInterval = 30;                // Time interval between deceleration steps (ms)
    int augDec = 20;                        // Unused variable for auger deceleration

    // While treadmill is still running or harness hasn't finished moving
    while ((currentTreadmillSpeed > 0 ) || harness.distanceToGo() != 0) {
        if (millis() - lastUpdate >= updateInterval) { // Time to update?
            lastUpdate = millis();                    // Reset last update time
            
            if (currentTreadmillSpeed > 0) {          // If treadmill is still running
                currentTreadmillSpeed = max(0, currentTreadmillSpeed - decelStep); // Gradually slow down
                treadmill.setSpeed(-currentTreadmillSpeed); // Apply new speed
            }
        }

        // Keep all motors running while decelerating & moving stepper
        treadmill.runSpeed();         // Keep treadmill stepping
        harness.run();                // Keep harness stepping (non-blocking)
    }

    // Ensure all motors fully stop
    treadmill.setSpeed(0);            // Stop treadmill
    treadmill.runSpeed();             // Apply 0 speed to stop stepper motion

    harness.moveTo(harness.currentPosition() + 200); // Return harness to original position
    while (harness.distanceToGo() != 0) {            // Wait until return move is complete
        harness.run();                               // Continue moving harness
    }

    Serial.println("EMERGENCY STOP COMPLETE - ENTERING LOW POWER MODE"); // Notify over serial

    lcd.clear();                      // Clear LCD screen
    lcd.setCursor(0, 0);              // Set LCD cursor to first row
    lcd.print("SYSTEM SHUTDOWN");    // Display shutdown message
    lcd.setCursor(0, 1);              // Set LCD cursor to second row
    lcd.print("RESTART REQUIRED");   // Display restart notice

    delay(5000);                      // Pause 5 seconds before sleep
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // Enable external wakeup on GPIO 0
    esp_deep_sleep_start();          // Enter deep sleep mode
}

void handleHarnessMode() {
  eButtonState = digitalRead(emergencyButton); // Read emergency button
  if (eButtonState == HIGH) eButtonPressed = false; // Reset emergency flag if not pressed

  if (digitalRead(clockwiseButton) == LOW) {
    harness.setSpeed(200);                // Set harness speed CW
    harness.runSpeed();                   // Run harness motor
  } else if (digitalRead(counterclockwiseButton) == LOW) {
    harness.setSpeed(-200);               // Set harness speed CCW
    harness.runSpeed();                   // Run harness motor
  } else {
    harness.setSpeed(0);                  // Stop harness motor
  }

  harness.runSpeed();                     // Ensure harness motor runs at current speed
}

void returnToMenu() {
  currentMode = MENU;                     // Switch mode to menu
  buttonReleaseGuard = true;              // Enable guard to prevent immediate reentry
  buttonReleaseTime = millis();           // Save time of return
  positionSaved = false;                  // Reset saved position flag
  eButtonPressed = false;                 // Reset emergency button flag
  lcd.clear();                            // Clear LCD
  displayMenu();                          // Show menu
  Serial.println("Returned to MENU");     // Print to serial
}
