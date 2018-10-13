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
** MOSI - pin 11 [to pin 15]
** MISO - pin 12
** CLK - pin 13
** CS - pin 7 (for MKRZero SD: SDCARD_SS_PIN) [to pin 0]

Valve wiring:
- to GND, + to valvePin

MMA wiring:
5 = SDA
4 = SCL

*/
const int chipSelect = 0;
const int valvePin = 16;

const char PSK[] = "spaceshot";
const char AP_SSID[] = "CO2GroundTest";

boolean recording = false;

Adafruit_MMA8451 mma = Adafruit_MMA8451();

WiFiServer server(80);

void setup() {
  delay(5000);
  Serial.println("begin setup...");
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

  if (!SD.begin(chipSelect))
    Serial.println("SD Card failed");
  else
  Serial.println("SD Card initialized.");


  if (! mma.begin())
  {   Serial.println("Couldnt start MMA");
     return;}
   else
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
      if (req.indexOf("/data_start") != -1)
      {
        Serial.println("starting data");
        recording=true;
        httputils::HTTPRespond(client, "starting data");
      }
      else if (req.indexOf("/open") != -1)
      {
        Serial.println("opening valve");
        digitalWrite(valvePin, 1);
        httputils::HTTPRespond(client, "opened");
      }
      else if (req.indexOf("/close") != -1)
      {
        Serial.println("closing valve");
        digitalWrite(valvePin, 0);
        httputils::HTTPRespond(client, "closed");
      }
      else if (req.indexOf("/data_stop") != -1)
      {
        Serial.println("stopping data");
        recording = false;
        httputils::HTTPRespond(client, "stopping data");
      }
    }
    // put your main code here, to run repeatedly:
}

String getMMAData()
{
  sensors_event_t event;
  mma.read();
mma.getEvent(&event);
String buf;
buf+="a_x: ";
buf+=String(event.acceleration.x,6);
buf+=",a_y: ";
buf+=String(event.acceleration.y,6);
buf+=",a_z: ";
buf+=String(event.acceleration.z,6);
buf+="\n";
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
  Serial.println(data);
  delay(1000);
    //logToSD(data);

}
