#include <WiFi.h>
#include <WebServer.h>
#include "webinterface.h"

WebServer server(80);

const char* ssid = "ESP-FU-Steuerung";
const char* password = "esp12345";

int dutyCycle = 0;
int currentPwm = 0;
int targetPwm = 0;
bool startFreigegeben = false;
unsigned long lastTotmannSignal = 0;
const unsigned long totmannTimeout = 400;  // Sicherheitsrelevant, 400ms sinnvoller Kompromiss

float scheibenDurchmesser = 7.5;

const int pwmPin = 5;  // GPIO5 (entspricht D5 auf ESP8266)
const int pwmChannel = 0;
const int pwmFreq = 2900;
const int pwmResolution = 10;  // 10 Bit Auflösung (0-1023)
const int pwmRange = 1023;

const int relay1Pin = 18;  // GPIO für Relais 1
const int relay2Pin = 19;  // GPIO für Relais 2


const int n = 11;
float fahrzeit[] = { 38.28, 38.25, 44.73, 51.84, 61.45, 73.56, 94.41, 126.24, 184.87, 531.15, 3000 };
int pwmProzent[] = { 100, 90, 80, 70, 60, 50, 40, 30, 20, 10, 1 };

unsigned long lastRampUpdate = 0;
const unsigned long rampStepDelay = 2;

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
      dutyCycle = map(val, 0, 100, 0, pwmRange);
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
    Serial.println("Start verweigert: Totmann nicht aktiv.");
    server.send(400, "text/plain", "Start verweigert: Totmann nicht aktiv");
    return;
  }

  if (dutyCycle <= 0) {
    Serial.println("Start verweigert: dutyCycle=0.");
    server.send(400, "text/plain", "Start verweigert: dutyCycle=0");
    return;
  }

  startFreigegeben = true;
  targetPwm = dutyCycle;
  Serial.println("Start Befehl erhalten.");
  server.send(200, "text/plain", "Start OK");
}

void handleStop() {
  targetPwm = 0;
  startFreigegeben = false;
  Serial.println("Stop Befehl erhalten.");
  server.send(200, "text/plain", "Stop OK");
}

void setRichtungStop() {
  digitalWrite(relay1Pin, HIGH);
  digitalWrite(relay2Pin, HIGH);
  Serial.println("Richtung: STOP");
}

void handleStatus() {
  bool recentTotmann = (millis() - lastTotmannSignal) < totmannTimeout;

  if (recentTotmann && startFreigegeben) {
    server.send(200, "text/plain", "aktiv");
  } else if (recentTotmann && !startFreigegeben) {
    server.send(200, "text/plain", "warte");
  } else {
    server.send(200, "text/plain", "stop");
  }
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

void handleRichtung() {
  if (server.hasArg("dir")) {
    String dir = server.arg("dir");

    if (dir == "iuz") {
      digitalWrite(relay1Pin, HIGH);
      digitalWrite(relay2Pin, LOW);
      server.send(200, "text/plain", "Richtung: Im Uhrzeigersinn");
      return;
    }

    if (dir == "guz") {
      digitalWrite(relay1Pin, LOW);
      digitalWrite(relay2Pin, HIGH);
      server.send(200, "text/plain", "Richtung: Gegen Uhrzeigersinn");
      return;
    }
  }
}


//////////////////////////////////SETUP/////////////////////////////////////
void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  IPAddress local_IP(192, 168, 1, 1);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);

  Serial.println("AP gestartet. IP-Adresse: " + WiFi.softAPIP().toString());

  ledcAttach(pwmPin, pwmFreq, pwmResolution);
  ledcWrite(pwmChannel, 0);
  pinMode(pwmPin, OUTPUT);

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

  server.begin();
  Serial.println("Webserver gestartet");

  lastTotmannSignal = millis();
}

//////////////////////////////////LOOP/////////////////////////////////////
void loop() {
  server.handleClient();

  unsigned long now = millis();
  if (currentPwm != targetPwm && (now - lastRampUpdate >= rampStepDelay)) {
    if (currentPwm < targetPwm) currentPwm++;
    else if (currentPwm > targetPwm) currentPwm--;

    ledcWrite(pwmChannel, currentPwm);
    lastRampUpdate = now;
  }

  if ((millis() - lastTotmannSignal) > totmannTimeout && startFreigegeben) {
    Serial.println("NOT-AUS: Totmann losgelassen!");
    startFreigegeben = false;
    targetPwm = 0;
    currentPwm = 0;
    ledcWrite(pwmChannel, 0);
    setRichtungStop();
  }
}
