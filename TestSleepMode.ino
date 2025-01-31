#include <LiquidCrystal_PCF8574.h>
#include <AccelStepper.h>
#include "esp_sleep.h"  // ESP32 Deep Sleep Library

// Pin Definitions
#define encoderCLK 4
#define encoderDT 5
#define rotaryButton 18
#define treadDirectionPin 25
#define treadPulsePin 27
#define augerDirectionPin 14
#define augerPulsePin 13
#define harnessDirectionPin 16
#define harnessPulsePin 15
#define Button 19
#define emergencyButton 26
#define clockwiseButton 23
#define counterclockwiseButton 17

// LCD Settings
const int lcdColumns = 16, lcdRows = 2;
LiquidCrystal_PCF8574 lcd(0x27);

// Treadmill Settings
AccelStepper treadmill(1, treadPulsePin, treadDirectionPin);
int treadmillSpeed = 0;                   
const int maxSpeed = 1337;                
const int treadmillIncrement = 17;        

// Auger Settings
AccelStepper auger(1, augerPulsePin, augerDirectionPin);
int augerSpeed = 0;                       
const int augerMaxSpeed = 2044;           
const int augerIncrement = 26;            

// Harness Motor Settings
AccelStepper stepper(1, harnessPulsePin, harnessDirectionPin); 
bool positionSaved = false;
int setPosition = 0;
bool eButtonPressed = false;
unsigned long lastMoveTime = 0;  
int count = 0;             

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
const unsigned long buttonReleaseDelay = 200; 

// Function Declarations
void displayMenu();
void handleEncoderForMenu();
bool buttonPressed(int pin);
void handleMenuSelection();
void returnToMenu();
void handleHarnessMode();  // <=== Now declared
void handleSpeedMode();
void eStopProcedure();

void setup() {
  Serial.begin(115200);
  
  // Initialize LCD
  lcd.begin(lcdColumns, lcdRows);
  lcd.setBacklight(255);
  displayMenu();

  // Initialize Pins
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
  stepper.setMaxSpeed(800);         
  stepper.setAcceleration(300);    
  stepper.setSpeed(0);             

  lastCLKState = digitalRead(encoderCLK);
}

void loop() {
  static Mode previousMode = MENU;

  // === GLOBAL EMERGENCY STOP CHECK ===
  if (digitalRead(emergencyButton) == LOW && !eButtonPressed) {
    eButtonPressed = true;
    Serial.println("Emergency Stop Activated!");
    eStopProcedure();
  }
  if (digitalRead(emergencyButton) == HIGH) {
    eButtonPressed = false; 
  }

  if (currentMode == MENU) {
    if (buttonReleaseGuard && millis() - buttonReleaseTime > buttonReleaseDelay) {
      buttonReleaseGuard = false; 
    }

    if (!buttonReleaseGuard) {
      if (previousMode != MENU) {
        lcd.clear();
        displayMenu();
        previousMode = MENU; 
      }
      handleEncoderForMenu();
      if (buttonPressed(rotaryButton)) handleMenuSelection();
    }

    if (treadmillSpeed > 0) {
      treadmill.runSpeed(); 
      auger.runSpeed();
    }
  } else if (currentMode == SPEED_MODE) {
    handleSpeedMode();
    if (buttonPressed(rotaryButton)) returnToMenu();
  } else if (currentMode == HARNESS_MODE) {
    handleHarnessMode();  // <=== Now correctly calls the function
    if (buttonPressed(rotaryButton)) returnToMenu();
  }
}

// === eStopProcedure() WITH LOW POWER MODE ===
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
    delay(200);
  }

  Serial.println("EMERGENCY STOP COMPLETE - ENTERING LOW POWER MODE");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SYSTEM SHUTDOWN");
  lcd.setCursor(0, 1);
  lcd.print("Restart Required");

  // Wait 5 seconds before shutting down
  delay(5000);

  // Put ESP32 into deep sleep mode (LOW POWER MODE)
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // Only reset will wake it up
  esp_deep_sleep_start();
}

// === HANDLE HARNESS MODE (NOW ADDED) ===
void handleHarnessMode() {
  if (buttonPressed(Button)) {
    Serial.println("Position Saved");
    setPosition = stepper.currentPosition();
    positionSaved = true;
  }

  if (digitalRead(clockwiseButton) == LOW) {
    stepper.setSpeed(200);
  } else if (digitalRead(counterclockwiseButton) == LOW) {
    stepper.setSpeed(-200);
  } else {
    stepper.setSpeed(0);
  }

  stepper.runSpeed();
}

// === SPEED MODE FIX: STOP MOTORS ON EMERGENCY ===
void handleSpeedMode() {
  if (eButtonPressed) {
    treadmillSpeed = 0;
    augerSpeed = 0;
    treadmill.setSpeed(0);
    auger.setSpeed(0);
    Serial.println("Emergency Stop in Speed Mode - Motors Stopped!");
    return;
  }

  treadmill.runSpeed();
  auger.runSpeed();
}

// === MENU FUNCTIONS ===
void displayMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MENU");
  lcd.setCursor(0, 1);
  lcd.print(menuIndex == 0 ? "-> SPEED" : "-> HARNESS");
}

void handleEncoderForMenu() {
  int currentCLKState = digitalRead(encoderCLK);
  if ((millis() - lastDebounceTime) > debounceDelay && currentCLKState != lastCLKState) {
    if (digitalRead(encoderDT) != currentCLKState) {
      menuIndex++; 
    } else {
      menuIndex--; 
    }
    menuIndex = constrain(menuIndex, 0, numMenuItems - 1);
    displayMenu();
    lastCLKState = currentCLKState;
    lastDebounceTime = millis();
  }
}

void handleMenuSelection() {
  lcd.clear();
  lcd.print(menuItems[menuIndex] + " SELECTED");
  Serial.println(menuItems[menuIndex] + " SELECTED");
  delay(1000); 

  if (menuIndex == 0) {
    currentMode = SPEED_MODE;
    lcd.clear();
    lcd.print("Speed Mode");
  } else if (menuIndex == 1) {
    currentMode = HARNESS_MODE;
    lcd.clear();
    lcd.print("Harness Mode");
  }
}

void returnToMenu() {
  currentMode = MENU;
  buttonReleaseGuard = true;
  buttonReleaseTime = millis();
  lcd.clear();
  displayMenu();
  Serial.println("Returned to MENU");
}

bool buttonPressed(int pin) {
  return (digitalRead(pin) == LOW);
}
