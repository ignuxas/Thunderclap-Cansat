const int outputPin = A3;
const int inputPin = A2;
const int threshold = 1000;

const int sampleRate = 50;
const int sampleCount = 1000/sampleRate;

unsigned long startTime = 0;

int listIndex = 0;
int cpsList[20];
int listIndexCpm = 0;
int cpmList[60];

int cps = 0;
int cpm = 0;

int timesPassed = 0;

void setup() {
  pinMode(outputPin, OUTPUT);
  pinMode(inputPin, INPUT);

  Serial.begin(9600);

  // put your setup code here, to run once:
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - startTime >= sampleRate) {
    if(listIndex >= sampleCount){
      cps = 0;
      for(int i=0; i < sizeof(cpsList)/sizeof(int); i++){
        cps += cpsList[i];
      }
      Serial.print("cps: ");
      Serial.println(cps);

      cpmList[listIndexCpm] = cps;

      memset(cpsList, 0, sizeof(cpsList)); // clear array
      listIndexCpm += 1;
      listIndex = 0;
    }

    int sensorValue = analogRead(inputPin);  // Read the signal from the radiation counter

    // Update running sum
    if (sensorValue > threshold) {
      cpsList[listIndex] = 1;
    } else {
      cpsList[listIndex] = 0;  // Store 0 for no detection
    }

    //Serial.print("other: ");
    //Serial.println(cpsList[listIndex]);

    listIndex += 1;
    timesPassed += 1;
    
    startTime = currentTime;  // Reset timer for next sampling period
  }

  if(timesPassed >= 1200){ // every minute  60000 / sampleRate
    cpm = 0;

    for(int i=0; i < sizeof(cpmList)/sizeof(int); i++){
      cpm += cpmList[i];
    }

    Serial.print("cpm: ");
    Serial.println(cpm);

    listIndexCpm = 0;
    
    timesPassed = 0;
    memset(cpmList, 0, sizeof(cpmList)); // clear array
  }

  // put your main code here, to run repeatedly:

}
