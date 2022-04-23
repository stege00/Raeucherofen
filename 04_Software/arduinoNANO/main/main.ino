/***********************************************************************************************************************************
INCLUDES
************************************************************************************************************************************/
#include <LiquidCrystal.h>
#include <protothreads.h>
#include <SPI.h>
#include <TimeLib.h>
#include <WiFiNINA.h>
#include <Wire.h>

#include "secret.h"                              // please enter your sensitive data in the Secret tab

/************************************************************************************************************************************
DEFINES
************************************************************************************************************************************/

#define UTC_OFFSET (3600)

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

/************************************************************************************************************************************
FUNCTION DECLARATIONS
************************************************************************************************************************************/
// menu
int check_buttons(struct pt* pt);
void update_lcd();
// wifi_communication
void wifiStartup();
void wifiConnect();
void wifiConnect();
void printWifiData();
void printCurrentNet();
void printMacAddress(byte mac[]);
int httpCommunication(struct pt* pt, WiFiClient client);
String sendHTML();
// sensors
int get_latest_data(struct pt* pt);
void getDateTime();
int get_sensor_temperature_humidity(struct pt* pt);
int get_sensor_fire(struct pt* pt, int para);
int get_sensor_hall(struct pt* pt);

/***********************************************************************************************************************************
GLOBAL VARIABLES
************************************************************************************************************************************/

/*********************************************************** PERIPHERALS ***********************************************************/

// LCD
const int RS = 8, EN = 9, D4 = 4, D5 = 5, D6 = 6, D7 = 7;
LiquidCrystal lcd(RS,EN,D4,D5,D6,D7);

// buttons
const uint8_t button_CONFIRM = 15;						  
const uint8_t button_UP = 21;
const uint8_t button_DOWN = 20;

// fire detection
const uint8_t pin_flame_analog = 3;                       
const uint8_t pin_flame_digital = 2;

// door detection
const uint8_t pin_hall_analog = 10;    

// humidity - temperature
const int i2cAdress_HumTemp = 0x28;

//LED
const uint8_t LED_red = 14;
const bool LED_status = HIGH;

/************************************************************* LOOP ****************************************************************/

// necessary protothreads (used in loop)
  pt ptButtons;
  pt ptWiFiCommunication;
  pt ptData;
  pt ptSensorTempHum;
  pt ptSensorFire;
  pt ptSensorHall;
  
// loop intervals
  unsigned long previousMillisSensors = 0;          // will store last time sensor information was updated
  unsigned long previousMillisWifi = 0;             // will store last time Wi-Fi information was updated
  unsigned long previousMillisServer = 0;           //   "    "     "    "  Server      "      "     "
  const int intervalSensors = 5000;                 // interval at which to update the sensor information
  const int intervalWifiInfo = 10000;               // interval at which to update the Wi-Fi information
  const int intervalServerInfo = 10000;             //   "       "   "    "   "     "  Server     "

/****************************************************** WIFI COMMUNICATION ********************************************************/

WiFiServer server(80);                            // set WiFi Server at port 80
WiFiClient client;

const char ssid[] = SECRET_SSID;                  // your network SSID (name)
const char pass[] = SECRET_PASS;                  // your network password (use for WPA, or use as key for WEP)   
int wifi_status = WL_IDLE_STATUS;                 // the Wi-Fi radio's status

/*********************************************************** SENSORS **************************************************************/

// FiFo storage
const int ELEMENT_CNT_MAX = 20;                         // defines maximum count of datasets that are stored
int cnt_sensorData = 0;
sensorData data_latest;                                 // will store latest set of sensor data
sensorData data_saved[ELEMENT_CNT_MAX];                 // will store latest x sets of data
boolean sensor_timeout;

// Sensor variables
int flame_detct_cnt = 0;                                // counts flame detection tries
int cnt_flame = 0;                                      // counts flame detections in given tries
const int analog_flame_threshold = 800;                 // 0... 1023
int hall_detct_cnt = 0;                                // counts flame detection tries
int cnt_hall = 0;
const int analog_hall_threshold = 800;

// status of protothreads
int status_ptSensorTempHum = 0;                         // 1 = protothread active
int status_ptSensorFire = 0;                            
int status_ptSensorHall = 0;                            

