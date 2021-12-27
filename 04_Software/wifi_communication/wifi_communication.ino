#include <SPI.h>
#include <WiFiNINA.h>

#include "secret.h"
//please enter your sensitive data in the Secret tab
char ssid[] = SECRET_SSID;                        // your network SSID (name)
char pass[] = SECRET_PASS;                        // your network password (use for WPA, or use as key for WEP)

int wifi_status = WL_IDLE_STATUS;                 // the Wi-Fi radio's status
unsigned long previousMillisWifi = 0;             // will store last time Wi-Fi information was updated
const int intervalWifiInfo = 10000;               // interval at which to update the board information

void wifiStartup();
void wifiConnect();
void printWifiData();
void printCurrentNet();
void printMacAddress(byte mac[]);

void setup() {
  // serial startup
  Serial.begin(9600);
  /*while(!Serial){
    ;
  }*/

  wifiStartup();
  wifiConnect();
  
  printCurrentNet();
  printWifiData();

}

void loop() {
  // check the network connection once every 10 seconds:
  if ((millis() - previousMillisWifi > intervalWifiInfo) && (wifi_status != WL_CONNECTED)) {
    wifiConnect();
    printCurrentNet();
    previousMillisWifi = millis();
  }
}

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

    // wait 10 seconds for connection:
    delay(10000);
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
