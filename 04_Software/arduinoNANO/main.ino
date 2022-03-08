// includes
#include "src\menu.h"
#include "src\sensor.h"
#include "src\wifi_communication.h"
// please enter your sensitive data in the Secret tab
char ssid[] = SECRET_SSID;                        // your network SSID (name)
char pass[] = SECRET_PASS;                        // your network password (use for WPA, or use as key for WEP)

#define UTC_OFFSET (3600)

typedef struct sensorData
{
  int humidity=0;
  int temperature=0;
  bool flame_detection=false;
  bool open_door=false;
  char date_formatted[11];
  char time_formatted[9];
}sensorData;

// declaration of functions

void getDateTime();
void get_sensor_temperature_humidity();
void get_sensor_fire();
void get_sensor_hall();
void check_buttons();
void update_lcd();

// necessary protothreads
pt ptDebounce;
pt ptSendData;
pt ptLCD;

// loop intervals
unsigned long previousMillisSensors = 0;          // will store last time sensor information was updated
unsigned long previousMillisWifi = 0;             // will store last time Wi-Fi information was updated
unsigned long previousMillisServer = 0;           //   "    "     "    "  Server      "      "     "
const int intervalSensors = 5000;                 // interval at which to update the sensor information
const int intervalWifiInfo = 10000;               // interval at which to update the Wi-Fi information
const int intervalServerInfo = 10000;             //   "       "   "    "   "     "  Server     "

// WiFi
WiFiServer server(80);                            // set WiFi Server at port 80
int wifi_status = WL_IDLE_STATUS;                 // the Wi-Fi radio's status

// FiFo storage
const int ELEMENT_CNT_MAX = 20;                   // defines maximum count of datasets that are stored
int cnt_sensorData = 0;
sensorData data_latest;                           // will store latest set of sensor data
sensorData data_saved[ELEMENT_CNT_MAX];           // will store latest x sets of data

// LCD
const int RS = 8, EN = 9, D4 = 4, D5 = 5, D6 = 6, D7 = 7;
LiquidCrystal lcd(RS,EN,D4,D5,D6,D7);

// menu variables
int state_screen = 1, next_state = 0, last_state = 0;
const int state_min = 0, state_max = 5;

// buttons
namespace buttons
{
  const int UP = 21, DOWN = 20;
}

// sensors

// fire detection
const int pin_flame_analog = 11;                        // flame
const int pin_flame_digital = 10;
const int analog_flame_threshold = 800;                 // 0... 1023
int cnt_flame = 0;

// door detection
const int pin_hall_analog = 12;                         // hall
const int analog_hall_threshold = 800;                  // 0... 1023
int cnt_hall = 0;


void setup() {
  // init necessary protothreads
  PT_INIT(&ptDebounce);
  PT_INIT(&ptSendData);
  PT_INIT(&ptLCD);

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

  //I2C
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
  
  // check buttons and update lcd
  check_buttons();
  update_lcd();

  // check the network connection once every 10 seconds:
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

  // check the webserver status once every 10 seconds:    
  if (millis() - previousMillisServer > intervalServerInfo) {
    if (server.status() != 1)
      server.begin();
    Serial.print("Server Status: ");
    Serial.println(server.status());
    previousMillisServer = millis();
  }

  // get data once every interval:
  if (millis() - previousMillisSensors > intervalSensors) {
    previousMillisSensors = millis();

    PT_SCHEDULE(get_latest_data(&ptSensors));
  }

  WiFiClient client = server.available();
  if (client) {
    PT_SCHEDULE(httpCommunication(&ptSendData, client));
  }
}




