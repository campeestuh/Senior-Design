// Pin Definitions
const int stepPin = 9;    // Step pulse pin
const int dirPin = 8;     // Direction pin
const int enablePin = 7;  // Enable pin (optional)

// Set the delay between steps (controls speed)
int stepDelay = 100;      // Adjusted delay for 6 m/s speed

void setup() {
  // Set pins as outputs
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(enablePin, OUTPUT);

  // Enable the motor driver
  digitalWrite(enablePin, LOW);  // LOW to enable the motor (depends on driver)

  // Set direction for clockwise rotation
  digitalWrite(dirPin, HIGH);    // Set HIGH for CW (swap to LOW for CCW)
}

void loop() {
  // Spin motor for one revolution (1600 pulses per revolution)
  for (int i = 0; i < 1600; i++) {   
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(stepDelay);   // Adjust this value to control the speed
    digitalWrite(stepPin, LOW);
    delayMicroseconds(stepDelay);   // Same delay for pulse spacing
  }

}