/************************************************************* MENU ***************************************************************/

int state_screen = 1, next_state = 0, last_state = 0;
const int state_min = 0, state_max = 6;

/***********************************************************************************************************************************
SETUP
************************************************************************************************************************************/

void setup() {
  /*
  The setup function is called once after every restart/reset of the mC.
  It is used to initialise the needed components (e.g. pin modes, wifi connection...)
  */

  // init necessary protothreads
  PT_INIT(&ptButtons);
  PT_INIT(&ptWiFiCommunication);
  PT_INIT(&ptData);
  PT_INIT(&ptSensorTempHum);
  PT_INIT(&ptSensorFire);
  PT_INIT(&ptSensorHall);

  // serial startup
  Serial.begin(9600);
  while(!Serial){
    ;
  }

  // LCD
	lcd.begin(16,2);    //set 16 columns and 2 rows of 16x2 LCD
  
  //LED 
	pinMode(LED_red, OUTPUT);

  // button
	pinMode(button_UP, INPUT);
	pinMode(button_DOWN, INPUT);
	pinMode(button_CONFIRM, INPUT);						 

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
  //Serial.println(server.status());
  server.begin();
  //Serial.println(server.status());
}

/***********************************************************************************************************************************
LOOP
************************************************************************************************************************************/

void loop() {
  /*
  The loop function is continually called.
  It is used to check the peripherals in given intervals and connected networks for changes/errors.
  */
  update_lcd();
  PT_SCHEDULE(check_buttons(&ptButtons));
  
  PT_SCHEDULE(get_latest_data(&ptData));
  PT_SCHEDULE(get_sensor_temperature_humidity(&ptSensorTempHum));
  PT_SCHEDULE(get_sensor_fire(&ptSensorFire, 0));
  PT_SCHEDULE(get_sensor_hall(&ptSensorHall));


  // check for new clients at the webserver every loop iteration
  client = server.available();
  if (client)
    httpCommunication(client);

  // check the network connection once every WiFi interval:
  if (millis() - previousMillisWifi > intervalWifiInfo) {
    wifi_status = WiFi.status();
    
    if (wifi_status != WL_CONNECTED) {
      Serial.print("WiFi disconnected:\nwifistatus: ");
      Serial.println(wifi_status);
      wifiConnect();
      printCurrentNet();
    }
    previousMillisWifi = millis();
  }

  // check the webserver status once every Server-Info interval:    
  if (millis() - previousMillisServer > intervalServerInfo) {

    if (server.status() != 1) {
      Serial.print("Server Status: ");
      Serial.println(server.status());
      Serial.println("Restarting Server");
      server.begin();
    }
    previousMillisServer = millis();
  }

}

/***********************************************************************************************************************************
MENU
************************************************************************************************************************************/

int check_buttons(struct pt* pt) {
  /*
  function to check input from buttons and debounce if needed
  */
  PT_BEGIN(pt);


  for(;;) {

    if(digitalRead(button_DOWN))
    {
      PT_SLEEP(pt, 5);
      if(digitalRead(button_DOWN))
      {
        if(state_screen!=state_min)
        {
          next_state-=1;
        }
        PT_YIELD_UNTIL(pt, !digitalRead(button_DOWN));
      }
    }
    else if(digitalRead(button_UP))
    {
      PT_SLEEP(pt, 5);
      if(digitalRead(button_UP))
      {
        if(state_screen!=state_max)
        {
          next_state+=1;
        }
        PT_YIELD_UNTIL(pt, !digitalRead(button_UP));
      }
    }
	  else if(digitalRead(button_CONFIRM))
    {
      PT_SLEEP(pt, 5);
      if(digitalRead(button_CONFIRM))
      {
        if(state_screen!=state_max)
        {
          Serial.println("Button Confirm");
        }
        PT_YIELD_UNTIL(pt, !digitalRead(button_UP));
      }
    }									
    else
    {
      PT_YIELD(pt);
    }
  }

  PT_END(pt);
}

