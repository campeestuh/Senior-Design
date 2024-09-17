#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD settings
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// Rotary Encoder Wiring
const int CLK = 4;   // Pin 4 -> CLK pin
const int DT = 5;    // Pin 5 -> DT pin 
const int rotaryButton = 18;  // Pin 18 -> SW (Switch)
const int Button = 19;     // Pin 19 -> New Button
volatile int encoderPos = 0; // Default starting 
volatile bool rotaryButtonPressed = false; // Always set at 0 initially
volatile bool buttonPressed = false; // Always set at 0 initially

int menuIndex = 0;
int pageIndex = 0;
const int itemsPerPage = 1;  // Number of menu items to display per page
const int numMenuItems = 2;  // Number of menu items
String menuItems[numMenuItems] = {"SPEED", "HARNESS"};
const int numPages = (numMenuItems + itemsPerPage - 1) / itemsPerPage;  // Calculate number of pages

enum Mode { MENU, SELECTION };
Mode currentMode = MENU;

// Store the previous state of the rotary encoder's CLK pin
volatile int lastCLKState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // 50 ms debounce delay

// Global variables to track timer state
bool timerRunning = false;
unsigned long startTime = 0;
unsigned long elapsedTime = 0;
unsigned long lastDisplayUpdate = 0;
const unsigned long displayUpdateInterval = 100; // Update the display every 100 ms
unsigned long stoppedTime = 0; // Time at which the timer was stopped

void setup() {
  Serial.begin(9600);  // Initialize serial communication
  lcd.init();
  lcd.backlight();
  bootMessage();
  
  // Rotary Encoder setup
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(rotaryButton, INPUT_PULLUP);  // Use internal pull-up resistor
  pinMode(Button, INPUT_PULLUP);      // Use internal pull-up resistor

  attachInterrupt(digitalPinToInterrupt(CLK), readEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(rotaryButton), rotaryButtonPressedISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(Button), ButtonPressedISR, FALLING);

  displayMenu();
}

void loop() {
  // Check if encoder position has changed and update the menu
  if (encoderPos != 0 && currentMode == MENU) {
    menuIndex += encoderPos;

    // Ensure menuIndex stays within bounds
    menuIndex = constrain(menuIndex, 0, numMenuItems - 1);

    // Update the pageIndex based on the new menuIndex
    pageIndex = menuIndex / itemsPerPage;

    // Refresh the menu display
    displayMenu();

    // Reset encoder position after handling
    encoderPos = 0;
  }

  // Check if the rotary button was pressed
  if (rotaryButtonPressed) {
    rotaryButtonPressed = false;
    handleRotaryButtonPress();
  }

  // If the new button is pressed, handle the timer logic
  if (buttonPressed) {
    buttonPressed = false;
    handleButtonPress();
  }

  // Update the timer display if running
  if (timerRunning) {
    unsigned long currentTime = millis();
    elapsedTime = currentTime - startTime;

    // Update the timer display only if the specified interval has passed
    if (currentTime - lastDisplayUpdate >= displayUpdateInterval) {
      lastDisplayUpdate = currentTime;

      // Convert elapsedTime to minutes and seconds
      unsigned long seconds = elapsedTime / 1000;
      unsigned long minutes = seconds / 60;
      seconds = seconds % 60;

      // Display in MM:SS format
      lcd.setCursor(11, 0);
      lcd.print("   "); // Clear existing timer text
      lcd.setCursor(11, 0);
      lcd.print(minutes);
      lcd.print(":");
      if (seconds < 10) {
        lcd.print("0");  // Leading zero for seconds less than 10
      }
      lcd.print(seconds);
    }
  } else if (!timerRunning && stoppedTime > 0) {
    // Display the stopped time only if it has changed
    unsigned long seconds = stoppedTime / 1000;
    unsigned long minutes = seconds / 60;
    seconds = seconds % 60;

    // Update the display if stopped time is different from previous display
    static unsigned long lastStoppedTime = 0;
    if (stoppedTime != lastStoppedTime) {
      lastStoppedTime = stoppedTime;
      lcd.setCursor(11, 0);
      lcd.print("   "); // Clear previous timer display
      lcd.setCursor(11, 0);
      lcd.print(minutes);
      lcd.print(":");
      if (seconds < 10) {
        lcd.print("0");  // Leading zero for seconds less than 10
      }
      lcd.print(seconds);
    }
  }
}

