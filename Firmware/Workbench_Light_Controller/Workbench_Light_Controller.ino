//Includes
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>


//Defines
#define BUTTON_ONE_LED 1
#define BUTTON_ONE_SWITCH 3
#define BUTTON_TWO_LED 2
#define WPS_WAIT 10000

//Global Variables
WiFiEventHandler gotIpEventHandler;
int wpsCountdown;


bool wps_connect() {

  bool wpsVariable = WiFi.beginWPSConfig();

  gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event)
  {
    wpsCountdown = WPS_WAIT;
  });

  for (wpsCountdown = 0; wpsCountdown < WPS_WAIT; wpsCountdown++) {
    delay(1);
  }

  return (WiFi.status() == WL_CONNECTED);
}



void setup() {
  //pinmodes
  pinMode(0, OUTPUT); //boot
  pinMode(BUTTON_ONE_LED, OUTPUT); //transmit, builtin LED
  pinMode(BUTTON_TWO_LED, OUTPUT); //nothing special
  pinMode(BUTTON_ONE_SWITCH, INPUT); //recieve, only input that can be driven during boot
  analogWrite(BUTTON_ONE_LED, 1023);
  analogWrite(BUTTON_TWO_LED, 1023);
  
  // WPS works in STA (Station mode) only. One must connect to WPS after instantiating STA
  WiFi.mode(WIFI_STA);
  delay(1000);
  WiFi.begin("","");
  delay(4000);
  
  bool yeet = wps_connect();

  if (yeet) {
    analogWrite(BUTTON_ONE_LED, 128);
  } else {
    analogWrite(BUTTON_TWO_LED, 128);
  }
  
  //MDNS Setup
  if (MDNS.begin("esp8266test")) {              // Start the mDNS responder for esp8266test.local
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }
}


void loop() {
  //eee
  MDNS.update();
}
