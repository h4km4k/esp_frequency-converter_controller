#include <ETH.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include "DhcpServerLAN.h"
#include "esp_wifi.h"
#include "webinterface.h"
#include "favicon.h"
#include "manifest.h"

DhcpServerLAN dhcp;

WebServer server(80);

const char sw_js[] PROGMEM = R"(
self.addEventListener('install', (e) => {
  self.skipWaiting();
});
self.addEventListener('fetch', (e) => {
  e.respondWith(fetch(e.request));
});
)";

const char* ssid = "ESP-FU-Steuerung";
const char* password = "esp12345";

String currentDirection = "IUZ";
int dutyCycle = 0;
int currentPwm = 0;
int currentDuty = 0;
int targetPwm = 0;
bool startFreigegeben = false;
bool lanActive = false;
unsigned long lastTotmannSignal = 0;
const unsigned long totmannTimeout = 1000;

float scheibenDurchmesser = 7.5;



const int pwmPin = 4;
const int pwmFreq = 2900;
const int pwmResolution = 10;  // 10 Bit Auflösung (0-1023)
const int pwmRange = 1023;
const int minPWM = 80;  // Mindestwert, bei dem dein Motor zuverlässig anläuft (kannst du anpassen)

const int relay1Pin = 32;
const int relay2Pin = 33;


const int n = 11;
float fahrzeit[] = { 38.28, 38.25, 44.73, 51.84, 61.45, 73.56, 94.41, 126.24, 184.87, 531.15, 2000 };
int pwmProzent[] = { 100, 90, 80, 70, 60, 50, 40, 30, 20, 10, 1 };

unsigned long lastRampUpdate = 0;
const unsigned long rampStepDelay = 4;



int getPWMForFahrzeit(float zeit) {
  if (zeit <= fahrzeit[0]) return pwmProzent[0];
  if (zeit >= fahrzeit[n - 1]) return pwmProzent[n - 1];

  for (int i = 0; i < n - 1; i++) {
    if (zeit >= fahrzeit[i] && zeit <= fahrzeit[i + 1]) {
      float t1 = fahrzeit[i];
      float t2 = fahrzeit[i + 1];
      int p1 = pwmProzent[i];
      int p2 = pwmProzent[i + 1];

      float ratio = (zeit - t1) / (t2 - t1);
      int pwm = p1 + ratio * (p2 - p1);

      return pwm;
    }
  }
  return pwmProzent[n - 1];
}

