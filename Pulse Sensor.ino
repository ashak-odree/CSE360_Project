#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address and LCD dimensions
int pulsePin = A0; // Pulse Sensor connected to analog pin A0
int blinkPin = 13; // LED pin to blink at each beat

// Volatile Variables, used in the interrupt service routine!
volatile int BPM; // Holds raw Analog in 0, updated every 2mS
volatile int Signal; // Holds the incoming raw data
volatile int IBI = 600; // Time interval between beats, must be seeded
volatile boolean Pulse = false; // True when User's live heartbeat is detected, false when not a "live beat"
volatile boolean QS = false; // Becomes true when Arduino finds a beat
static boolean serialVisual = true; // Set to 'false' by Default, re-set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse
volatile int rate[10]; // Array to hold last ten IBI values
volatile unsigned long sampleCounter = 0; // Used to determine pulse timing
volatile unsigned long lastBeatTime = 0; // Used to find IBI
volatile int P = 512; // Used to find peak in pulse wave, seeded
volatile int T = 512; // Used to find trough in pulse wave, seeded
volatile int thresh = 525; // Used to find instant moment of heart beat, seeded
volatile int amp = 100; // Used to hold amplitude of pulse waveform, seeded
volatile boolean firstBeat = true; // Used to seed rate array for reasonable BPM startup
volatile boolean secondBeat = false; // Used to seed rate array for reasonable BPM startup

void setup() {
  pinMode(blinkPin, OUTPUT); // Pin for heartbeat LED
  Serial.begin(115200); // Serial communication setup
  interruptSetup(); // Sets up to read Pulse Sensor signal every 2mS
  lcd.init(); // LCD initialization
  lcd.begin();
  lcd.backlight();
}

void loop() {
  serialOutput();
  if (QS == true) {
    serialOutputWhenBeatHappens();
    QS = false; // Reset the Quantified Self flag for next time
  }
  delay(20);
}

void interruptSetup() {
  TCCR2A = 0x02; // Disable PWM on digital pins 3 and 11, and go into CTC mode
  TCCR2B = 0x06; // Don't force compare, 256 prescaler
  OCR2A = 0X7C; // Set the top of the count to 124 for 500Hz sample rate
  TIMSK2 = 0x02; // Enable interrupt on match between Timer2 and OCR2A
  sei(); // Make sure global interrupts are enabled
}

void serialOutput() {
  if (serialVisual == true) {
    arduinoSerialMonitorVisual('-', Signal); // Serial Monitor Visualizer
  } else {
    sendDataToSerial('S', Signal); // Send data to serial function
  }
}

void serialOutputWhenBeatHappens() {
  if (serialVisual == true) {
    Serial.print(" Heart-Beat Found "); // Print heartbeat found
    Serial.print("BPM: ");
    Serial.println(BPM);
    lcd.print("Heart-Beat Found ");
    lcd.setCursor(1, 1);
    lcd.print("BPM: ");
    lcd.setCursor(5, 1);
    lcd.print(BPM);
    delay(300);
    lcd.clear();
  } else {
    sendDataToSerial('B', BPM); // Send heart rate with a 'B' prefix
    sendDataToSerial('Q', IBI); // Send time between beats with a 'Q' prefix
  }
}

void arduinoSerialMonitorVisual(char symbol, int data ) {
  const int sensorMin = 0; // Sensor minimum
  const int sensorMax = 1024; // Sensor maximum
  int sensorReading = data;
  int range = map(sensorReading, sensorMin, sensorMax, 0, 11);
}

void sendDataToSerial(char symbol, int data ) {
  Serial.print(symbol);
  Serial.println(data);
}

ISR(TIMER2_COMPA_vect) {
  cli(); // Disable interrupts
  Signal = analogRead(pulsePin); // Read the Pulse Sensor
  sampleCounter += 2; // Keep track of the time in mS
  int N = sampleCounter - lastBeatTime; // Monitor the time since the last beat to avoid noise

  if (Signal < thresh && N > (IBI / 5) * 3) {
    if (Signal < T) {
      T = Signal;
    }
  }

  if (Signal > thresh && Signal > P) {
    P = Signal;
  }

  if (N > 250) {
    if ((Signal > thresh) && (Pulse == false) && (N > (IBI / 5) * 3)) {
      Pulse = true;
      digitalWrite(blinkPin, HIGH);
      IBI = sampleCounter - lastBeatTime;
      lastBeatTime = sampleCounter;

      if (secondBeat) {
        secondBeat = false;
        for (int i = 0; i <= 9; i++) {
          rate[i] = IBI;
        }
      }

      if (firstBeat) {
        firstBeat = false;
        secondBeat = true;
        sei();
        return;
      }

      word runningTotal = 0;
      for (int i = 0; i <= 8; i++) {
        rate[i] = rate[i + 1];
        runningTotal += rate[i];
      }
      rate[9] = IBI;
      runningTotal += rate[9];
      runningTotal /= 10;
      BPM = 60000 / runningTotal;
      QS = true;
    }
  }

  if (Signal < thresh && Pulse == true) {
    digitalWrite(blinkPin, LOW);
    Pulse = false;
    amp = P - T;
    thresh = amp / 2 + T;
    P = thresh;
    T = thresh;
  }

  if (N > 2500) {
    thresh = 512;
    P = 512;
    T = 512;
    lastBeatTime = sampleCounter;
    firstBeat = true;
    secondBeat = false;
  }

  sei(); // Enable interrupts
}
