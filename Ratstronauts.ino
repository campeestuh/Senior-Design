#include <LiquidCrystal_PCF8574.h>
#include <AccelStepper.h>
#include <esp_sleep.h> // might change back

// AUGER IS SPINNING THE WRONG WAY MUST BE COUNTERCLOCKWIESE

// Pin Definitions
#define encoderCLK 4
#define encoderDT 5
#define rotaryButton 18

#define treadDirectionPin 25
#define treadPulsePin 27

#define augerDirectionPin 14
#define augerPulsePin 13

#define auger2DirectionPin 32
#define auger2PulsePin 33

#define harnessPulsePin 15
#define harnessDirectionPin 16

#define Button 29
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

// 2nd Auger (mirrors auger1)
AccelStepper auger2(1, auger2PulsePin, auger2DirectionPin);

// Harness Motor Settings
AccelStepper harness(1, harnessPulsePin, harnessDirectionPin);
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

int buttonState = 0;
int eButtonState = 0;

void setup() {
  Serial.begin(115200);

  lcd.begin(lcdColumns, lcdRows);
  lcd.setBacklight(255);
  displayMenu();

  pinMode(encoderCLK, INPUT);
  pinMode(encoderDT, INPUT);
  pinMode(rotaryButton, INPUT_PULLUP);
  pinMode(Button, INPUT_PULLUP);
  pinMode(emergencyButton, INPUT_PULLUP);
  pinMode(clockwiseButton, INPUT_PULLUP);
  pinMode(counterclockwiseButton, INPUT_PULLUP);

  treadmill.setMaxSpeed(maxSpeed);
  treadmill.setAcceleration(2000);

  auger.setMaxSpeed(augerMaxSpeed);
  auger.setAcceleration(2000);

  auger2.setMaxSpeed(augerMaxSpeed);
  auger2.setAcceleration(2000);

  harness.setMaxSpeed(800);
  harness.setAcceleration(300);
  harness.setSpeed(0);

  lastCLKState = digitalRead(encoderCLK);
}

void loop() {
  static Mode previousMode = MENU;

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
      auger2.runSpeed();
    }
  } else if (currentMode == SPEED_MODE) {
    handleSpeedMode();
    if (treadmillSpeed && augerSpeed == 0) {
      if (buttonPressed(rotaryButton)) returnToMenu();
    }
  } else if (currentMode == HARNESS_MODE) {
    handleHarnessMode();
    if (buttonPressed(rotaryButton)) returnToMenu();
  }
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
    treadmill.setSpeed(treadmillSpeed);
  } else if (menuIndex == 1) {
    currentMode = HARNESS_MODE;
    lcd.clear();
    lcd.print("Harness Mode");
  }
}

void displayMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MENU");
  lcd.setCursor(0, 1);
  lcd.print(menuIndex == 0 ? "-> SPEED" : "-> HARNESS");
}

bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW && (millis() - lastDebounceTime) > debounceDelay) {
    lastDebounceTime = millis();
    return true;
  }
  return false;
}

void handleSpeedMode() {
  int currentCLKState = digitalRead(encoderCLK);

  if (currentCLKState != lastCLKState) {
    if (digitalRead(encoderDT) != currentCLKState) {
      treadmillSpeed += treadmillIncrement;
      augerSpeed += augerIncrement;
    } else {
      treadmillSpeed -= treadmillIncrement;
      augerSpeed -= augerIncrement;
    }

    treadmillSpeed = constrain(treadmillSpeed, 0, maxSpeed);
    augerSpeed = constrain(augerSpeed, 0, augerMaxSpeed);

    treadmill.setSpeed(-treadmillSpeed);

    // 🔄 Reverse auger1 and match auger2
    auger.setSpeed(augerSpeed);
    auger2.setSpeed(augerSpeed);

    lastCLKState = currentCLKState;
  }

  treadmill.runSpeed();
  auger.runSpeed();
  auger2.runSpeed();

  eButtonState = digitalRead(emergencyButton);
  if (eButtonState == HIGH) eButtonPressed = false;

  if (treadmillSpeed == 0 && augerSpeed == 0) {
    if (buttonPressed(rotaryButton)) returnToMenu();
  }
}

void handleHarnessMode() {

  eButtonState = digitalRead(emergencyButton);
  if (eButtonState == HIGH) eButtonPressed = false;

  if (digitalRead(clockwiseButton) == LOW) {
    harness.setSpeed(200);
    harness.runSpeed();
  } else if (digitalRead(counterclockwiseButton) == LOW) {
    harness.setSpeed(-200);
    harness.runSpeed();
  } else {
    harness.setSpeed(0);
  }

  harness.runSpeed();
}

void returnToMenu() {
  currentMode = MENU;
  buttonReleaseGuard = true;
  buttonReleaseTime = millis();
  positionSaved = false;
  eButtonPressed = false;
  lcd.clear();
  displayMenu();
  Serial.println("Returned to MENU");
}
