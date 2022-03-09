#include <SPI.h>
#include <WiFiNINA.h>
#include <protothreads.h>

#include "secret.h"                              // please enter your sensitive data in the Secret tab

void wifiStartup();
void wifiConnect();
void wifiConnect();
void printWifiData();
void printCurrentNet();
void printMacAddress(byte mac[]);
int httpCommunication(struct pt* pt, WiFiClient client);
String sendHTML();

// WiFi
const char ssid[] = SECRET_SSID;                  // your network SSID (name)
const char pass[] = SECRET_PASS;                  // your network password (use for WPA, or use as key for WEP)   
WiFiServer server(80);                            // set WiFi Server at port 80
int wifi_status = WL_IDLE_STATUS;                 // the Wi-Fi radio's status