#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <Wire.h>

const char* networkName = "esp32_mpu6050";
const char* networkPswd = "mathias_esp32";

//IP address to send UDP data to:
const char* udpAddress = "192.168.17.21";
const int udpPort = 3333;

//Are we currently connected?
boolean connected = false;

int ledPin = 5;

//The udp library class
WiFiUDP udp;

//sensor stuff
long accelX, accelY, accelZ;
float gForceX, gForceY, gForceZ;
long gyroX, gyroY, gyroZ;
float rotX, rotY, rotZ;


void setup(void) {

  Serial.begin(115200);

  Wire.begin();
  setupMPU();

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  //Connect to the WiFi network
  connectToWiFi(networkName, networkPswd);

  //set in sta mode
  WiFi.mode(WIFI_STA);
  delay(100);

  //register event handler
  WiFi.onEvent(WiFiEvent);
  delay(100);
}

void setupMPU() {
  Wire.beginTransmission(0b1101000);  //This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
  Wire.write(0x6B);                   //Accessing the register 6B - Power Management (Sec. 4.28)
  Wire.write(0b00000000);             //Setting SLEEP register to 0. (Required; see Note on p. 9)
  Wire.endTransmission();
  Wire.beginTransmission(0b1101000);  //I2C address of the MPU
  Wire.write(0x1B);                   //Accessing the register 1B - Gyroscope Configuration (Sec. 4.4)
  Wire.write(0x00000000);             //Setting the gyro to full scale +/- 250deg./s
  Wire.endTransmission();
  Wire.beginTransmission(0b1101000);  //I2C address of the MPU
  Wire.write(0x1C);                   //Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5)
  Wire.write(0b00000000);             //Setting the accel to +/- 2g
  Wire.endTransmission();
}

void loop(void) {


  if (connected) {

    digitalWrite(ledPin, !digitalRead(ledPin));

    OSCMessage msg("/mpu6050");
    msg.add((float)gyroX);  //there the message to send
    msg.add((float)gyroY);
    msg.add((float)gyroZ);
    msg.add((float)gForceX);  //there the message to send
    msg.add((float)gForceY);
    msg.add((float)gForceZ);


    udp.beginPacket(udpAddress, udpPort);
    msg.send(udp);
    udp.endPacket();

  } else {
    for (int i = 0; i < 5; i++) {
      delay(2000);
      digitalWrite(ledPin, !digitalRead(ledPin));
    }
    if (!connected) {
      connectToWiFi(networkName, networkPswd);
    }
  }

  recordAccelRegisters();
  recordGyroRegisters();
  printData();
  delay(100);
}

void connectToWiFi(const char* ssid, const char* pwd) {
  Serial.println("Connecting to WiFi network: " + String(ssid));

  // delete old config
  WiFi.disconnect(true);
  delay(500);

  //Initiate connection
  WiFi.begin(ssid, pwd);

  Serial.println("Waiting for WIFI connection...");
}


//wifi event handler
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      //When connected set
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());
      //initializes the UDP state
      //This initializes the transfer buffer
      udp.begin(WiFi.localIP(), udpPort);
      digitalWrite(ledPin, HIGH);
      connected = true;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      connected = false;
      digitalWrite(ledPin, LOW);
      break;
  }
}



void recordAccelRegisters() {
  Wire.beginTransmission(0b1101000);  //I2C address of the MPU
  Wire.write(0x3B);                   //Starting register for Accel Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000, 6);  //Request Accel Registers (3B - 40)
  while (Wire.available() < 6);
  accelX = Wire.read() << 8 | Wire.read();  //Store first two bytes into accelX
  accelY = Wire.read() << 8 | Wire.read();  //Store middle two bytes into accelY
  accelZ = Wire.read() << 8 | Wire.read();  //Store last two bytes into accelZ
  processAccelData();
}

void processAccelData() {
  gForceX = accelX / 16384.0;
  gForceY = accelY / 16384.0;
  gForceZ = accelZ / 16384.0;
}

void recordGyroRegisters() {
  Wire.beginTransmission(0b1101000);  //I2C address of the MPU
  Wire.write(0x43);                   //Starting register for Gyro Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000, 6);  //Request Gyro Registers (43 - 48)
  while (Wire.available() < 6)
    ;
  gyroX = Wire.read() << 8 | Wire.read();  //Store first two bytes into accelX
  gyroY = Wire.read() << 8 | Wire.read();  //Store middle two bytes into accelY
  gyroZ = Wire.read() << 8 | Wire.read();  //Store last two bytes into accelZ
  processGyroData();
}

void processGyroData() {
  rotX = gyroX / 131.0;
  rotY = gyroY / 131.0;
  rotZ = gyroZ / 131.0;
}

void printData() {
  Serial.print("Gyro (deg)");
  Serial.print(" X=");
  Serial.print(rotX);
  Serial.print(" Y=");
  Serial.print(rotY);
  Serial.print(" Z=");
  Serial.print(rotZ);
  Serial.print(" Accel (g)");
  Serial.print(" X=");
  Serial.print(gForceX);
  Serial.print(" Y=");
  Serial.print(gForceY);
  Serial.print(" Z=");
  Serial.println(gForceZ);
}
