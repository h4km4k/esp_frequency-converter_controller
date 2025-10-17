#include <WiFi.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "webinterface.h"

// #include <WiFiUdp.h>
// WiFiUDP udp;
// const unsigned int udpPort = 4210;

AsyncWebServer server(80);

// AsyncWebSocket ws("/ws");  // Endpunkt: ws://<ESP-IP>/ws

// unsigned long lastAlive = 0;  // optional separat tracken

const char *ssid = "ESP-FU-Steuerung";
const char *password = "esp12345";

String currentDirection = "IUZ";  // oder dein Default, z. B. "IUZ" / "GUZ"
int dutyCycle = 0;
int currentPwm = 0;
int currentDuty = 0;  // Startwert 0 oder was du willst
int targetPwm = 0;
bool startFreigegeben = false;
unsigned long lastTotmannSignal = 0;
const unsigned long totmannTimeout = 1000;  // Sicherheitsrelevant, 400ms sinnvoller Kompromiss

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

float getPWMForFahrzeit(float zeit) {
  if (zeit <= fahrzeit[0]) return pwmProzent[0];
  if (zeit >= fahrzeit[n - 1]) return pwmProzent[n - 1];

  for (int i = 0; i < n - 1; i++) {
    float t1 = fahrzeit[i];
    float t2 = fahrzeit[i + 1];

    if (zeit >= t1 && zeit <= t2) {
      float p1 = pwmProzent[i];
      float p2 = pwmProzent[i + 1];
      float ratio = (zeit - t1) / (t2 - t1);
      float pwm = p1 + ratio * (p2 - p1);
      return pwm;
    }
  }

  return pwmProzent[n - 1];
}


// void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
//                AwsEventType type, void *arg, uint8_t *data, size_t len) {
//   if (type == WS_EVT_CONNECT) {
//     Serial.printf("Totmann WebSocket verbunden (Client #%u)\n", client->id());
//   } else if (type == WS_EVT_DISCONNECT) {
//     Serial.printf("Totmann WebSocket getrennt (Client #%u)\n", client->id());
//   } else if (type == WS_EVT_DATA) {
//     // jedes empfangene Paket gilt als "alive"
//     lastTotmannSignal = millis();
//     // Optional Debug:
//     // Serial.printf("WS Totmann-Signal von #%u, len=%u\n", client->id(), len);
//   }
// }


void handleSet(AsyncWebServerRequest *request) {
  if (request->hasParam("duty", true)) {  // true = auch POST-Body prüfen
    int val = request->getParam("duty", true)->value().toInt();

    if (val >= 0 && val <= 100) {
      dutyCycle = map(val, 0, 100, 0, pwmRange);
      request->send(200, "text/plain", "OK");
      return;
    }
  }

  request->send(400, "text/plain", "Bad Request");
}


void handleTotmann(AsyncWebServerRequest *request) {
  lastTotmannSignal = millis();
  request->send(200, "text/plain", "OK");
}


void handleStart(AsyncWebServerRequest *request) {
  bool recentTotmann = (millis() - lastTotmannSignal) < totmannTimeout;

  if (!recentTotmann) {
    Serial.println("Start verweigert: Totmann nicht aktiv.");
    request->send(403, "text/plain", "Totmann nicht aktiv");
    return;
  }

  if (dutyCycle <= 0) {
    Serial.println("Start verweigert: dutyCycle=0.");
    request->send(400, "text/plain", "DutyCycle=0");
    return;
  }

  startFreigegeben = true;
  targetPwm = dutyCycle;

  Serial.println("Start Befehl erhalten.");
  request->send(200, "text/plain", "Start OK");
}


void handleStop(AsyncWebServerRequest *request) {
  targetPwm = 0;
  startFreigegeben = false;
  Serial.println("Stop Befehl erhalten.");
  request->send(200, "text/plain", "Stop OK");
}


void setRichtungStop() {
  digitalWrite(relay1Pin, HIGH);
  digitalWrite(relay2Pin, HIGH);
  Serial.println("Richtung: STOP");
}


void handleStatus(AsyncWebServerRequest *request) {
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

  request->send(200, "application/json", json);
}