void update_lcd() {

  lcd.clear();	  
  last_state=state_screen;
  state_screen=next_state;
  //state_machine
  
  //LED out for firedetektion not final version
	if(data_latest.flame_detection)
	  {
		digitalWrite(LED_red, HIGH);
	  }
	else
	  {
		digitalWrite(LED_red, LOW);
	  }
  
  
  switch (state_screen)
  {
      case 0:							  			//headpage
      {
        lcd.print("Studienarbeit");
		lcd.setCursor(0,1);
		lcd.print("TEL19GR3    2022");
        break;
      }
      case 1:										//data temp
      {
        lcd.print("Temp: ");
        lcd.print((float)data_latest.temperature, 0);
		lcd.print(" \xdf");							// ° Symbol
		lcd.print("C");
        break;
      }
      case 2:										//data humi
      {
		lcd.print("Feucht: ");
        lcd.print((float)data_latest.humidity, 0);
		lcd.print(" %");
        break;
      }
      case 3:										//data fire
      {
        lcd.print("Feuer: ");
        if(data_latest.flame_detection)
			  {
				  lcd.print("Ja");
			  }
		    else
			  {
				  lcd.print("Nein");
			  }
		    // lcd.setCursor(0,1);
		    // lcd.print(get_sensor_fire(1));
        break;
      }
      case 4:										//data hall
      {
        lcd.print("Tuer ");
        if(data_latest.open_door)
			  {
				  lcd.print("auf");
			  }
		    else
			  {
				  lcd.print("zu");
			  }
        break;
      }
      case 5:										//show all
      {
        lcd.print("Te: ");
		lcd.print((float)data_latest.temperature, 0);
		lcd.print("\xdf");				// ° Symbol
		lcd.print("C ");
		lcd.print("Hu: ");
        lcd.print((float)data_latest.humidity, 0);
		lcd.print("%");
		lcd.setCursor(0,1);
		lcd.print("Tu: ");
		if(data_latest.open_door)
			  {
				  lcd.print("auf ");
			  }
		    else
			  {
				  lcd.print("zu   ");
			  }
		lcd.print(" Fi: ");
        if(data_latest.flame_detection)
			  {
				  lcd.print("Yes");
			  }
		    else
			  {
				  lcd.print("No");
			  }
        break;
      }
	  case 6:										//Network
      {
        lcd.print("Netz:");
		switch(wifi_status)
		{
			case 3:
			{
				lcd.print(" OK");
				break;
			}
			case 4:
			{
				lcd.print(" n.C.");
				break;
			}
			default:
			{
				lcd.print("error ");
			}
		}
        lcd.setCursor(0,1);
		lcd.print(ssid);
        break;
      }
      default:
      {
        lcd.print("error ");
      }
  }
  delay(45);	                                    //replace with protothreads
}

/***********************************************************************************************************************************
WIFI COMMUNICATION
************************************************************************************************************************************/

void wifiStartup() {
   // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true); // don't continue
  }

  // check for firmware updates
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
}

void wifiConnect() {

  // attempt to connect to Wifi network:
  unsigned short int cnt = 0;
  while ((wifi_status != WL_CONNECTED) && (cnt < 3)) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    wifi_status = WiFi.begin(ssid, pass);

    // wait 5 seconds for connection:
    delay(5000);
    cnt+=1;
  }
  if (wifi_status != WL_CONNECTED) {
  Serial.println("Not connected to the network, try again later");
  }else {
    Serial.println("Successfully connected to the network");
  }
}

void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  
  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");

  printMacAddress(bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}

int httpCommunication(WiFiClient client) {
  Serial.println("new client");
  // an http request ends with a blank line
  boolean currentLineIsBlank = true;

  while (client.connected()) {

    if (client.available()) {

      char c = client.read();
      Serial.write(c);

      // the http request has ended, if it is the end of
      // the line (we received a newline character) and the
      // line is blank, so we can send a reply

      if (c == '\n' && currentLineIsBlank) {

        // send a standard http response header
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");      // the connection will be closed after completion of the response
        //client.println("Refresh: 10");          // refresh the page automatically every 10 sec
        client.println();
        client.print(sendHTML());
        break;
      }
      if (c == '\n') {

        // you're starting a new line
        currentLineIsBlank = true;
      } else if (c != '\r') {

        // you've gotten a character on the current line
        currentLineIsBlank = false;
      }
    }
  }
  // give the web browser time to receive the data
  delay(1);

  // close the connection:
  client.stop();
  Serial.println("client disconnected");
}

