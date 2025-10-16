#include "webinterface.h"

// Dein HTML als Rohstringliteral (PROGMEM für ESP8266/ESP32)
const char webPage[] PROGMEM = R"rawliteral(



<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Hitachi FU Steuerung</title>
  <style>
    html, body {
      margin: 0;
      padding: 0;
      font-family: "Segoe UI", Tahoma, sans-serif;
      background: #f4f4f4;
      overscroll-behavior: none; /* verhindert Bounce-Effekt auf Mobilgeräten */
    }

    .container {
      background: #fff;
      padding: 30px;
      margin: 30px auto;
      border-radius: 12px;
      width: 90%;
      max-width: 500px;
      text-align: center;
    }

    h2 {
      color: #333;
    }

    input[type=range], input[type=number], select {
      width: 100%;
      margin-top: 20px;
      font-size: 18px;
      padding: 8px;
      box-sizing: border-box;
    }

    #slider {
      touch-action: manipulation;
    }

    #val {
      font-size: 24px;
      font-weight: bold;
      color: #2196F3;
    }

    #status {
      margin-top: 15px;
      font-size: 20px;
      padding: 15px;
      border-radius: 10px;
      background-color: #ccc;
      color: #fff;
      width: 100%;
      box-sizing: border-box;
    }

    .status-aktiv { background-color: #4CAF50; }
    .status-warte { background-color: #FF9800; }
    .status-stop  { background-color: #f44336; }

    #stop {
      margin-top: 20px;
      width: 100%;
      height: 60px;
      font-size: 22px;
      font-weight: bold;
      background-color: #f44336;
      color: white;
      border: none;
      border-radius: 8px;
      display: flex;
      justify-content: center;
      align-items: center;
      touch-action: none;
      user-select: none;
      -webkit-user-select: none;
    }

    #stop:active {
      background-color: #d32f2f;
    }

    #buttonRow {
      margin-top: 20px;
      display: flex;
      justify-content: space-between;
      gap: 10px;
    }

    #totmann, #start {
      width: 48%;
      height: 60px;
      font-size: 20px;
      font-weight: bold;
      border-radius: 8px;
      color: white;
      display: flex;
      align-items: center;
      justify-content: center;
      touch-action: none;
      user-select: none;
      -webkit-user-select: none;
    }

    #totmann {
      background-color: #FFEB3B;
      color: black;
    }

    #totmann.active {
      background-color: #FBC02D;
    }

    #start {
      background-color: #4CAF50;
    }

    #start.active {
      background-color: #388E3C;
    }

    #zeitContainer {
      margin-top: 30px;
      text-align: left;
    }

    #zeitContainer label {
      font-weight: bold;
      font-size: 18px;
    }

    #zeitContainer input[type=number] {
      margin-top: 8px;
      touch-action: manipulation;
    }

    #zeitContainer button {
      margin-top: 10px;
      width: 100%;
      height: 40px;
      font-size: 18px;
      font-weight: bold;
      background-color: #2196F3;
      color: white;
      border: none;
      border-radius: 6px;
      touch-action: manipulation;
    }

    #zeitContainer button:active {
      background-color: #1976D2;
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>Hitachi FU Steuerung</h2>
    <p><strong>Drehzahl:</strong> <span id="val">0</span>%</p>
    <input type="range" min="0" max="100" value="0" id="slider" />

    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; gap: 20px;">
      <div>
        <label for="diameterSelect"><strong>Durchmesser:</strong></label><br />
        <select id="diameterSelect">
          <option value="7.5" selected>7,5m</option>
          <option value="8.5">8,5m</option>
        </select>
      </div>
      <div>
        <label><strong>Geschwindigkeit:</strong></label><br />
        <span id="speedDisplay">0,00 m/s</span>
      </div>
    </div>

    <div id="zeitContainer">
      <label for="zeitInput">Zeit/Umdrehung (Sekunden):</label>
      <input type="number" id="zeitInput" min="0" step="0.01" placeholder="z.B. 70" />
      <button id="zeitButton">Zeit übernehmen</button>
    </div>

    <div id="status">Status: Unbekannt</div>

    <div id="stop">⏹ STOP</div>

    <div id="buttonRow">
      <div id="totmann">Totmann</div>
      <div id="start">▶ Start</div>
    </div>
  </div>

  <script>
    const totmannBtn = document.getElementById('totmann');
    const startBtn = document.getElementById('start');
    const stopBtn = document.getElementById('stop');
    const statusDiv = document.getElementById('status');
    const slider = document.getElementById('slider');
    const valDisplay = document.getElementById('val');
    const speedDisplay = document.getElementById('speedDisplay');
    const diameterSelect = document.getElementById('diameterSelect');
    const zeitInput = document.getElementById('zeitInput');
    const zeitButton = document.getElementById('zeitButton');

    let totmannPressed = false;
    let totmannInterval = null;
    let dutyCycle = 0;
    let scheibenDurchmesser = parseFloat(diameterSelect.value) || 7.5;

    function updateSpeedDisplay() {
      const umdrehzeit100 = 38;
      const rpmMax = 60 / umdrehzeit100;
      const rpm = (dutyCycle / 100) * rpmMax;
      const umfang = Math.PI * scheibenDurchmesser;
      const geschwindigkeit = (umfang * rpm) / 60;
      speedDisplay.innerText = geschwindigkeit.toFixed(3) + " m/s";
    }

    function updateZeitFromDuty(pwm) {
      if (pwm > 0) {
        const umdrehzeit = 38 * (100 / pwm);
        zeitInput.value = umdrehzeit.toFixed(2);
      } else {
        zeitInput.value = "";
      }
    }

    function fetchDutyCycle() {
      fetch('/getDuty')
        .then(response => response.text())
        .then(val => {
          let dutyVal = parseInt(val);
          if (isNaN(dutyVal)) dutyVal = 0;
          slider.value = dutyVal;
          valDisplay.innerText = dutyVal;
          dutyCycle = dutyVal;
          updateSpeedDisplay();
          updateZeitFromDuty(dutyVal);
        })
        .catch(() => {
          slider.value = 0;
          valDisplay.innerText = 0;
          dutyCycle = 0;
          updateSpeedDisplay();
          updateZeitFromDuty(0);
        });
    }

    window.addEventListener('load', () => {
      fetchDutyCycle();
      updateSpeedDisplay();

      diameterSelect.addEventListener('change', () => {
        scheibenDurchmesser = parseFloat(diameterSelect.value);
        fetch('/setDiameter?d=' + scheibenDurchmesser).catch(() => {});
        updateSpeedDisplay();
      });
    });

    slider.addEventListener('input', (e) => {
      const duty = parseInt(e.target.value);
      if (!isNaN(duty)) {
        valDisplay.innerText = duty;
        dutyCycle = duty;
        fetch('/set?duty=' + duty).catch(() => {});
        updateSpeedDisplay();
        updateZeitFromDuty(duty);
      }
    });

    zeitButton.addEventListener('click', () => {
      let zeit = parseFloat(zeitInput.value);
      if (isNaN(zeit) || zeit <= 0) {
        alert("Bitte eine gültige Zeit größer 0 eingeben.");
        return;
      }
      fetch('/getPWMforTime?time=' + encodeURIComponent(zeit))
        .then(response => {
          if (!response.ok) throw new Error("Serverfehler");
          return response.text();
        })
        .then(pwmText => {
          const pwm = parseInt(pwmText);
          if (isNaN(pwm)) throw new Error("Ungültige Serverantwort");
          slider.value = pwm;
          valDisplay.innerText = pwm;
          dutyCycle = pwm;
          updateSpeedDisplay();
//          updateZeitFromDuty(pwm);
          fetch('/set?duty=' + pwm).catch(() => {});
        })
        .catch(err => {
          alert("Fehler beim Abrufen der PWM: " + err.message);
        });
    });

    function sendTotmannSignal() {
      fetch('/totmann').catch(() => {});
    }

    totmannBtn.addEventListener('touchstart', (e) => {
      e.preventDefault();
      if (!totmannPressed) {
        totmannPressed = true;
        totmannBtn.classList.add('active');
        sendTotmannSignal();
        totmannInterval = setInterval(sendTotmannSignal, 200);
      }
    });

    function stopTotmann() {
      if (totmannPressed) {
        totmannPressed = false;
        totmannBtn.classList.remove('active');
        if (totmannInterval !== null) {
          clearInterval(totmannInterval);
          totmannInterval = null;
        }
      }
    }

    totmannBtn.addEventListener('touchend', stopTotmann);
    totmannBtn.addEventListener('touchcancel', stopTotmann);

    startBtn.addEventListener('touchstart', (e) => {
      e.preventDefault();
      fetch('/start')
        .then(response => {
          if (!response.ok) throw new Error("Start fehlgeschlagen");
          startBtn.classList.add('active');
          setTimeout(() => startBtn.classList.remove('active'), 500);
        })
        .catch(() => {
          alert("Start fehlgeschlagen. Totmann aktiv?");
        });
    });

    stopBtn.addEventListener('touchstart', (e) => {
      e.preventDefault();
      fetch('/stop')
        .then(() => {
          startBtn.classList.remove('active');
        })
        .catch(() => {
          alert("Stopp fehlgeschlagen.");
        });
    });

    function updateStatus() {
      fetch('/status')
        .then(response => response.text())
        .then(data => {
          statusDiv.classList.remove('status-aktiv', 'status-warte', 'status-stop');
          let baseText = "";
          if (data === "aktiv") {
            baseText = "✅ Status: AKTIV";
            statusDiv.classList.add('status-aktiv');
          } else if (data === "warte") {
            baseText = "⏳ Status: WARTE AUF START";
            statusDiv.classList.add('status-warte');
          } else {
            baseText = "⏹ Status: STOP";
            statusDiv.classList.add('status-stop');
          }
          statusDiv.innerText = baseText;
        })
        .catch(() => {
          statusDiv.innerText = "⛔ Status: Verbindung verloren";
          statusDiv.classList.add('status-stop');
        });
    }

    setInterval(updateStatus, 500);
  </script>
</body>
</html>





)rawliteral";

void handleRoot(ESP8266WebServer &server) {
  // Sende die gespeicherte HTML-Seite aus dem Flash-Speicher
  server.send_P(200, "text/html", webPage);
}
