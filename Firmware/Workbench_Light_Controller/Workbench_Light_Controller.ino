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
#define BUTTON_ONE_LED 1
#define BUTTON_ONE_SWITCH 3
#define BUTTON_TWO_LED 2
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

int find_linear(float total, float index)
{
    float amount = index / total;
    return (1023.0 * amount);
}

void wps_connect() {

  bool wpsVariable = WiFi.beginWPSConfig();

  //Animation variables
  uint8_t maxAnimationCycles = 10;
  int maxAnimationKeyframes = 6000; //explanation: 0 - 4000 first, 1000 - 5000 second
  int animationOffset = 1000;
  int animationLength = maxAnimationKeyframes - animationOffset;

  gotIpEventHandler = WiFi.onStationModeGotIP([maxAnimationCycles, maxAnimationKeyframes](const WiFiEventStationModeGotIP& event)
  {
    wpsAnimationCycle = maxAnimationCycles;
    wpsAnimationKeyframe = maxAnimationKeyframes;
  });

  for (wpsAnimationCycle = 0; wpsAnimationCycle < maxAnimationCycles; wpsAnimationCycle++) {
    for (wpsAnimationKeyframe = 0; wpsAnimationKeyframe < maxAnimationKeyframes; wpsAnimationKeyframe++) {
      analogWrite(BUTTON_ONE_LED, 1023 - find_bell_curve(animationLength, constrain(wpsAnimationKeyframe, 0, animationLength))); //between 0 - 4000
      analogWrite(BUTTON_TWO_LED, 1023 - find_bell_curve(animationLength, (constrain((wpsAnimationKeyframe - animationOffset), 0, animationLength)))); //between 1000 - 4000
      ESP.wdtFeed(); //Important so it doesn't crash during this loop
    }
  }
}

void setup() {
  // pinmodes
  pinMode(0, OUTPUT); //boot
  pinMode(BUTTON_ONE_LED, OUTPUT); //transmit, builtin LED
  pinMode(BUTTON_TWO_LED, OUTPUT); //nothing special
  pinMode(BUTTON_ONE_SWITCH, INPUT); //recieve, only input that can be driven during boot
  analogWrite(BUTTON_ONE_LED, 1023);
  analogWrite(BUTTON_TWO_LED, 1023);
  
  //reset detection
  
  while (millis() < 5000) {
    ESP.wdtFeed();
    analogWrite(BUTTON_ONE_LED, 1023);
    //inside of here is within the first 5 seconds
    uint16_t savedTime = millis();
    while (digitalRead(BUTTON_ONE_SWITCH) == LOW) {
      ESP.wdtFeed();
      analogWrite(BUTTON_ONE_LED, 128); //quarter brightness
      if ((millis() - savedTime) > 5000) {
        //boom! reset time
        while (true) {
          int maxKeyframes = 1500;
          int doubleMaxKeyframes = maxKeyframes * 2;
          for (int i = 0; i < maxKeyframes; i++) {
            analogWrite(BUTTON_ONE_LED, find_bell_curve(doubleMaxKeyframes, (i + maxKeyframes))); //descending
            analogWrite(BUTTON_TWO_LED, find_bell_curve(doubleMaxKeyframes, i)); //ascending
          }
          //ESP.wdtFeed();
          delay(63);
          for (int i = 0; i < maxKeyframes; i++) {
            analogWrite(BUTTON_ONE_LED, find_bell_curve(doubleMaxKeyframes, i)); //ascending
            analogWrite(BUTTON_TWO_LED, find_bell_curve(doubleMaxKeyframes, (i + maxKeyframes))); //descending
          }
          ESP.wdtFeed();
          delay(63);
          /*
          int maxAnimationKeyframes = 6000;
          int animationOffset = 1000;
          int animationLength = maxAnimationKeyframes - animationOffset;
          
          for (int animationKeyframe = 0; animationKeyframe < maxAnimationKeyframes; animationKeyframe++) {
            analogWrite(1, find_bell_curve(animationLength, constrain(animationKeyframe, 0, animationLength))); //between 0 - 4000
            analogWrite(2, find_bell_curve(animationLength, (constrain((animationKeyframe - animationOffset), 0, animationLength)))); //between 1000 - 4000
            ESP.wdtFeed(); //Important so it doesn't crash during this loop
          }
          */
        }
      }
    }
  }
  
  analogWrite(BUTTON_ONE_LED, 512); //no more reset check, half brightness
  
  //meta configuration
  wifi_station_set_hostname("Lightswitch");
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
    analogWrite(BUTTON_TWO_LED, flippedcurve);
  }
}
