#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "HTTPUtils.hpp"
#include <SPI.h>
#include <SD.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#include <sstream>

/*
SD wiring:
** MOSI - pin 11
** MISO - pin 12
** CLK - pin 13
** CS - pin 7 (for MKRZero SD: SDCARD_SS_PIN)

Valve wiring:
- to GND, + to valvePin

MMA wiring:
4 = SDA
5 = SCL

*/
const int chipSelect = 7;
const int valvePin = 8;

const char PSK[] = "spaceshot";
const char AP_SSID[] = "CO2GroundTest";

Adafruit_MMA8451 mma = Adafruit_MMA8451();
sensors_event_t event;
WiFiServer server(80);

void setup() {

  Serial.begin(115200);

  pinMode(valvePin, OUTPUT);
  digitalWrite(valvePin, LOW);

  WiFi.mode(WIFI_AP);

  IPAddress ip(192, 168, 4, 1);
  IPAddress dns(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(AP_SSID, PSK);

  server.begin();

  if (!SD.begin(chipSelect)) {
    Serial.println("SD Card failed");
  }
  Serial.println("SD Card initialized.");


  if (! mma.begin()) {
     Serial.println("Couldnt start MMA");
   }
   Serial.println("MMA8451 found!");
   mma.setRange(MMA8451_RANGE_2_G);
}


void handleWifi()
{
    WiFiClient client = server.available();
    if (client)
    {
      String req = client.readStringUntil('\r');
      client.flush();
      String resp = "";

      if (req.indexOf("/open") != -1)
      {
        digitalWrite(valvePin, 0);
        httputils::HTTPRespond(client, "opened");
      }
      else if (req.indexOf("/closed") != -1)
      {
        digitalWrite(valvePin, 1);
        httputils::HTTPRespond(client, "closed");
      }
    }
    // put your main code here, to run repeatedly:
}

String getMMAData()
{
mma.getEvent(&event);
String buf;
buf+=F("a_x: ");
buf+=String(event.acceleration.x,6);
buf+=F(",a_y: ");
buf+=String(event.acceleration.y,6);
buf+=F(",a_z: ");
buf+=String(event.acceleration.z,6);
buf+=F("\n");
return buf;

}

void logToSD(String dataString)
{

// open the file. note that only one file can be open at a time,
// so you have to close this one before opening another.
File dataFile = SD.open("datalog.txt", FILE_WRITE);

// if the file is available, write to it:
if (dataFile) {
  dataFile.println(dataString);
  dataFile.close();
  // print to the serial port too:
  Serial.println(dataString);
}
// if the file isn't open, pop up an error:
else {
  Serial.println("error opening datalog.txt");
}

}


void loop() {

  handleWifi();
  String data = getMMAData();
  logToSD(data);

}
