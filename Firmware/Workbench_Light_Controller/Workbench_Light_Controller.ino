//Includes
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <FS.h>
#include <MD5Builder.h>
//#include <ArduinoJson.h>


//Defines
#define BUTTON_ONE_LED 1
#define BUTTON_ONE_SWITCH 3
#define BUTTON_TWO_LED 2
#define WPS_WAIT 10000

//Global Variables
MD5Builder _md5;
ESP8266WebServer server(80);

String md5(String str) {
  _md5.begin();
  _md5.add(String(str));
  _md5.calculate();
  return _md5.toString();
}

bool vesyncLogin(String username, String password) {
  String baseJSON = R"({"account": "<email>", "devToken": "", "password": "<password>"})";
  baseJSON.replace("<email>", username);
  baseJSON.replace("<password>", md5(password));
  
  std::unique_ptr<BearSSL::WiFiClientSecure>secureClient(new BearSSL::WiFiClientSecure);
  secureClient->setInsecure();
  HTTPClient http;
  http.begin(*secureClient, "https://smartapi.vesync.com/vold/user/login");
  http.addHeader("Content-Type", "application/json");
  http.POST(baseJSON);
  if (http.getString().indexOf("error") > 0) {
    return false;
  } else {
    return true;
  } 
}

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
        railroad_flash(5);
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
      ESP.wdtFeed(); //is this needed? try taking out
      error_flash();
    }      
  }

  //FS Setup
  SPIFFS.begin();
  
  //MDNS Setup
  if (MDNS.begin("esp8266test")) {              // Start the mDNS responder for esp8266test.local
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }
  
  //Web Server Setup
  server.on("/", handleRoot);
  server.on("/api/devices", handleDevices);
  server.on("/api/credentials", handleCredentials);
  server.onNotFound(handleNotFound);
  server.begin();

}


void loop() {
  server.handleClient();
  MDNS.update();
}

//web server functions
void handleDevices() {
  if (server.hasArg("plain") == false) {
    server.send(200, "text/html", "devices page without body");  
  } else {
    String message = "devices page with body: ";
    message += server.arg("plain");
    server.send(200, "text/html", message);
  }
}

void handleCredentials() {
  if (server.hasArg("plain") == false) {
    server.send(200, "text/html", "credentials page without body");  
  } else {
    if (vesyncLogin(server.arg("username"), server.arg("password"))) {
      server.send(200, "text/html", "successfully logged into vesync");  
    } else {
      
      String baseJSON = R"({"account": "<email>", "devToken": "", "password": "<password>"})";
      baseJSON.replace("<email>", server.arg("username"));
      baseJSON.replace("<password>", md5(server.arg("password")));
      
      server.send(200, "text/html", baseJSON);
    }
    /*
    if (SPIFFS.exists("credentials.txt")) {
      server.send(409, "text/html", "Valid credentials already exist, please reset."); 
    } else {
      File f = SPIFFS.open("credentials.txt", "a");
      f.println("yeet");
      f.close();
      server.send(200, "text/html", "Successfully wrote credentials.");
    }
    */
  }
}

void handleRoot() {
  //At its core, this should work as a single-page web app. Remember KISS: Keep It Simple, Stupid! No scope creep.
  server.send(200, "text/html", "root page");
}

void handleNotFound() {
  server.send(404, "text/html", "404 page");
}
