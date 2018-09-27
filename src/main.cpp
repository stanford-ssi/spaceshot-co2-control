#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "HTTPUtils.hpp"
#include <SPI.h>
#include <SD.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#include <sstream>

#include <ESPAsyncTCP.h>
#include <DNSServer.h>
#include <vector>
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

#define SERVER_HOST_NAME "StabilityTest"
#define TCP_PORT 7050
#define DNS_PORT 53

const char PSK[] = "spaceshot";
const char AP_SSID[] = "CO2GroundTest";

boolean recording = false;

Adafruit_MMA8451 mma = Adafruit_MMA8451();

WiFiServer server(80);

static DNSServer DNS;

static AsyncClient* curr_client;

 /* clients events */
static void handleError(void* arg, AsyncClient* client, int8_t error) {
	Serial.printf("\n connection error %s from client %s \n", client->errorToString(error), client->remoteIP().toString().c_str());
}

static void handleData(void* arg, AsyncClient* client, void *data, size_t len) {
	Serial.printf("\n data received from client %s \n", client->remoteIP().toString().c_str());
	Serial.write((uint8_t*)data, len);

  int cmd = *(int*)data;

  switch(cmd)
  {
    case 0:
    recording=true;
    break;
    case 1:
    recording=false;
    break;
    case 2:
    digitalWrite(valvePin,1);
    break;
    case 3:
    digitalWrite(valvePin,0);
    break;
    default:
    break;
  }
}

static void handleDisconnect(void* arg, AsyncClient* client) {
	Serial.printf("\n client %s disconnected \n", client->remoteIP().toString().c_str());
}

static void handleTimeOut(void* arg, AsyncClient* client, uint32_t time) {
	Serial.printf("\n client ACK timeout ip: %s \n", client->remoteIP().toString().c_str());
}


/* server events */
static void handleNewClient(void* arg, AsyncClient* client) {
	Serial.printf("\n new client has been connected to server, ip: %s", client->remoteIP().toString().c_str());

	// make accessible
	curr_client = client;

	// register events
	client->onData(&handleData, NULL);
	client->onError(&handleError, NULL);
	client->onDisconnect(&handleDisconnect, NULL);
	client->onTimeout(&handleTimeOut, NULL);
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


	// start dns server
	if (!DNS.start(DNS_PORT, SERVER_HOST_NAME, WiFi.softAPIP()))
		Serial.printf("\n failed to start dns service \n");

	AsyncServer* server = new AsyncServer(TCP_PORT); // start listening on tcp port 7050
	server->onClient(&handleNewClient, server);
	server->begin();

  if (! mma.begin())
  {   Serial.println("Couldnt start MMA");
     return;}
   else
   Serial.println("MMA8451 found!");
   mma.setRange(MMA8451_RANGE_2_G);
}



void loop() {
  DNS.processNextRequest();
  String data = getMMAData();
  char data_array[data.length()+1];
  strcpy(data_array, data.c_str());
  if(recording==true)
  {
    if (curr_client->space() > strlen(data_array) && curr_client->canSend()) {
      curr_client->add(data_array, strlen(data_array));
      curr_client->send();
    }
  }
  //delay();
    //logToSD(data);

}
