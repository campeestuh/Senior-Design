#include <LiquidCrystal_I2C.h>

// LCD settings
const int lcdColumns = 16, lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// Rotary Encoder and Button pins
const int CLK = 4, DT = 5, rotaryButton = 18, Button = 19;  // Pin 4 -> CLK, Pin 5 -> DT, Pin 18 -> SW, Pin 19 -> New Button

// Variables for rotary encoder and button states
volatile int encoderPos = 0, lastCLKState = HIGH;  // Tracks rotary encoder position and previous CLK state
volatile bool rotaryButtonPressed = false, buttonPressed = false;  // Tracks whether the rotary button or new button is pressed

// Menu settings
int menuIndex = 0, pageIndex = 0;
const int itemsPerPage = 1, numMenuItems = 2;
String menuItems[numMenuItems] = {"SPEED", "HARNESS"};  // Menu items
const int numPages = (numMenuItems + itemsPerPage - 1) / itemsPerPage;  // Number of pages for the menu

// Program modes and debounce settings
enum Mode { MENU, SELECTION };  // Modes: MENU or SELECTION
Mode currentMode = MENU;
unsigned long lastDebounceTime = 0, lastDisplayUpdate = 0;
const unsigned long debounceDelay = 50, displayUpdateInterval = 100;  // Debounce delay and display update interval

// Timer state
bool timerRunning = false;  // Timer running state
unsigned long startTime = 0, elapsedTime = 0, stoppedTime = 0;  // Tracks timer state

// Setup function initializes the system
void setup() {
  Serial.begin(9600);  
  lcd.init();
  lcd.backlight();
  bootMessage();  // Display boot message

  // Pin setup for rotary encoder and buttons
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(rotaryButton, INPUT_PULLUP);  
  pinMode(Button, INPUT_PULLUP);

  // Interrupt setup for rotary encoder and buttons
  attachInterrupt(digitalPinToInterrupt(CLK), readEncoder, CHANGE);  // Interrupt for rotary encoder turning
  attachInterrupt(digitalPinToInterrupt(rotaryButton), []{ rotaryButtonPressed = true; }, FALLING);  // Rotary button interrupt
  attachInterrupt(digitalPinToInterrupt(Button), []{ buttonPressed = true; }, FALLING);  // New button interrupt

  displayMenu();  // Display the initial menu
}

// Main loop function handles user input and display updates
void loop() {
  handleEncoder();  // Handle rotary encoder movement
  if (rotaryButtonPressed) handleRotaryButtonPress();  // Handle rotary button press
  if (buttonPressed) handleButtonPress();  // Handle new button press
  updateTimerDisplay();  // Update timer display if running
}

// Function to handle encoder position and update menu
void handleEncoder() {
  if (encoderPos != 0 && currentMode == MENU) {
    menuIndex = constrain(menuIndex + encoderPos, 0, numMenuItems - 1);  // Update menu index based on encoder movement
    pageIndex = menuIndex / itemsPerPage;  // Update page index
    displayMenu();  // Refresh the menu display
    encoderPos = 0;  // Reset encoder position
  }
}

// Function to read encoder rotation with debounce
void readEncoder() {
  if (millis() - lastDebounceTime > debounceDelay) {  // Debounce check
    if (digitalRead(CLK) != lastCLKState) {  // If the state of CLK has changed
      encoderPos = (digitalRead(CLK) == digitalRead(DT)) ? 1 : -1;  // Determine direction of rotation
      lastDebounceTime = millis();
    }
    lastCLKState = digitalRead(CLK);  // Update last CLK state
  }
}

// Function to handle rotary button press (switch between menu and selection)
void handleRotaryButtonPress() {
  rotaryButtonPressed = false;  // Reset flag
  currentMode = (currentMode == MENU) ? SELECTION : MENU;  // Switch mode
  if (currentMode == SELECTION) handleMenuSelection();  // Handle selection
  else displayMenu();  // Display menu
}

// Function to handle menu item selection
void handleMenuSelection() {
  lcd.clear();
  lcd.print(menuItems[menuIndex] + " selected");  // Display selected menu item
  delay(1000);
  displayMenu();  // Return to menu after selection
}

// Function to handle new button press (start/stop the timer)
void handleButtonPress() {
  buttonPressed = false;  // Reset flag
  timerRunning = !timerRunning;  // Toggle timer running state

  if (timerRunning) {
    startTime = millis();  // Record start time
    lcd.setCursor(11, 0);
    lcd.print("00:0");  // Initial timer display
    stoppedTime = 0;
  } else {
    stoppedTime = millis() - startTime;  // Calculate stopped time
    displayStoppedTime();  // Display the stopped time
  }
}

// Function to update the timer display while running
void updateTimerDisplay() {
  if (timerRunning && millis() - lastDisplayUpdate >= displayUpdateInterval) {
    lastDisplayUpdate = millis();
    displayTime(millis() - startTime);  // Display current elapsed time
  } else if (!timerRunning && stoppedTime > 0) {
    displayStoppedTime();  // Display stopped time if timer is not running
  }
}

// Function to display the current time (minutes and seconds)
void displayTime(unsigned long time) {
  unsigned long seconds = time / 1000, minutes = seconds / 60;
  seconds %= 60;
  lcd.setCursor(11, 0);
  lcd.print(minutes);
  lcd.print(':');
  lcd.print(seconds < 10 ? "0" : "");  // Add leading zero for seconds < 10
  lcd.print(seconds);
}

// Function to display the stopped time
void displayStoppedTime() {
  displayTime(stoppedTime);  // Display the last recorded stopped time
}

// Boot message function
void bootMessage() {
  lcd.print("WELCOME");  // Display welcome message
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("RATSTRONAUTS!");
  delay(1000);
  lcd.clear(); 
}

// Function to display the menu and update cursor position
void displayMenu() {
  lcd.clear();
  lcd.print("MENU");
  lcd.setCursor(0, 1);
  lcd.print(menuIndex == pageIndex * itemsPerPage ? "-> " : "   ");  // Highlight current menu item
  lcd.print(menuItems[menuIndex]);
}
