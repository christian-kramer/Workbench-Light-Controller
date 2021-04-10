//Includes
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <FS.h>
#include <MD5Builder.h>
#include <ArduinoJson.h>


//Defines
#define BUTTON_ONE_LED 1
#define BUTTON_ONE_SWITCH 3
#define BUTTON_TWO_LED 2
#define BUTTON_TWO_SWITCH 0
#define WPS_WAIT 10000
#define CREDENTIAL_FILENAME "/credentials.txt"
#define OUTLET_FILENAME "/outlets.txt"

//Global Variables
MD5Builder _md5;
ESP8266WebServer server(80);
String vesyncToken;

struct OutletIDs {
  String top;
  String bottom;
} outletIDs;

HTTPClient http;
std::unique_ptr<BearSSL::WiFiClientSecure>secureClient(new BearSSL::WiFiClientSecure);


String md5(String str) {
  _md5.begin();
  _md5.add(String(str));
  _md5.calculate();
  return _md5.toString();
}

String pickFunHeader() {
  long randNumber = random(3);
  if (randNumber == 0) {
    String OpenSSLVersion = String(random(3)) + '.' + String(random(3)) + '.' + String(random(3));
    return "Apache/2.4.1 (Scientific Linux) OpenSSL/" + OpenSSLVersion + "k-fips mod_fcgid/2.3.9 PHP/4.0.3";
  } else if (randNumber == 1) {
    String IISVersion = "Microsoft-IIS/7." + String(random(5));
    return IISVersion;
  } else if (randNumber == 2) {
    return "Oracle-Application-Server-10g";
  }
}

bool vesyncLogin(String username, String password) {
  String baseJSON = R"({"account": "<email>", "devToken": "", "password": "<password>"})";
  baseJSON.replace("<email>", username);
  baseJSON.replace("<password>", password);
  
  http.begin(*secureClient, "https://smartapi.vesync.com/vold/user/login");
  http.addHeader("Content-Type", "application/json");
  http.POST(baseJSON);

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, http.getString());
  const char* tempToken = doc["tk"];
  vesyncToken = tempToken;
  http.end();

  return !(doc.containsKey("error"));
}

bool outletState(String id) {
  String baseURL = "https://smartapi.vesync.com/v1/device/<outletid>/detail";
  baseURL.replace("<outletid>", id);
  http.begin(*secureClient, baseURL);
  http.addHeader("tk", vesyncToken);
  http.GET();
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, http.getString());
  const char* tempState = doc["deviceStatus"];
  http.end();
  
  return (strcmp(tempState, "on") == 0);
}


bool turnOutlet(String id, String verb) {
  String baseURL = "https://smartapi.vesync.com/v1/wifi-switch-1.3/<outletid>/status/<verb>";
  baseURL.replace("<outletid>", id);
  baseURL.replace("<verb>", verb);
  
  http.begin(*secureClient, baseURL);
  http.addHeader("tk", vesyncToken);
  int httpResponseCode = http.sendRequest("PUT", "");
  http.end();

  return (httpResponseCode == -1); //Confusing, I know, but returning "0" on success is what we want.
}


uint8_t toggleOutlet(String id) {
  String verb;

  if (WiFi.status() != WL_CONNECTED) { return 2; } //not even going to try if no wifi
  if (vesyncToken == "") { return 3; } //no token, this is a different show-stopping error
  
  if (outletState(id)) {
    verb = "off";
  } else {
    verb = "on";
  }

  if (turnOutlet(id, verb) == 1) {return 4;}
  
  
  return 0; //success
}

