
//Global Variables
WiFiEventHandler gotIpEventHandler;
uint8_t wpsAnimationCycle;
int wpsAnimationKeyframe;
bool wpsLooking = true;
bool wpsSuccess = false;

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

bool wps_connect() {

  bool wpsVariable = WiFi.beginWPSConfig();

  //Animation variables
  /*
  uint8_t maxAnimationCycles = 10;
  int maxAnimationKeyframes = 3000; //explanation: 0 - 4000 first, 1000 - 5000 second
  int animationOffset = 400;
  int animationLength = maxAnimationKeyframes - animationOffset;
  */

  gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event)
  {
    /*
    wpsAnimationCycle = maxAnimationCycles;
    wpsAnimationKeyframe = maxAnimationKeyframes;
    */
    wpsLooking = false;
    wpsSuccess = true;
  });

  /* Use this animation for the NORMAL startup routine
  for (wpsAnimationCycle = 0; wpsAnimationCycle < maxAnimationCycles; wpsAnimationCycle++) {
    for (wpsAnimationKeyframe = 0; wpsAnimationKeyframe < maxAnimationKeyframes; wpsAnimationKeyframe++) {
      analogWrite(BUTTON_ONE_LED, find_bell_curve(animationLength, constrain(wpsAnimationKeyframe, 0, animationLength))); //between 0 - 4000
      analogWrite(BUTTON_TWO_LED, find_bell_curve(animationLength, (constrain((wpsAnimationKeyframe - animationOffset), 0, animationLength)))); //between 1000 - 4000
      ESP.wdtFeed(); //Important so it doesn't crash during this loop
    }
  }
  */

  unsigned long startLookingTime = millis();
  int maxKeyframes = 1500;
  int doubleMaxKeyframes = maxKeyframes * 2;
  
  while (wpsLooking) {
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

    if ((millis() - startLookingTime) > 30000) {
      //30 seconds have elapsed in this while loop
      wpsLooking = false;
    }
  }

  return wpsSuccess;
  
}