String sendHTML() {
  String content = "<!DOCTYPE HTML> <html>\n";
  content += "<head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  content += "<style> table, th, td { border:1px solid black; border-collapse: collapse;} </style>\n";
  content += "<title>Sensordata</title>\n</head>\n";
  content += "<body>\n <div id=\"latest_data\">\n";
  content += "<h2>Latest Sensordata</h2> ";
  content += "<p>humidity: ";
  content += String(data_latest.humidity);
  content += "</p>\n<p>temperature: ";
  content += String(data_latest.temperature);
  content += "</p>\n<p>flame detection: ";
  content += String(data_latest.flame_detection);
  content += "</p>\n<p>door open: ";
  content += String(data_latest.open_door);
  content += "</p>\n<p>time + date: ";
  content += String(data_latest.time_formatted) + " " + String(data_latest.date_formatted);
  content += "</p>\n";
  content += "</div>\n";
  content += "<div id=\"all_data\">\n";
  content += "<table>\n<tr>\n";
  content += "<th>humidity</th>\n";
  content += "<th>temperature</th>\n";
  content += "<th>flame detection</th>\n";
  content += "<th>door open</th>\n";
  content += "<th>time</th>\n";
  content += "<th>date</th>\n</tr>\n";
  for (int i = 0; i < cnt_sensorData; i++) {
    content += "<tr>\n<td>";
    content += String(data_saved[i].humidity);
    content += "</td>\n<td>";
    content += String(data_saved[i].temperature);
    content += "</td>\n<td>";
    content += String(data_saved[i].flame_detection);
    content += "</td>\n<td>";
    content += String(data_saved[i].open_door);
    content += "</td>\n<td>";
    content += data_saved[i].time_formatted;
    content += "</td>\n<td>";
    content += data_saved[i].date_formatted;
    content += "</td>\n</tr>\n";
  }
  content +="</div>\n </body>\n </html>\n";
  return content;
}

/***********************************************************************************************************************************
SENSORS
************************************************************************************************************************************/

int get_latest_data(struct pt* pt) {
  PT_BEGIN(pt);  

  for(;;) {  
    // get data once every sensor interval:
    if (millis() - previousMillisSensors > intervalSensors) {
      Serial.println("\nStart getting data:");
      status_ptSensorTempHum = 1;
      status_ptSensorFire = 1;
      status_ptSensorHall = 1;
      previousMillisSensors = millis();

      getDateTime();

      PT_SLEEP(pt, 5);

      Serial.println("waiting for sensors");

      PT_YIELD_UNTIL(pt, ((status_ptSensorTempHum || status_ptSensorFire || status_ptSensorHall) == 0) || (sensor_timeout = millis() - previousMillisSensors > 500));
      if (sensor_timeout)
      {
        Serial.println("sensors timed out");
      } else
      {
        Serial.println("sensors finished");
      }
      
      if (cnt_sensorData >= ELEMENT_CNT_MAX) {
        // first in first out storage:
        for (int i=0; i < ELEMENT_CNT_MAX - 1; i++){
          data_saved[i]=data_saved[i+1];
        }
        data_saved[ELEMENT_CNT_MAX - 1] = data_latest;
      }else {
        data_saved[cnt_sensorData]=data_latest;
        cnt_sensorData += 1;
      }
    }
    else
    {
      PT_YIELD(pt);
    }
    
  }
  PT_END(pt);
}

void getDateTime() {
  /*
  Function to get current Date and Time via the WiFi module.
  */

  unsigned long epochTime = WiFi.getTime() + UTC_OFFSET;               // time since 1.1.1970 in sec.
  int currentYear = year(epochTime);
  int currentMonth = month(epochTime);
  int monthDay = day(epochTime);
  int currentHour = hour(epochTime);
  int currentMinute = minute(epochTime);
  int currentSecond = second(epochTime);
  
  (String(monthDay)+"."+String(currentMonth)+"."+String(currentYear)).toCharArray(data_latest.date_formatted, 11);
  (String(currentHour)+":"+String(currentMinute)+":"+String(currentSecond)).toCharArray(data_latest.time_formatted, 9); 
}

