//Includes
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include <ESP.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
//Defines
#define BUTTON_ONE_LED 1
#define BUTTON_ONE_SWITCH 3
#define BUTTON_TWO_LED 2


void setup() {
  // pinmodes
  pinMode(0, OUTPUT); //boot
  pinMode(BUTTON_ONE_LED, OUTPUT); //transmit, builtin LED
  pinMode(BUTTON_TWO_LED, OUTPUT); //nothing special
  pinMode(BUTTON_ONE_SWITCH, INPUT); //recieve, only input that can be driven during boot
  analogWrite(BUTTON_ONE_LED, 1023);
  analogWrite(BUTTON_TWO_LED, 1023);

  
  WiFi.mode(WIFI_STA);
  delay(1000);
  WiFi.begin("","");
  delay(4000);
  
  //reset detection

  bool enteredReset = false;
  while (millis() < 15000) {
    ESP.wdtFeed();
    if (digitalRead(BUTTON_ONE_SWITCH)) {
      analogWrite(BUTTON_ONE_LED, 128);
    } else {
      analogWrite(BUTTON_ONE_LED, 1023);
    }

    //inside of here is within the first 5 seconds

    uint16_t savedTime = millis();
    while (digitalRead(BUTTON_ONE_SWITCH) == LOW) {

      ESP.wdtFeed();
      if ((millis() - savedTime) > 5000) {
        //boom! reset time
        enteredReset = true;
        
        for (int i = 0; i < 2; i++) {
          //flash
          for (int j = 0; j < 500; j++) {
            analogWrite(BUTTON_ONE_LED, find_bell_curve(8091, j));
            analogWrite(BUTTON_TWO_LED, find_bell_curve(8091, j));
          }
        }
        
        if (wps_connect()) {
          for (int i = 0; i < 2; i++) {
            //flash
            for (int j = 0; j < 8091; j++) {
              analogWrite(BUTTON_ONE_LED, find_bell_curve(8091, j));
              analogWrite(BUTTON_TWO_LED, find_bell_curve(8091, j));
            }
          }
        } else {
          while (true) {
            digitalWrite(BUTTON_ONE_LED, HIGH);
            digitalWrite(BUTTON_TWO_LED, HIGH);
            delay(500);
            digitalWrite(BUTTON_ONE_LED, LOW);
            digitalWrite(BUTTON_TWO_LED, LOW);
            delay(500);
          }
        }
      }

    }
  }
  
  
  //meta configuration
  wifi_station_set_hostname("Lightswitch");
  // WPS works in STA (Station mode) only.

  
  if (MDNS.begin("TimerButton")) {              // Start the mDNS responder for esp8266.local
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  
  MDNS.update();
  /*
  int maxsteps = 8191;
  
  for (int i = 0; i < maxsteps; i++)
  {
    int flippedcurve = 1023 - find_bell_curve(maxsteps, i);
    analogWrite(BUTTON_TWO_LED, flippedcurve);
  }
  */
}
