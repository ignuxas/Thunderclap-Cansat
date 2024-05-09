#define threshold 1          // Minimum signal level to count as radiation
#define inputPin A2            // Pin for CPS output (analog)
#define outputPin A3            // Pin for running sum output (analog)
#define debugPin 1               // Serial port for debugging (change to your preferred serial pin)
#define sampleTime 1000          // Sampling time in milliseconds (1 second)

const int scanTime = 50;

int buffer[sampleTime/scanTime];  // Buffer to store counts for the last second (divided by 10ms for efficiency)
int bufferIndex = 0;         // Index pointing to the next element in the buffer
unsigned long startTime = 0; // Stores the start time of the current second

void setup() {
  pinMode(inputPin, OUTPUT);
  pinMode(outputPin, OUTPUT);
  pinMode(debugPin, OUTPUT);  // Initialize serial communication for debugging
  Serial.begin(9600);          // Set baud rate (optional, adjust as needed)
  startTime = millis();       // Start timer at setup
}

void loop() {
  unsigned long currentTime = millis();

  // Check if enough time has passed for a new sample (10 milliseconds)
  if (currentTime - startTime >= scanTime) {
    int sensorValue = analogRead(A2);  // Read the signal from the radiation counter

    // Update running sum
    if (sensorValue > threshold) {
      buffer[bufferIndex] = 1;  // Store 1 for detected radiation event
    } else {
      buffer[bufferIndex] = 0;  // Store 0 for no detection
    }

    // Update buffer index and handle overflow
    bufferIndex = (bufferIndex + 1) % (sampleTime/scanTime);
  }

  // Check if a second has passed (every 10ms)
  if (currentTime - startTime >= sampleTime) {
    int currentSum = calculateSum();
    float cps = (float)currentSum / (sampleTime/1000);  // Calculate CPS

    Serial.print("CPS: ");
    Serial.println(cps);

    // Reset for next second
    bufferIndex = 0;  // Reset buffer index
    startTime = currentTime;  // Reset timer for next second
  }

  // Output running sum on separate analog pin (updated every 10ms)
  analogWrite(outputPin, calculateSum());
}

// Function to calculate the sum of all counts in the buffer
int calculateSum() {
  int sum = 0;
  for (int i = 0; i < sampleTime/scanTime; i++) {
    sum += buffer[i];
  }
  return sum;
}