void parseOutletConfig() {
  if (SPIFFS.exists(OUTLET_FILENAME)) {
    File f = SPIFFS.open(OUTLET_FILENAME, "r");
    String outletsJson;
    while(f.available()) {
      char character = f.read();
      outletsJson += char(character);
    }
    f.close();

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, outletsJson);
    const char* topTemp = doc["top"];
    const char* bottomTemp = doc["bottom"];
    outletIDs.top = topTemp;
    outletIDs.bottom = bottomTemp;
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

void error_flash(int requestedAnimationCycles = 1) {
  for (int i = 0; i < requestedAnimationCycles; i++) {
    digitalWrite(BUTTON_ONE_LED, HIGH);
    digitalWrite(BUTTON_TWO_LED, HIGH);
    delay(250);
    digitalWrite(BUTTON_ONE_LED, LOW);
    digitalWrite(BUTTON_TWO_LED, LOW);
    delay(250);
  }
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
  pinMode(BUTTON_TWO_SWITCH, INPUT); //recieve, only input that can be driven during boot
  analogWrite(BUTTON_ONE_LED, 0);
  analogWrite(BUTTON_TWO_LED, 0);
  
  // WPS works in STA (Station mode) only. One must connect to WPS after instantiating STA
  WiFi.mode(WIFI_STA);

  bool enteredWPSMode = false;
  
  //insert alternate startup mode code here
  while (millis() < 10000) {
    yield();
    unsigned long modeEnterTime = millis();
    while(get_button_state(BUTTON_ONE_SWITCH, BUTTON_ONE_LED)) {
      yield();
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
      error_flash();
    }      
  }

  //FS Setup
  SPIFFS.begin();
  
  //MDNS Setup
  MDNS.begin("esp8266test"); //esp8266test.local

  //secureClient Setup
  secureClient->setInsecure();
  
  //Web Server Setup
  server.on("/", handleRoot);
  server.on("/api/devices", handleDevices);
  server.on("/api/credentials", HTTP_POST, handleCredentials);
  server.onNotFound(handleNotFound);
  server.begin();

  //Login to Vesync with saved credentials (if they exist)
  if (SPIFFS.exists(CREDENTIAL_FILENAME)) {
    File f = SPIFFS.open(CREDENTIAL_FILENAME, "r");
    String credentialsJson;
    while(f.available()) {
      char character = f.read();
      credentialsJson += char(character);
    }
    f.close();

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, credentialsJson);
    const char* usernameTemp = doc["username"];
    const char* md5passwordTemp = doc["md5password"];
    String username = usernameTemp;
    String md5password = md5passwordTemp;
    vesyncLogin(username, md5password);
  }

  //Read outlet configuration file into memory
  parseOutletConfig();

  //HTTP Request Timeout
  //http.setTimeout(5); //5 seconds
}


void loop() {
  server.handleClient();
  MDNS.update();

  if (get_button_state(BUTTON_ONE_SWITCH, BUTTON_ONE_LED) || get_button_state(BUTTON_TWO_SWITCH, BUTTON_TWO_LED)) {
    bool button_one_bool = false;
    bool button_two_bool = false;
    unsigned long buttonPressedTime = millis();
    while (millis() < (buttonPressedTime + 250)) {
      button_one_bool = button_one_bool | (!digitalRead(BUTTON_ONE_SWITCH));
      analogWrite(BUTTON_ONE_LED, button_one_bool * 512);
      button_two_bool = button_two_bool | (!digitalRead(BUTTON_TWO_SWITCH));
      analogWrite(BUTTON_TWO_LED, button_two_bool * 512);
    }

    if (button_one_bool) {
      error_flash(toggleOutlet(outletIDs.top));
      digitalWrite(BUTTON_ONE_LED, LOW);
    }
    
    if (button_two_bool) {
      error_flash(toggleOutlet(outletIDs.bottom));
      digitalWrite(BUTTON_TWO_LED, LOW);
    }
  }
}

//web server functions
void handleDevices() {
  server.sendHeader("Access-Control-Allow-Origin","*");
  server.sendHeader("Server", pickFunHeader());
  if (server.method() == HTTP_GET) {
    
    http.begin(*secureClient, "https://smartapi.vesync.com/vold/user/devices");
    http.addHeader("tk", vesyncToken);
    http.GET();
    String response = http.getString();
    http.end();
    
    server.send(200, "text/html", response);
  } else {    
    //create json string
    String fileJson;
    DynamicJsonDocument doc(1024);
    doc["top"] = server.arg("top");
    doc["bottom"] = server.arg("bottom");
    serializeJson(doc, fileJson);
    
    //save json string in file
    File f = SPIFFS.open(OUTLET_FILENAME, "w");
    f.print(fileJson);
    f.close();

    
    outletIDs.top = server.arg("top");
    outletIDs.bottom = server.arg("bottom");
    
    server.send(200, "text/html", "Outlet assignment confirmed");
  }
}

void handleCredentials() {
  server.sendHeader("Access-Control-Allow-Origin","*");
  server.sendHeader("Server", pickFunHeader());
  String username = server.arg("username");
  String md5password = md5(server.arg("password"));
  if (vesyncLogin(username, md5password)) {
    //successful login

    //create json string
    String fileJson;
    DynamicJsonDocument doc(1024);
    doc["username"] = username;
    doc["md5password"] = md5password;
    serializeJson(doc, fileJson);
    
    //save json string in file
    File f = SPIFFS.open(CREDENTIAL_FILENAME, "w");
    f.print(fileJson);
    f.close();
    
    server.send(200, "text/html", "successfully logged into vesync");
  } else {
    //unsuccessful login
    server.send(401, "text/html", "unsuccessful vesync login");
  }
}

void handleRoot() {
  //At its core, this should work as a single-page web app. Remember KISS: Keep It Simple, Stupid! No scope creep.
  server.sendHeader("Server", pickFunHeader());
  server.send(200, "text/html", "root page");
}

void handleNotFound() {
  server.sendHeader("Server", pickFunHeader());
  server.send(404, "text/html", "404 page");
}
