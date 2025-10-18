#ifndef MANIFEST_H
#define MANIFEST_H

const char manifest_json[] PROGMEM = R"rawliteral({
  "name": "Drehscheibe Steuerung",
  "short_name": "Drehscheibe",
  "start_url": "/",
  "scope": "/",
  "display": "standalone",
  "orientation": "portrait",
  "background_color": "#000000",
  "theme_color": "#000000",
  "icons": [
    { "src": "/favicon.png", "sizes": "192x192", "type": "image/png" },
    { "src": "/favicon.png", "sizes": "512x512", "type": "image/png" }
  ]
})rawliteral";

#endif
