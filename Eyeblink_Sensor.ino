const int sensorPin = 2;
const int motorPin = 8;
const int buzzerPin = 9;
long time; // Variable to store time

void setup() {
  pinMode(motorPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT); // Set buzzerPin as an OUTPUT
  digitalWrite(motorPin, HIGH); // Turn on the motor initially
  pinMode(sensorPin, INPUT);
}

void loop() {
  // Check if the IR sensor (sensorPin) is triggered (LOW means it's triggered)
  if (!digitalRead(sensorPin)) {
    time = millis(); // Record the current time
    // Keep checking if the IR sensor is still triggered
    while (!digitalRead(sensorPin)) {
      // When the IR sensor is triggered, turn off the buzzer and turn on the motor and wait for 1 second
      digitalWrite(buzzerPin, HIGH); // Turn on the buzzer when eyes are closed
      digitalWrite(motorPin, HIGH);
      delay(1000);
    }
  } else {
    // If the IR sensor is not triggered (eyes open)
    // Check the time elapsed since the sensor was last triggered
    if (TimeDelay() >= 3) {
      digitalWrite(buzzerPin, LOW); // Turn off the buzzer when eyes are open
      if (TimeDelay() >= 4) {
        digitalWrite(motorPin, LOW); // If 4 seconds have passed, turn off the motor
      }
    }
  }
}

int TimeDelay() {
  long t = millis() - time; // Calculate the time elapsed since the IR sensor was triggered
  t = t / 1000; // Convert milliseconds to seconds
  return t; // Return the elapsed time in seconds
}
