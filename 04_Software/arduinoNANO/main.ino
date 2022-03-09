// includes
#include "src\menu.h"
#include "src\sensors.h"
#include "src\wifi_communication.h"

// definition of storage struct
  typedef struct sensorData
  {
    int humidity=0;
    int temperature=0;
    bool flame_detection=false;
    bool open_door=false;
    char date_formatted[11];
    char time_formatted[9];
  }sensorData;

// global variables
  // necessary protothreads
  pt ptDebounce;
  pt ptSendData;
  pt ptSensors;

  // loop intervals
  unsigned long previousMillisSensors = 0;          // will store last time sensor information was updated
  unsigned long previousMillisWifi = 0;             // will store last time Wi-Fi information was updated
  unsigned long previousMillisServer = 0;           //   "    "     "    "  Server      "      "     "
  const int intervalSensors = 5000;                 // interval at which to update the sensor information
  const int intervalWifiInfo = 10000;               // interval at which to update the Wi-Fi information
  const int intervalServerInfo = 10000;             //   "       "   "    "   "     "  Server     "

  // FiFo storage
  const int ELEMENT_CNT_MAX = 20;                   // defines maximum count of datasets that are stored
  int cnt_sensorData = 0;
  sensorData data_latest;                           // will store latest set of sensor data
  sensorData data_saved[ELEMENT_CNT_MAX];           // will store latest x sets of data

  // Periphery
    // LCD
    const int RS = 8, EN = 9, D4 = 4, D5 = 5, D6 = 6, D7 = 7;
    LiquidCrystal lcd(RS,EN,D4,D5,D6,D7);

    // buttons
    namespace buttons
    {
      const int UP = 21, DOWN = 20;
    }

    // fire detection
    const int pin_flame_analog = 11;                       
    const int pin_flame_digital = 10;

    // door detection
    const int pin_hall_analog = 12;    

    // humidity - temperature
    const int i2cAdress_HumTemp = 0x28;

void setup() {
  /*
  The setup function is called once after every restart/reset of the mC.
  It is used to initialise the needed components (e.g. pin modes, wifi connection...)
  */

  // init necessary protothreads
  PT_INIT(&ptDebounce);
  PT_INIT(&ptSendData);
  PT_INIT(&ptSensors);

  // serial startup
  Serial.begin(9600);
  while(!Serial){
    ;
  }

  // LCD
	lcd.begin(16,2);    //set 16 columns and 2 rows of 16x2 LCD
  
  // button
	pinMode(buttons.UP, INPUT);
	pinMode(buttons.DOWN, INPUT);

  //flame sensor
	pinMode(pin_flame_analog, INPUT);
	pinMode(pin_flame_digital, INPUT);

  //I2C --> temp/humidity sensor
	Wire.begin();

  // WiFi
  wifiStartup();
  wifiConnect();
  
  printCurrentNet();
  printWifiData();
  Serial.println(server.status());
  server.begin();
  Serial.println(server.status());
}

void loop() {
  /*
  The loop function is continually called.
  It is used to check the peripherals in given intervals and connected networks for changes/errors.
  */
  
  // check buttons and update lcd every loop iteration
  check_buttons();
  update_lcd();

  // check the network connection once every WiFi interval:
  if (millis() - previousMillisWifi > intervalWifiInfo) {
    wifi_status = WiFi.status();
    Serial.print("wifistatus: ");
    Serial.println(wifi_status);
    if (wifi_status != WL_CONNECTED) {
      wifiConnect();
      printCurrentNet();
    }
    previousMillisWifi = millis();
  }

  // check the webserver status once every Server-Info interval:    
  if (millis() - previousMillisServer > intervalServerInfo) {
    if (server.status() != 1)
      server.begin();
    Serial.print("Server Status: ");
    Serial.println(server.status());
    previousMillisServer = millis();
  }

  // get data once every sensor interval:
  if (millis() - previousMillisSensors > intervalSensors) {
    previousMillisSensors = millis();
    PT_SCHEDULE(get_latest_data(&ptSensors));
  }

  // check for new clients at the webserver every loop iteration
  WiFiClient client = server.available();
  if (client) {
    PT_SCHEDULE(httpCommunication(&ptSendData, client));
  }
}