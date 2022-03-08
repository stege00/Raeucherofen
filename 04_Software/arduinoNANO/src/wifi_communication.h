#include <SPI.h>
#include <WiFiNINA.h>
#include <protothreads.h>

#include "secret.h"

int httpCommunication(struct pt* pt, WiFiClient client)
String sendHTML();
void wifiStartup();
void wifiConnect();
void printWifiData();
void printCurrentNet();
void printMacAddress(byte mac[]);