void handleGetDuty(AsyncWebServerRequest *request) {
  int dutyPercent = map(dutyCycle, 0, pwmRange, 0, 100);
  request->send(200, "text/plain", String(dutyPercent));
}


void handleGetPwm(AsyncWebServerRequest *request) {
  if (request->hasParam("time")) {
    float zeit = request->getParam("time")->value().toFloat();
    float pwm = getPWMForFahrzeit(zeit);
    request->send(200, "text/plain", String(pwm, 2));
  } else {
    request->send(400, "text/plain", "Missing 'time' parameter");
  }
}


void handleSetDiameter(AsyncWebServerRequest *request) {
  if (request->hasParam("d", true)) {  // true = auch POST-Body prüfen
    String val = request->getParam("d", true)->value();
    if (val == "7.5" || val == "8.5") {
      scheibenDurchmesser = val.toFloat();
      request->send(200, "text/plain", "OK");
      return;
    }
  }
  request->send(400, "text/plain", "Bad Request");
}


void handleRichtung(AsyncWebServerRequest *request) {
  if (request->hasParam("dir", true)) {
    String dir = request->getParam("dir", true)->value();

    if (dir == "iuz") {
      digitalWrite(relay1Pin, HIGH);
      digitalWrite(relay2Pin, LOW);
      request->send(200, "text/plain", "Richtung: Im Uhrzeigersinn");
      return;
    }

    if (dir == "guz") {
      digitalWrite(relay1Pin, LOW);
      digitalWrite(relay2Pin, HIGH);
      request->send(200, "text/plain", "Richtung: Gegen Uhrzeigersinn");
      return;
    }

    if (dir == "stop") {
      digitalWrite(relay1Pin, HIGH);
      digitalWrite(relay2Pin, HIGH);
      request->send(200, "text/plain", "Richtung: Stop");
      return;
    }
  }

  request->send(400, "text/plain", "Bad Request");
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

  // 🚀 Wichtig: WLAN-Schlafmodus ausschalten
  WiFi.setSleep(false);

  Serial.println("AP gestartet. IP-Adresse: " + WiFi.softAPIP().toString());

  // udp.begin(udpPort);
  // Serial.printf("UDP Totmann Empfänger gestartet auf Port %u\n", udpPort);

  ledcAttach(pwmPin, pwmFreq, pwmResolution);
  ledcWrite(pwmChannel, 0);
  pinMode(pwmPin, OUTPUT);

  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  setRichtungStop();

  // // Websocket
  // ws.onEvent(onWsEvent);
  // server.addHandler(&ws);

  // ✅ alle Handler direkt einbinden
  server.on("/", HTTP_GET, handleRoot);
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
  unsigned long now = millis();

  // ws.cleanupClients();  // hält WebSocket-Verbindungen sauber

  // PWM sanft rampen
  if (currentPwm != targetPwm && (now - lastRampUpdate >= rampStepDelay)) {
    if (currentPwm < targetPwm) currentPwm++;
    else if (currentPwm > targetPwm) currentPwm--;

    ledcWrite(pwmChannel, currentPwm);
    lastRampUpdate = now;
  }

  // // --- UDP Totmannsignal prüfen ---
  // int packetSize = udp.parsePacket();
  // if (packetSize) {
  //   char buffer[16];
  //   int len = udp.read(buffer, sizeof(buffer) - 1);
  //   if (len > 0) buffer[len] = '\0';

  //   // Beliebiger Inhalt reicht – jedes Paket gilt als "alive"
  //   lastTotmannSignal = millis();

  //   // Optional Debug:
  //   // Serial.printf("Totmann UDP empfangen von %s, Inhalt: %s\n",
  //   //               udp.remoteIP().toString().c_str(), buffer);
  // }

  // Totmann-Sicherheitslogik
  if ((millis() - lastTotmannSignal) > totmannTimeout && startFreigegeben) {
    Serial.println("NOT-AUS: Totmann losgelassen!");
    startFreigegeben = false;
    targetPwm = 0;
    currentPwm = 0;
    ledcWrite(pwmChannel, 0);
    setRichtungStop();
  }
}
