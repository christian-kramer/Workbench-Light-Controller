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
    for (int animationKeyframe = 0; animationKeyframe < maxAnimationKeyframes; animationKeyframe++) {
      analogWrite(1, find_bell_curve(animationLength, constrain(animationKeyframe, 0, animationLength)));
      analogWrite(2, find_bell_curve(animationLength, (constrain((animationKeyframe - animationOffset), 0, animationLength))));
      yield();
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
      yield();
    }
    delay(63);
    for (int i = 0; i < maxKeyframes; i++) {
      analogWrite(BUTTON_ONE_LED, find_bell_curve(doubleMaxKeyframes, i)); //ascending
      analogWrite(BUTTON_TWO_LED, find_bell_curve(doubleMaxKeyframes, (i + maxKeyframes))); //descending
      yield();
    }
    delay(63);
  }
}

void success_flash(int requestedAnimationCycles = 1) {
  int maxKeyframes = 1500;
  for (int animationCycles = 0; animationCycles < requestedAnimationCycles; animationCycles++) {
    for (int i = 0; i < maxKeyframes; i++) {
      analogWrite(BUTTON_ONE_LED, find_bell_curve(maxKeyframes, i));
      analogWrite(BUTTON_TWO_LED, find_bell_curve(maxKeyframes, i));
      yield();
    }
  }
}

void error_flash() {
  digitalWrite(BUTTON_ONE_LED, HIGH);
  digitalWrite(BUTTON_TWO_LED, HIGH);
  delay(250);
  digitalWrite(BUTTON_ONE_LED, LOW);
  digitalWrite(BUTTON_TWO_LED, LOW);
  delay(250);
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


bool get_button_state(uint8_t buttonID, uint8_t ledID) {
  bool button_state = !digitalRead(buttonID);
  analogWrite(ledID, button_state * 512);
  return button_state;
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

  bool enteredWPSMode = false;
  
  //insert alternate startup mode code here
  while (millis() < 10000) {
    ESP.wdtFeed();
    unsigned long modeEnterTime = millis();
    while(get_button_state(BUTTON_ONE_SWITCH, BUTTON_ONE_LED)) {
      ESP.wdtFeed();
      if (millis() > (modeEnterTime + 5000)) {
        enteredWPSMode = true;
        rolling_flash();
        analogWrite(BUTTON_ONE_LED, 128);
        analogWrite(BUTTON_TWO_LED, 128);
        WiFi.beginWPSConfig(); //this blocks
        int maxFlashes = 30;
        for (int i = 0; i < maxFlashes; i++) {
          railroad_flash();
          if (WiFi.status() == WL_CONNECTED) {
            i = maxFlashes;
          }
        }
        /*
        railroad_flash(5);
        rolling_flash();
        
        if (wps_connect()){
          success_flash(2);
        } else {
          while(true) {
            ESP.wdtFeed();
            error_flash();
          }
        }
        */
      }
    }
  }


  if (!enteredWPSMode) {
    WiFi.begin("","");
    while (WiFi.status() != WL_CONNECTED) {
      rolling_flash();
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    success_flash(2);
  } else {
    while(true) {
      ESP.wdtFeed();
      error_flash();
    }      
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