// Rotary encoder read function with debounce
void readEncoder() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastDebounceTime > debounceDelay) {
    int currentCLKState = digitalRead(CLK);  
    int stateB = digitalRead(DT);  

    if (currentCLKState != lastCLKState) {
      if (digitalRead(CLK) == digitalRead(DT)) {
        encoderPos = 1;  // Clockwise rotation
      } else {
        encoderPos = -1;  // Counterclockwise rotation
      }
      lastDebounceTime = currentTime;
    }
    lastCLKState = currentCLKState;
  }
}

// Interrupt service routine for the rotary button press
void rotaryButtonPressedISR() {
  rotaryButtonPressed = true;
}

// Interrupt service routine for the new button press
void ButtonPressedISR() {
  buttonPressed = true;
}

// Function to handle the rotary button press
void handleRotaryButtonPress() {
  if (currentMode == MENU) {
    currentMode = SELECTION;
    handleMenuSelection();
  } else if (currentMode == SELECTION) {
    currentMode = MENU;
    displayMenu();
    resetEncoderState(); // Reset encoder state when returning to menu
  }
}

// Function to handle the action when a menu option is selected
void handleMenuSelection() {
  lcd.clear();
  switch (menuIndex) {
    case 0:
////////////////////////////////// TO DO //////////////////////////////////
      lcd.print("Speed selected");
      delay(1000);
      break;
    case 1:
////////////////////////////////// TO DO //////////////////////////////////
      lcd.print("Harness selected");
      delay(1000);
      break;
  }
  displayMenu(); // Return to menu after selection
}


// Function to handle actions for the new button (timer start/stop)
void handleButtonPress() {
  if (!timerRunning) {
    // Start the timer
    timerRunning = true;
    startTime = millis();  // Record the start time
    lcd.setCursor(11, 0);
    lcd.print("00:0");  // Initial timer display
    stoppedTime = 0;     // Reset stoppedTime
  } else {
    // Stop the timer
    timerRunning = false;
    stoppedTime = millis() - startTime; // Record the time at which the timer was stopped

    // Display the stopped time
    unsigned long seconds = stoppedTime / 1000;
    unsigned long minutes = seconds / 60;
    seconds = seconds % 60;

    lcd.setCursor(11, 0);
    lcd.print("   "); // Clear previous timer display
    lcd.setCursor(11, 0);
    lcd.print(minutes);
    lcd.print(":");
    if (seconds < 10) {
      lcd.print("0");  // Leading zero for seconds less than 10
    }
    lcd.print(seconds);
  }
}

// Boot Message is displayed when first turned on
void bootMessage() {
  lcd.setCursor(0, 0);
  lcd.print("WELCOME");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("RATSTRONAUTS!");
  delay(1000);
  lcd.clear(); 
}

// Function to reset encoder state
void resetEncoderState() {
  encoderPos = 0;
  lastCLKState = HIGH;
  lastDebounceTime = millis();
  Serial.println("Encoder state reset");  // Debugging output
}

// Function to display the current menu on the LCD
void displayMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);

  // Display "MENU" at the top
  lcd.print("MENU");

  for (int i = 0; i < itemsPerPage; i++) {
    int menuItemIndex = pageIndex * itemsPerPage + i;
    lcd.setCursor(0, i + 1);

    if (menuItemIndex < numMenuItems) {
      if (menuItemIndex == menuIndex) {
        lcd.print("-> ");
      } else {
        lcd.print("  ");
      }
      lcd.setCursor(2, i + 1);
      lcd.print(menuItems[menuItemIndex]);
    } else {
      lcd.print("                ");
    }
  }
}
