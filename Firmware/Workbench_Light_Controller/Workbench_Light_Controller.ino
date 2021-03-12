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

uint16_t find_bell_curve(float total, float index)
{
  float exponent = -(pow(((5.0 * (index / total)) - 2.5), 2) / 0.08);
  return (pow(1.085, exponent) * 1023);
}

void rolling_flash(int requestedAnimationCycles = 1) {
  int maxAnimationKeyframes = 2000;
  int animationOffset = 333;
  int animationLength = maxAnimationKeyframes - animationOffset;
  for (int animationCycles = 0; animationCycles < requestedAnimationCycles; animationCycles++) {
    ESP.wdtFeed();
    for (int animationKeyframe = 0; animationKeyframe < maxAnimationKeyframes; animationKeyframe++) {
      analogWrite(1, find_bell_curve(animationLength, constrain(animationKeyframe, 0, animationLength)));
      analogWrite(2, find_bell_curve(animationLength, (constrain((animationKeyframe - animationOffset), 0, animationLength))));
      ESP.wdtFeed(); //Important so it doesn't crash during this loop
    }
  }
}

void railroad_flash(int requestedAnimationCycles = 1) {
  int maxKeyframes = 1500;
  int doubleMaxKeyframes = maxKeyframes * 2;
  for (int animationCycles = 0; animationCycles < requestedAnimationCycles; animationCycles++) {
    for (int i = 0; i < maxKeyframes; i++) {
      analogWrite(BUTTON_ONE_LED, find_bell_curve(doubleMaxKeyframes, (i + maxKeyframes))); //descending
      analogWrite(BUTTON_TWO_LED, find_bell_curve(doubleMaxKeyframes, i)); //ascending
    }
    ESP.wdtFeed();
    delay(63);
    for (int i = 0; i < maxKeyframes; i++) {
      analogWrite(BUTTON_ONE_LED, find_bell_curve(doubleMaxKeyframes, i)); //ascending
      analogWrite(BUTTON_TWO_LED, find_bell_curve(doubleMaxKeyframes, (i + maxKeyframes))); //descending
    }
    ESP.wdtFeed();
    delay(63);
  }
}

bool getEnterCondition() {
  return !digitalRead(BUTTON_ONE_SWITCH);
}

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
  pinMode(0, INPUT); //boot
  pinMode(BUTTON_ONE_LED, OUTPUT); //transmit, builtin LED
  pinMode(BUTTON_TWO_LED, OUTPUT); //nothing special
  pinMode(BUTTON_ONE_SWITCH, INPUT); //recieve, only input that can be driven during boot
  analogWrite(BUTTON_ONE_LED, 0);
  analogWrite(BUTTON_TWO_LED, 0);
  
  // WPS works in STA (Station mode) only. One must connect to WPS after instantiating STA
  WiFi.mode(WIFI_STA);

  rolling_flash(2);

  bool enteredWPSMode = false;
  
  //insert alternate startup mode code here
  while (millis() < 5000) {
    ESP.wdtFeed();
    /*
    analogWrite(BUTTON_ONE_LED, constrain((1023 * digitalRead(BUTTON_ONE_SWITCH)) + 128, 0, 1023));
    analogWrite(BUTTON_TWO_LED, constrain((1023 * digitalRead(BUTTON_ONE_SWITCH)) + 128, 0, 1023));
    */
    analogWrite(BUTTON_ONE_LED, !digitalRead(BUTTON_ONE_SWITCH) * 128);
    unsigned long modeEnterTime = millis();
    while(getEnterCondition()) {
      ESP.wdtFeed();
      if (millis() > (modeEnterTime + 5000)) {
        enteredWPSMode = true;
        
        railroad_flash(5);
        rolling_flash();
        
        if (wps_connect()){
          //insert "success flash" here
        } else {
          analogWrite(BUTTON_TWO_LED, 128);
        }
      }
    }
  }


  if (!enteredWPSMode) {
    WiFi.begin("","");
    rolling_flash(8);
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
