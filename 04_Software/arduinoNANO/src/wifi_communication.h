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
extern const char ssid[];                  // your network SSID (name)
extern const char pass[];                  // your network password (use for WPA, or use as key for WEP)   
extern int wifi_status;                    // the Wi-Fi radio's status