int get_sensor_temperature_humidity(struct pt* pt) {
  /*
  Function to get sensor data from the temperature/humidity sensor module.
  Requests sensor data via I2C (adress of HYT221 is 0x28).
  First two bytes are humidity the last two are temperature.
  */

  PT_BEGIN(pt);


  for(;;) {  

    // get data once every sensor interval:
    if (status_ptSensorTempHum == 1) {
      Serial.println("start i2c");

      Wire.beginTransmission(i2cAdress_HumTemp);
      Wire.requestFrom(i2cAdress_HumTemp, 4);
      Serial.println("i2c request, waiting for answer");
	
      /*PT_SLEEP(pt, 20);								Code generates errors in I2C communication
      Serial.println("available stuff:");
      while (Wire.available())
      {
        int c = Wire.read();
        Serial.println(c);
      }

      PT_YIELD_UNTIL(pt, (Wire.available() == 4));
      Serial.println("i2c available");

      */
	  
      // Read the bytes if they are available
      int c1 = Wire.read();
      int c2 = Wire.read();
      int c3 = Wire.read();
      int c4 = Wire.read();
      
      Wire.endTransmission();

      // combine bytes
      int rawHumidity = c1 << 8 | c2;
      // first two bits are status/stall bits --> compound bitwise to get 14 bit measurement
      rawHumidity =  (rawHumidity &= 0x3FFF);
	  rawHumidity = 100.0 / pow(2,14) * rawHumidity;		//sensordata in permille
      // Mask away 2 least significant bits (14 bit measurement)
      c4 = (c4 >> 2);
      // combine bytes
      int rawTemperature = c3 << 6 | c4;
	  rawTemperature = 165.0 / pow(2,14) * rawTemperature - 40; // sensordata factor 10 to big
      
	  data_latest.humidity = rawHumidity;
      data_latest.temperature = rawTemperature;
      Serial.println("sensor temperature/humidity");
      Serial.print("raw data: ");
      Serial.println(c1);
      status_ptSensorTempHum = 0;
    }
    else
    {
      PT_YIELD(pt);
    }
  }

  PT_END(pt);
}

int get_sensor_fire(struct pt* pt, int para) {
  /*
  Function to get sensor data from the fire detection sensor module.
  Reads repeatedly sensor data to circumvent bouncing of sensor, if more than 60% of the Sensorvalues show positive, give positive flamedetection back.
  */

  PT_BEGIN(pt);


  for(;;) {  

    // get data once every sensor interval:
    if (status_ptSensorFire == 1) {
      cnt_flame = 0;

      for (flame_detct_cnt = 0; flame_detct_cnt < 10; flame_detct_cnt++)
      {

        if (para && (analogRead(pin_flame_analog) > analog_flame_threshold)) 
        {
          cnt_flame ++;
        }
        else if (!para && (digitalRead(pin_flame_digital)))
        {
          cnt_flame ++;
        }
        PT_SLEEP(pt, 5);
      }

      if (cnt_flame > 6) 
      {
        data_latest.flame_detection = true;
      }
      else
      {
        data_latest.flame_detection = false;
      }
      status_ptSensorFire = 0;
      Serial.println("Fire-Sensor finished");

    }
    else
    {
      PT_YIELD(pt);
    }
  }

  PT_END(pt);
}

int get_sensor_hall(struct pt* pt) {
  /*
  Function to get sensor data from the hall sensor (door detection).
  Reads repeatedly sensor data to circumvent bouncing of sensor, if less than 50% of the Sensorvalues show positive, give open door detection back.
  */
	PT_BEGIN(pt);


  for(;;) {  

    // get data once every sensor interval:
    if (status_ptSensorHall == 1) {
      
      cnt_hall = 0;

      for (hall_detct_cnt = 0; hall_detct_cnt < 10; hall_detct_cnt++)
      {
        if (analogRead(pin_hall_analog) > analog_hall_threshold) {
          cnt_hall ++;
        }
        PT_SLEEP(pt, 5);
      }

      if (cnt_hall < 5) 
      {
        data_latest.open_door = true;
      }
      else
      {
        data_latest.open_door = false;
      }
      status_ptSensorHall = 0;
      Serial.println("Hall-Sensor finished");

    }
    else
    {
      PT_YIELD(pt);
    }
  }

  PT_END(pt);
}