void handleSet() {
  if (server.hasArg("duty")) {
    int val = server.arg("duty").toInt();
    if (val >= 0 && val <= 100) {
      if (val <= 0) {
        dutyCycle = 0;
      } else {
        dutyCycle = minPWM + (int)((val - 1) / 99.0 * (1023 - minPWM));
      }
      currentDuty = val;
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Bad Request");
}

void handleTotmann() {
  lastTotmannSignal = millis();
  server.send(200, "text/plain", "Totmann OK");
}

void handleStart() {

  bool recentTotmann = (millis() - lastTotmannSignal) < totmannTimeout;

  if (!recentTotmann) {
    server.send(400, "text/plain", "Start verweigert: Totmensch nicht aktiv");
    return;
  }

  if (dutyCycle <= 0) {
    server.send(400, "text/plain", "Start verweigert: dutyCycle=0");
    return;
  }

  startFreigegeben = true;
  targetPwm = dutyCycle;
  server.send(200, "text/plain", "Start OK");
}

void handleStop() {
  targetPwm = 0;
  startFreigegeben = false;
  server.send(200, "text/plain", "Stop OK");
}


void handleStatus() {
  bool recentTotmann = (millis() - lastTotmannSignal) < totmannTimeout;

  String status;
  if (recentTotmann && startFreigegeben) {
    status = "aktiv";
  } else if (recentTotmann && !startFreigegeben) {
    status = "warte";
  } else {
    status = "stop";
  }

  String dir = currentDirection;
  int duty = currentDuty;
  String json = "{\"status\":\"" + status + "\",\"direction\":\"" + dir + "\",\"duty\":" + String(duty) + "}";

  server.send(200, "application/json", json);
}


void handleGetDuty() {
  int dutyPercent = map(dutyCycle, 0, pwmRange, 0, 100);
  server.send(200, "text/plain", String(dutyPercent));
}

void handleGetPwm() {
  if (server.hasArg("time")) {
    float zeit = server.arg("time").toFloat();
    int pwm = getPWMForFahrzeit(zeit);
    server.send(200, "text/plain", String(pwm));
  } else {
    server.send(400, "text/plain", "Missing 'time' parameter");
  }
}

void handleSetDiameter() {
  if (server.hasArg("d")) {
    String val = server.arg("d");
    if (val == "7.5" || val == "8.5") {
      scheibenDurchmesser = val.toFloat();
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Bad Request");
}


void setRichtungStop() {
  digitalWrite(relay1Pin, LOW);
  digitalWrite(relay2Pin, LOW);
}


void handleRichtung() {
  if (server.hasArg("dir")) {
    String dir = server.arg("dir");

    if (dir == "stop") {
      digitalWrite(relay1Pin, LOW);
      digitalWrite(relay2Pin, LOW);
      currentDirection = "STOP";
      server.send(200, "text/plain", "Richtung: STOP");
      return;
    }

    if (dir == "iuz") {
      digitalWrite(relay1Pin, HIGH);
      digitalWrite(relay2Pin, LOW);
      currentDirection = "IUZ";
      server.send(200, "text/plain", "Richtung: Im Uhrzeigersinn");
      return;
    }

    if (dir == "guz") {
      digitalWrite(relay1Pin, LOW);
      digitalWrite(relay2Pin, HIGH);
      currentDirection = "GUZ";
      server.send(200, "text/plain", "Richtung: Gegen Uhrzeigersinn");
      return;
    }

    // Falscher Parameterwert
    server.send(400, "text/plain", "Ungueltiger Wert fuer 'dir'");
    return;
  }

  // 'dir' fehlt komplett
  server.send(400, "text/plain", "Fehlender Parameter 'dir'");
}



//////////////////////////////////SETUP/////////////////////////////////////
void setup() {
  // Serial.begin(115200);

  IPAddress LanIP(192, 168, 0, 1);
  IPAddress WlanIP(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);


  ETH.begin();
  ETH.config(LanIP, LanIP, subnet);
  delay(10000);

  if (ETH.linkUp()) {

    // Serial.print("LAN IP: ");
    // Serial.println(LanIP);

    lanActive = true;

    dhcp.begin(LanIP);
    dhcp.setPoolRange(200, 254);
    dhcp.setLeaseTime(3600);

    WiFi.mode(WIFI_OFF);
  } else {
    ETH.end();
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(WlanIP, WlanIP, subnet);

    wifi_country_t myCountry = {
      .cc = "DE",
      .schan = 1,
      .nchan = 13,
      .policy = WIFI_COUNTRY_POLICY_MANUAL
    };
    esp_wifi_set_country(&myCountry);
    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);

    WiFi.softAP(ssid, password);

    // Serial.print("WLAN IP: ");
    // Serial.println(WiFi.softAPIP());
  }

  ledcAttach(pwmPin, pwmFreq, pwmResolution);
  ledcWrite(pwmPin, 0);

  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  setRichtungStop();


  server.on("/", []() {
    handleRoot(server);
  });

  server.on("/deadman", HTTP_POST, handleTotmann);
  server.on("/set", HTTP_POST, handleSet);
  server.on("/start", HTTP_POST, handleStart);
  server.on("/stop", HTTP_POST, handleStop);
  server.on("/setDiameter", HTTP_POST, handleSetDiameter);
  server.on("/direction", HTTP_POST, handleRichtung);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/getDuty", HTTP_GET, handleGetDuty);
  server.on("/getPWMforTime", HTTP_GET, handleGetPwm);

  server.on("/favicon.png", []() {
    server.send_P(200, "image/png", (const char*)favicon_png, favicon_png_len);
  });
  server.on("/manifest.webmanifest", []() {
    server.send_P(200, "application/manifest+json", manifest_json);
  });

  server.begin();
  ArduinoOTA.setHostname("WT32-ETH01-OTA");
  ArduinoOTA.setPassword("8fQ8Zvg8qPkN");
  ArduinoOTA.begin();

  lastTotmannSignal = millis();
}

//////////////////////////////////LOOP/////////////////////////////////////
void loop() {
  if (lanActive) {
    dhcp.loop();
  }

  server.handleClient();

  ArduinoOTA.handle();

  unsigned long now = millis();
  if (currentPwm != targetPwm && (now - lastRampUpdate >= rampStepDelay)) {
    if (currentPwm < targetPwm) currentPwm++;
    else if (currentPwm > targetPwm) currentPwm--;

    if (currentPwm < 0) currentPwm = 0;
    if (currentPwm > pwmRange) currentPwm = pwmRange;

    ledcWrite(pwmPin, currentPwm);
    lastRampUpdate = now;
  }

  if ((millis() - lastTotmannSignal) > totmannTimeout && startFreigegeben) {
    startFreigegeben = false;
    targetPwm = 0;
    currentPwm = 0;
    ledcWrite(pwmPin, 0);
    setRichtungStop();
  }
}
