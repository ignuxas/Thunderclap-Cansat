#include "Arduino.h"
#include "LoRa_E22.h"

#include <vector>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <MPU9250_asukiaaa.h>

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <HardwareSerial.h>
#include "gps_f.h"

// --------------- RADIO
#define DESTINATION_ADDL 0 // SHIT AIN'T WORKING
#define Chan 0 // SHIT AIN'T WORKING

LoRa_E22 e22ttl(&Serial2, 18, 21, 19);  //  RX AUX M0 M1
void printParameters(struct Configuration configuration);

// --------------- Temperature / Humidty
#define DHTPIN 33
#define DHTTYPE DHT11
uint32_t delayMS;

DHT_Unified dht(DHTPIN, DHTTYPE);

// --------------- Gyro / Accelerometer
#ifdef _ESP32_HAL_I2C_H_
#define SDA_PIN 32
#define SCL_PIN 25
#endif

MPU9250_asukiaaa mySensor;
float aX, aY, aZ, aSqrt, gX, gY, gZ, mDirection, mX, mY, mZ;

float aXoffset = 0;
float aYoffset = 0.9;
float aZoffset = -0.04;

float gXoffset = -95;
float gYoffset = 98;
float gZoffset = 6;

// --------------- SD Card
#define HSPI_MISO   12 // 13
#define HSPI_MOSI   13
#define HSPI_SCLK   4
#define HSPI_SS     15

SPIClass *spi = NULL;

File myFile;

// --------------- GPS
const int gpsRX = 22;
const int gpsTX = 23;

String gpsData = ""; // this is not a setting, do not change

unsigned long start;
HardwareSerial gpsSerial(1);

const int altDetectionNum = 5; // how many altitude readings to extend the antenna

// --------------- Geiger counter
const int geigerPin = 35;
float geigerCPM = 0;
const int numReadings = 100; // Number of readings to store
int readings[numReadings] = {0}; // Array to store readings
int readIndex = 0; // Index to keep track of current position in array
long total = 0; // Running total of readings

// --------------- Other
int DelayT = 100;
int TransmissionDelay = 0;

std::vector<String> dataList;
std::vector<float> altList;

bool canTransmit = true;

bool isDecreasing(std::vector<float> values) {
  for (int i = 0; i < values.size() - 1; i++) {
    if (values[i] < values[i + 1]) {
      return false;
    }
  }
  return true;
}

// --------------- Multi-Threading
//TaskHandle_t Transmission;

void TransmissionFunc(/*void * parameter*/) {
  //while(true){
    /*
    myFile = SD.open("/data.txt", FILE_APPEND);
    String finalString = "";

    for(int i = 0; i < dataList.size(); i++){
      String thisString = dataList[i] + "\n";
      finalString += thisString;
      myFile.print(thisString);
      Serial.println(dataList[i]);
    }
    dataList.clear();
    */
    if(canTransmit){
        //if(gpsData != ""){
          //if(!canTransmit) altList.push_back(extractAltitude(gpsData));
          e22ttl.sendFixedMessage(0, DESTINATION_ADDL, Chan, gpsData);
        //}
    }
    //myFile.close();

    //FAILSAFE
    //ResponseContainer rc = e22ttl.receiveMessage();
    //if(rc.data == "FAILSAFE"){ canTransmit = true; }
    //----
    //if(!canTransmit && isDecreasing(altList)){ canTransmit = true; }

    if(altList.size() >= altDetectionNum){
      //altList.erase(altList.begin()); // delete first altitude recording
    }


    delay(TransmissionDelay);
    //}
};

