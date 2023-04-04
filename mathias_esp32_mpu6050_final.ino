#include<Wire.h>
 
const int MPU_addr=0x68;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
 
int minVal=265;
int maxVal=402;
 
double x;
double y;
double z;

//for wifi
#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>

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
 
void setup(){
Wire.begin();
Wire.beginTransmission(MPU_addr);
Wire.write(0x6B);
Wire.write(0);
Wire.endTransmission(true);
Serial.begin(115200);
//Connect to the WiFi network
  connectToWiFi(networkName, networkPswd);

  //set in sta mode
  WiFi.mode(WIFI_STA);
  delay(100);

  //register event handler
  WiFi.onEvent(WiFiEvent);
}
void loop(){
Wire.beginTransmission(MPU_addr);
Wire.write(0x3B);
Wire.endTransmission(false);
Wire.requestFrom(MPU_addr,14,true);
AcX=Wire.read()<<8|Wire.read();
AcY=Wire.read()<<8|Wire.read();
AcZ=Wire.read()<<8|Wire.read();
int xAng = map(AcX,minVal,maxVal,-90,90);
int yAng = map(AcY,minVal,maxVal,-90,90);
int zAng = map(AcZ,minVal,maxVal,-90,90);
 
x= RAD_TO_DEG * (atan2(-yAng, -zAng)+PI);
y= RAD_TO_DEG * (atan2(-xAng, -zAng)+PI);
z= RAD_TO_DEG * (atan2(-yAng, -xAng)+PI);
 
Serial.print("AngleX= ");
Serial.println(x);
 
Serial.print("AngleY= ");
Serial.println(y);
 
Serial.print("AngleZ= ");
Serial.println(z);
Serial.println("-----------------------------------------");

//wifi osc send

  if (connected) {

    digitalWrite(ledPin, !digitalRead(ledPin));

    OSCMessage msg("");
    msg.add((float)x);  //there the message to send
    msg.add((float)y);
    msg.add((float)z);



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
      connected = true;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      connected = false;
      break;
  }
}