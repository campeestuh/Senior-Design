#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>

// LCD settings
const int lcdColumns = 16, lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// Rotary Encoder and Button pins
const int encoderCLK = 4, encoderDT = 5, rotaryButton = 3, Button = 6;  // Rotary encoder and button pins
const int emergencyButton = 2;                                         // Emergency button pin

// Variables for rotary encoder and motor control
int lastCLKState;               // Tracks the last state of the rotary encoder CLK pin
unsigned long lastMoveTime = 0; // To track the last movement time
unsigned long debounceDelay = 10; // Debounce delay for encoder
unsigned long lastDebounceTime = 0; // Last debounce time
int count = 0;                  // Encoder count
int motorDirection = 0;         // Motor direction (-1: counter-clockwise, 1: clockwise, 0: stop)
const int motorSpeed = 300;     // Fixed speed for the motor

// Menu settings
int menuIndex = 0;
const int numMenuItems = 2;
String menuItems[numMenuItems] = {"SPEED", "HARNESS"};  // Menu items

// Program modes
enum Mode { MENU, HARNESS_MODE };
Mode currentMode = MENU;

// Stepper Motor Settings
AccelStepper stepper(1, 8, 9);  // Stepper motor: Pin 8 -> CLK, Pin 9 -> Direction
int setPosition = 0;            // Placeholder for saved position
bool positionSaved = false;     // Flag for saved position
bool eButtonPressed = false;    // Tracks emergency button state

void setup() {
  Serial.begin(9600);  
  lcd.init();
  lcd.backlight();
  bootMessage();  // Display boot message

  // Pin setup for rotary encoder and buttons
  pinMode(encoderCLK, INPUT);
  pinMode(encoderDT, INPUT);
  pinMode(rotaryButton, INPUT_PULLUP);  // Enable internal pull-up resistor for SW pin
  pinMode(Button, INPUT_PULLUP);
  pinMode(emergencyButton, INPUT_PULLUP);

  // Initialize rotary encoder
  lastCLKState = digitalRead(encoderCLK);

  // Stepper motor setup
  stepper.setMaxSpeed(motorSpeed);  // Fixed motor speed
  stepper.setAcceleration(100);    // Smooth acceleration
  stepper.setSpeed(0);              // Initially, set the motor speed to 0

  displayMenu();  // Display the initial menu
}

void loop() {
  if (currentMode == MENU) {
    handleEncoderForMenu();
    if (digitalRead(rotaryButton) == LOW) {  // Handle rotary button press
      delay(200);  // Debounce delay
      handleMenuSelection();
    }
  }
  
  if (currentMode == HARNESS_MODE) {
    handleHarnessMode();
    if (digitalRead(rotaryButton) == LOW) {  // Check rotary button in Harness Mode
      delay(200);  // Debounce delay
      returnToMenu();  // Return to menu when rotary button is pressed
    }
  }
}

// Function to handle rotary encoder for menu navigation
void handleEncoderForMenu() {
  int currentCLKState = digitalRead(encoderCLK);

  if (currentCLKState != lastCLKState) {  // Detect rotation
    if (digitalRead(encoderDT) != currentCLKState) {
      menuIndex++;  // Clockwise rotation
    } else {
      menuIndex--;  // Counter-clockwise rotation
    }

    menuIndex = constrain(menuIndex, 0, numMenuItems - 1);  // Limit the menu index
    displayMenu();

    lastCLKState = currentCLKState;  // Update last CLK state
  }
}

// Function to handle menu item selection
void handleMenuSelection() {
  lcd.clear();
  lcd.print(menuItems[menuIndex] + " selected");
  Serial.println(menuItems[menuIndex] + " selected");
  delay(1000);

  if (menuItems[menuIndex] == "HARNESS") {
    currentMode = HARNESS_MODE;  // Enter "Harness" mode
    lcd.clear();
    lcd.print("HARNESS MODE");
    delay(1000);

    // Reset motor and encoder state
    motorDirection = 0;
    stepper.setSpeed(0);  // Ensure motor is stopped
    count = 0;  // Reset encoder count
    lastCLKState = digitalRead(encoderCLK);  // Synchronize encoder state
    lastMoveTime = millis();  // Reset idle timer
  } else {
    displayMenu();
  }
}

// Function to handle "Harness" mode logic
void handleHarnessMode() {
  int currentCLKState = digitalRead(encoderCLK);

  // Debounce the rotary encoder inputs
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (currentCLKState != lastCLKState) {  // Detect rotation
      if (digitalRead(encoderDT) == currentCLKState) {
        motorDirection = 1;  // Clockwise rotation
      } else {
        motorDirection = -1; // Counter-clockwise rotation
      }

      lastCLKState = currentCLKState;  // Update last CLK state
      lastMoveTime = millis();  // Reset idle timer

      Serial.print("Motor Direction: ");
      Serial.println(motorDirection);
    }
    lastDebounceTime = millis();
  }

  // Update motor speed based on direction
  if (motorDirection == 1) {
    stepper.setSpeed(motorSpeed);  // Clockwise at fixed speed
  } else if (motorDirection == -1) {
    stepper.setSpeed(-motorSpeed);  // Counter-clockwise at fixed speed
  } else {
    stepper.setSpeed(0);  // Stop motor
  }

  // Stop the motor if idle for 100ms
  if (millis() - lastMoveTime > 100) {
    motorDirection = 0;  // Stop motor
    stepper.setSpeed(0);
  }

  stepper.runSpeed();  // Continuously run the motor at the set speed

  // Save Position
  if (digitalRead(Button) == LOW) {
    delay(200);  // Debounce delay
    setPosition = stepper.currentPosition();  // Save the current position
    positionSaved = true;
    Serial.println("Position Saved");
  }

  // Emergency Button Logic
  if (digitalRead(emergencyButton) == LOW && !eButtonPressed) {
    eButtonPressed = true;

    if (positionSaved) {
      Serial.println("Returning to Saved Position");
      stepper.moveTo(setPosition);
      stepper.runToPosition();
    } else {
      Serial.println("Ascending Off Platform");
      int ascendPosition = stepper.currentPosition() - 500;
      stepper.moveTo(ascendPosition);
      stepper.runToPosition();
    }
  }

  if (digitalRead(emergencyButton) == HIGH) {
    eButtonPressed = false;  // Reset emergency button state
  }
}

// Function to return to menu
void returnToMenu() {
  currentMode = MENU;  // Switch back to MENU mode
  motorDirection = 0;  // Stop motor
  stepper.setSpeed(0);  // Ensure motor stops
  displayMenu();  // Refresh the menu display
  Serial.println("Returned to MENU");
}

// Boot message function
void bootMessage() {
  lcd.print("WELCOME");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("RATSTRONAUTS!");
  delay(1000);
  lcd.clear(); 
}

// Function to display the menu
void displayMenu() {
  lcd.clear();
  lcd.print("MENU");
  lcd.setCursor(0, 1);
  lcd.print(menuIndex == 0 ? "-> SPEED" : "-> HARNESS");
}