// --------------- [SETUP] ---------------
void setup() {
  delay(500);
  // -------------- GPS
  gpsSerial.begin(9600, SERIAL_8N1, gpsRX, gpsTX);
  //                      idk   idk   idk   idk   idk   idk   GPS
  byte settingsArray[] = {0x03, 0xFA, 0x00, 0x00, 0xE1, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
  configureUblox(settingsArray);

  Serial.begin(115200);

  delay(20);
  // --------------- RADIO
  e22ttl.begin();  // Startup all pins and UART
  delay(500);

  String msg = "[ThunderVision] Hello from CANSAT - ThunderVisionn";
  ResponseStatus rs = e22ttl.sendFixedMessage(0, DESTINATION_ADDL, Chan, msg);  // send message

  Serial.println("");
  Serial.println("------------------------------------------------------------------");
  Serial.println("CanSat ThunderVision ON, attempting to send message on radio...");
  Serial.println(rs.getResponseDescription());  // message Status
  Serial.println("------------------------------------------------------------------");

  // --------------- Temperature / Humidty
  dht.begin(); // Initialize
  delay(20);

  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;

  delay(200);

  // --------------- Gyro / Accelerometer

  #ifdef _ESP32_HAL_I2C_H_ // For ESP32
    Wire.begin(SDA_PIN, SCL_PIN);
    mySensor.setWire(&Wire);
  #endif

  mySensor.beginAccel();
  mySensor.beginGyro();

  delay(200);
  Serial.print("setup() running on core ");
  Serial.println(xPortGetCoreID());
  

  // --------------- SD Card
  spi = new SPIClass(HSPI);
  spi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);

  if(!SD.begin(HSPI_SS, *spi)){
    Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }
  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
      Serial.println("MMC");
  } else if(cardType == CARD_SD){
      Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
  } else {
      Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  myFile = SD.open("/data.txt", FILE_WRITE);

  if (myFile) {
    Serial.print("Writing to data.txt...");
    myFile.println("0 0 0 0 0 0 0 0 0 0");
    Serial.println(" done.");
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening data.txt");
    myFile.close();
  }

  delay(500);

  //delay(0);

  //--------------- Multi-Threading

  //xTaskCreatePinnedToCore(
  //    TransmissionFunc, /* Function to implement the task */
  //    "Transmission", /* Name of the task */
  //    10000,  /* Stack size in words */
  //    NULL,  /* Task input parameter */
  //    0,  /* Priority of the task */
  //    &Transmission,  /* Task handle. */
  //    0); /* Core where the task should run */
}

void loop() {
  String CurrentData = "";

  // --------------- Temperature / Humidty
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
    CurrentData += "- ";
  } else {
    String temperature = String(event.temperature);
    CurrentData += temperature + " ";
    //Serial.printf("%.1f", event.temperature);
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
      CurrentData += "- ";
    Serial.println(F("Error reading humidity!"));
  } else {
    int humidity = (int) event.relative_humidity;
    CurrentData += String(humidity) + " ";
  }

  // --------------- Gyro / Accelerometer
  uint8_t sensorId;
  int result;

  result = mySensor.accelUpdate(); // Acceleration
  if (result == 0) {
    aX = mySensor.accelX(); // THIS IS ACTUALLY GYRO
    aY = mySensor.accelY();
    aZ = mySensor.accelZ();
    aSqrt = mySensor.accelSqrt();
    CurrentData += String(aX + aXoffset) + " " + String(aY + aYoffset) + " " + String(aZ + aZoffset) + " ";
    //Serial.println("accelSqrt: " + String(aSqrt));
  } else {
    CurrentData += "- - - ";
    Serial.println("Cannod read accel values " + String(result));
  }

  result = mySensor.gyroUpdate(); // Gyro
  if (result == 0) {
    gX = mySensor.gyroX(); // THIS IS ACTUALLY GYRO
    gY = mySensor.gyroY();
    gZ = mySensor.gyroZ();
    CurrentData += String(gX + gXoffset) + " " + String(gY + gYoffset) + " " + String(gZ + gZoffset) + " ";
  } else {
    CurrentData += "- - - ";
    Serial.println("Cannot read gyro values " + String(result));
  }

  mySensor.beginMag();
  result = mySensor.magUpdate();

  gpsData = "";
  bool recievedLocation = false;
  if(gpsSerial.available()) { // GPS
  String currentString = gpsSerial.readStringUntil('$');
    if (currentString.length() > 0) {
      if (currentString.startsWith("GPGGA")) {
        recievedLocation = true;
        currentString.replace("\n", "\0");
        currentString.replace("\r", "\0");

        CurrentData += currentString + " ";
      }
    }
  }

  if(!recievedLocation){CurrentData += "0 ";}

  CurrentData += String(millis());

  // Geiger counter ------------------------
  const int geigerValue = analogRead(geigerPin);

  readings[readIndex] = geigerValue;
  readIndex++;

  total += geigerValue;

  // Calculate average (consider using float for decimal results)
  float average = (float)total / numReadings;

  if (readIndex >= numReadings-1) {
    readIndex = 0;
    geigerCPM = average;
    total = 0;
  }

  CurrentData += " " + String(geigerCPM);
  // ------------------------

  if(recievedLocation){gpsData = CurrentData;}
  
  dataList.push_back(CurrentData);
  TransmissionFunc();

  // Temp sensor: 1000
  // Gyro / Accelerometer : ???
  // Radio: 200 ?
  delay(DelayT); // Default: 1000
  
  Serial.println(CurrentData);
  //Serial.println();
}

