#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include <WebServer.h>  // ESP32 Webserver-Bibliothek

// Deklaration der Funktion mit ESP32 WebServer Referenz
void handleRoot(WebServer &server);

#endif