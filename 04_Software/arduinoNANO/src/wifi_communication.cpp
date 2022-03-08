#include "wifi_communication.h"

int httpCommunication(struct pt* pt, WiFiClient client) {
  PT_BEGIN(pt);

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
        client.println("Connection: close");    // the connection will be closed after completion of the response
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
  PT_SLEEP(pt, 1);

  // close the connection:
  client.stop();
  Serial.println("client disconnected");

  PT_END(pt);
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

