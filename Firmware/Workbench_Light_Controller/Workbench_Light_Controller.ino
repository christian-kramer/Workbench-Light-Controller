//Includes
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include <ESP.h>
//#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
//Defines

//Global Variables
WiFiEventHandler gotIpEventHandler;
uint8_t wpsAnimationCycle;
int wpsAnimationKeyframe;

//Functions
uint16_t find_bell_curve(float total, float index)
{
    float exponent = -(pow(((5.0 * (index / total)) - 2.5), 2) / 0.08);
    return (pow(1.085, exponent) * 1023);
}

void wps_connect() {

  bool wpsVariable = WiFi.beginWPSConfig();

  //Animation variables
  uint8_t maxAnimationCycles = 10;
  int maxAnimationKeyframes = 5000; //explanation: 0 - 4000 first, 1000 - 5000 second
  int animationOffset = 350;
  int animationLength = maxAnimationKeyframes - animationOffset;

  gotIpEventHandler = WiFi.onStationModeGotIP([maxAnimationCycles, maxAnimationKeyframes](const WiFiEventStationModeGotIP& event)
  {
    wpsAnimationCycle = maxAnimationCycles;
    wpsAnimationKeyframe = maxAnimationKeyframes;
  });

  for (wpsAnimationCycle = 0; wpsAnimationCycle < maxAnimationCycles; wpsAnimationCycle++) {
    for (wpsAnimationKeyframe = 0; wpsAnimationKeyframe < maxAnimationKeyframes; wpsAnimationKeyframe++) {
      analogWrite(1, 1023 - find_bell_curve(animationLength, constrain(wpsAnimationKeyframe, 0, animationLength))); //between 0 - 4000
      analogWrite(2, 1023 - find_bell_curve(animationLength, (constrain((wpsAnimationKeyframe - animationOffset), 0, animationLength)))); //between 1000 - 4000
      ESP.wdtFeed(); //Important so it doesn't crash during this loop
    }
  }
}

void setup() {
  // pinmodes
  pinMode(0, OUTPUT); //boot
  pinMode(1, OUTPUT); //transmit, builtin LED
  pinMode(2, OUTPUT); //
  pinMode(3, OUTPUT); //recieve
  wifi_station_set_hostname("Lightswitch");
  analogWrite(1, 1023);
  analogWrite(2, 1023);
  // WPS works in STA (Station mode) only.
  WiFi.mode(WIFI_STA);
  delay(1000);
  WiFi.begin("","");
  delay(4000);
  wps_connect();
}

void loop() {
  // put your main code here, to run repeatedly:
  int maxsteps = 8191;
  
  for (int i = 0; i < maxsteps; i++)
  {
    int flippedcurve = 1023 - find_bell_curve(maxsteps, i);
    analogWrite(2, flippedcurve);
  }
}
