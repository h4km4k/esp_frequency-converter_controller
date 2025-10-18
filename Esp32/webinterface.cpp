#include "webinterface.h"

// Dein HTML als Rohstringliteral (PROGMEM optional auf ESP32)
const char webPage[] PROGMEM = R"rawliteral(






<!DOCTYPE html>
<html lang="de">

<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <meta http-equiv="X-UA-Compatible" content="IE=edge" />
  <meta name="application-name" content="Drehscheibe" />
  <meta name="description" content="ESP32 Steuerung der Drehscheibe" />
  <meta name="apple-mobile-web-app-capable" content="yes" />
  <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent" />
  <meta name="mobile-web-app-capable" content="yes">
  <meta name="theme-color" content="#000000">
  <link rel="manifest" href="/manifest.webmanifest">
  <link rel="icon" href="/favicon.png" type="image/png">
  <link rel="apple-touch-icon" href="/favicon.png">
  <title>Drehscheibe Steuerung</title>

  <style>
    html,
    body {
      margin: 0;
      padding: 0;
      font-family: "Segoe UI", Tahoma, sans-serif;
      background: #f4f4f4;
      overscroll-behavior: none;
      overflow-x: hidden;
      box-sizing: border-box;
    }

    .container {
      background: #fff;
      padding: 5px;
      margin: 20px auto;
      border-radius: 12px;
      width: 90%;
      max-width: 500px;
      text-align: center;
    }

    h2 {
      color: #333;
    }

    input[type=range],
    input[type=number],
    select {
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

    .status-aktiv {
      background-color: #4CAF50;
    }

    .status-warte {
      background-color: #FF9800;
    }

    .status-stop {
      background-color: #f44336;
    }

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
      max-width: 100%;
      box-sizing: border-box;
      justify-content: center;
      align-items: center;
      user-select: none;
    }

    #stop:active {
      background-color: #d32f2f;
    }

    #buttonRow {
      margin-top: 20px;
      display: flex;
      max-width: 100%;
      box-sizing: border-box;
      justify-content: space-between;
      gap: 10px;
    }

    #totmann,
    #start {
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
    }

    #zeitContainer button:active {
      background-color: #1976D2;
    }

    .switch {
      position: relative;
      display: inline-block;
      width: 60px;
      height: 34px;
    }

    .switch input {
      display: none;
    }

    .slider {
      position: absolute;
      cursor: pointer;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background-color: #A8DADC;
      transition: 0.4s;
      border-radius: 34px;
    }

    .slider:before {
      position: absolute;
      content: "";
      height: 26px;
      width: 26px;
      left: 4px;
      bottom: 4px;
      background-color: white;
      transition: 0.4s;
      border-radius: 50%;
    }

    input:checked+.slider {
      background-color: #A8DADC;
    }

    input:checked+.slider:before {
      transform: translateX(26px);
    }
  </style>
</head>

<body>
  <div class="container">
    <h2>Drehscheibe Steuerung</h2>
    <p><strong>Drehzahl:</strong> <span id="val">0</span>%</p>
    <input type="range" min="0" max="100" value="0" id="slider" />

    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; gap: 10px;">
      <div>
        <label><strong>Drehrichtung:</strong></label><br />
        <label class="switch">
          <input type="checkbox" id="directionToggle" />
          <span class="slider"></span>
        </label>
        <span id="directionLabel">IUZ</span>
      </div>
      <div>
        <label><strong>Durchmesser:</strong></label><br />
        <label class="switch">
          <input type="checkbox" id="diameterToggle" />
          <span class="slider"></span>
        </label>
        <span id="diameterLabel">7,5m</span>
      </div>
      <div>
        <label><strong>Speed:</strong></label><br />
        <span id="speedDisplay">0,00 m/s</span>
      </div>
    </div>

    <div id="zeitContainer">
      <label for="zeitInput">Zeit/Umdrehung (Sekunden):</label>
      <input type="number" id="zeitInput" min="0" step="0.01" placeholder="z. B. 70" />
      <button id="zeitButton">Zeit übernehmen</button>
    </div>

    <div id="status">Status: Unbekannt</div>


    <div id="stop">⏹ STOP</div>

    <div id="buttonRow">
      <div id="totmann">Totmensch</div>
      <div id="start">▶ Start</div>
    </div>


    <div id="presetControls" style="margin-top: 30px; text-align: left;">
      <label for="presetName" style="font-weight: bold; font-size: 18px;">📝 Preset speichern:</label>
      <input type="text" id="presetName" placeholder="z. B. 1. Fahrt, Bild 1"
        style="width: 100%; padding: 8px; margin-top: 5px; box-sizing: border-box;" />

      <button id="savePreset"
        style="margin-top: 10px; width: 100%; height: 40px; font-size: 18px; font-weight: bold; background-color: #2196F3; color: white; border: none; border-radius: 6px;">
        💾 Preset speichern
      </button>

      <label for="presetSelect" style="display: block; margin-top: 20px; font-weight: bold; font-size: 18px;">📂 Preset
        laden:</label>
      <select id="presetSelect" style="width: 100%; padding: 8px; box-sizing: border-box; margin-top: 5px;">
        <option value="">-- Bitte wählen --</option>
      </select>

      <div style="margin-top: 10px; display: flex; gap: 10px;">
        <div id="playPreset" style="flex: 1; font-size: 16px; background-color: #4CAF50; color: white; border-radius: 6px; height: 36px;
              display: flex; align-items: center; justify-content: center;">
          🔁 Wiedergabe
        </div>

        <button id="deletePreset"
          style="flex: 1; font-size: 16px; background-color: #f44336; color: white; border: none; border-radius: 6px; height: 36px;">
          🗑️ Löschen
        </button>
      </div>

    </div>

    <div style="margin-top: 20px;">
      <button id="downloadPresetsBtn"
        style="width: 100%; height: 40px; font-size: 18px; font-weight: bold; background-color: #2196F3; color: white; border: none; border-radius: 6px;">
        📥 Alle Presets herunterladen (JSON)
      </button>
    </div>

    <div style="margin-top: 20px;">
      <label for="importPresetsInput" style="font-weight: bold; display: block; margin-bottom: 5px;">
        📂 Presets importieren (JSON):
      </label>
      <input type="file" id="importPresetsInput" accept=".json"
        style="width: 100%; padding: 8px; box-sizing: border-box;" />
    </div>
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
    const zeitInput = document.getElementById('zeitInput');
    const zeitButton = document.getElementById('zeitButton');
    const diameterToggle = document.getElementById('diameterToggle');
    const diameterLabel = document.getElementById('diameterLabel');
    const directionToggle = document.getElementById('directionToggle');
    const directionLabel = document.getElementById('directionLabel');
    const presetNameInput = document.getElementById('presetName');
    const savePresetBtn = document.getElementById('savePreset');
    const presetSelect = document.getElementById('presetSelect');
    const playPresetBtn = document.getElementById('playPreset');
    const deletePresetBtn = document.getElementById('deletePreset');

    let directionSentForPress = false;
    let totmannActive = false;
    let totmannTimer = null;
    let scheibenDurchmesser = 7.5;
    let recordingActive = false;
    let recordingStartTime = null;
    let recordingActions = [];
    let currentDirection;
    let currentDuty;
    let dutyCycle = 0;
    let actions;

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

    document.addEventListener('DOMContentLoaded', () => {
      // Lokale Grundwerte setzen
      fetchDutyCycle();
      updateSpeedDisplay();

      diameterToggle.checked = false;
      scheibenDurchmesser = 7.5;
      diameterLabel.textContent = "7,5m";

      directionToggle.checked = false;
      directionLabel.textContent = 'IUZ';

      loadPresets();

      // 🔄 Einmaliger Server-Abgleich beim Start
      fetch('/status')
        .then(r => r.json())
        .then(data => {
          if (data.direction) {
            currentDirection = data.direction;
            directionToggle.checked = (data.direction === 'GUZ');
            directionLabel.textContent = data.direction;
          }
          if (data.duty !== undefined) {
            currentDuty = data.duty;
            slider.value = currentDuty;
            valDisplay.innerText = currentDuty;
            updateSpeedDisplay();
            updateZeitFromDuty(currentDuty);
          }
          console.log('🔄 Initialwerte vom Server übernommen:', currentDirection, currentDuty);
        })
        .catch(() => console.warn('⚠️ Konnte Initialwerte nicht laden.'));
    });


    // Nur Anzeige live updaten, aber erst am Ende senden
    slider.addEventListener('input', (e) => {
      const duty = parseInt(e.target.value);
      if (!isNaN(duty)) {
        valDisplay.innerText = duty;
        dutyCycle = duty;
        updateSpeedDisplay();
        updateZeitFromDuty(duty);
      }
    });

    // Erst wenn Finger/Maus losgelassen wird, senden
    slider.addEventListener('change', (e) => {
      const duty = parseInt(e.target.value);
      if (!isNaN(duty)) {
        fetch('/set', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: 'duty=' + encodeURIComponent(duty)
        }).catch(() => { });
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
          fetch('/set', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: 'duty=' + encodeURIComponent(pwm)
          }).catch(() => { });
        })
        .catch(err => {
          alert("Fehler beim Abrufen der PWM: " + err.message);
        });
    });

    diameterToggle.addEventListener('change', () => {
      scheibenDurchmesser = diameterToggle.checked ? 8.5 : 7.5;
      diameterLabel.textContent = scheibenDurchmesser + "m";
      fetch('/setDiameter', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'd=' + encodeURIComponent(scheibenDurchmesser)
      }).catch(() => { });
      updateSpeedDisplay();
    });

    directionToggle.addEventListener('change', () => {
      const direction = directionToggle.checked ? 'GUZ' : 'IUZ';
      directionLabel.textContent = direction;
      currentDirection = direction;
    });

    // --- TOTMANN-FUNKTION ---

    // --- Signal an Server senden ---
    function sendTotmannSignal() {
      // ➕ Richtung nur einmal pro Tastendruck senden
      if (!directionSentForPress && currentDirection) {
        fetch('/direction', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: 'dir=' + encodeURIComponent(currentDirection.toLowerCase())
        }).catch(() => { });
        directionSentForPress = true; // ab jetzt nicht mehr senden, bis Taste losgelassen
      }
      fetch('/deadman', { method: 'POST' }).catch(() => { });
    }

    // --- Totmann starten ---
    function startTotmann() {
      if (totmannTimer) {
        clearInterval(totmannTimer);
        totmannTimer = null;
      }
      if (totmannActive) return;
      totmannActive = true;
      totmannBtn.classList.add('active');
      directionSentForPress = false;
      sendTotmannSignal();
      totmannTimer = setInterval(sendTotmannSignal, 300);
    }

    // --- Totmann stoppen ---
    function stopTotmann() {
      if (!totmannActive) return;
      totmannActive = false;
      totmannBtn.classList.remove('active');
      fetch('/direction', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'dir=stop'
      }).catch(() => { });
      directionSentForPress = false;
      if (totmannTimer) {
        clearInterval(totmannTimer);
        totmannTimer = null;
      }
      console.log("Totmann losgelassen");
    }

    totmannBtn.addEventListener('pointerdown', startTotmann);
    totmannBtn.addEventListener('pointerup', stopTotmann);
    totmannBtn.addEventListener('pointerleave', stopTotmann);
    totmannBtn.addEventListener('pointercancel', stopTotmann);

    // --- START-BUTTON (Maus + Touch vereint) ---
    startBtn.addEventListener('pointerdown', (e) => {
      e.preventDefault();

      if (!recordingActive) {
        startRecording();
      }

      // Wenn Aufnahme läuft, START-Aktion ins Preset schreiben
      if (recordingActive) {
        recordStart();
      }

      // Start-Kommando an ESP senden
      fetch('/start', { method: 'POST' })
        .then(response => {
          if (!response.ok) throw new Error("Start fehlgeschlagen");
          startBtn.classList.add('active');
          setTimeout(() => startBtn.classList.remove('active'), 500);
        })
        .catch(() => {
          alert("Start fehlgeschlagen. Totmensch aktiv & Drezahl gesetzt?");
          recordingActive = false;
        });
    });


    // --- STOP-BUTTON (Maus + Touch vereint) ---
    stopBtn.addEventListener('pointerdown', (e) => {
      e.preventDefault();

      // 1️⃣ Stop-Aktion ins Preset schreiben
      recordStop();

      // 2️⃣ ESP stoppen
      fetch('/stop', { method: 'POST' }).catch(() => { });

      // 3️⃣ Aufnahme beenden (aber Array behalten)
      stopRecording();
    });


    // Tastaturbedienung
    document.addEventListener('keydown', (e) => {
      const tag = document.activeElement.tagName;
      const id = document.activeElement.id || "";

      // Eingaben blockieren, aber Totmann erlauben
      if (['INPUT', 'TEXTAREA', 'SELECT'].includes(tag) ||
        (tag === 'BUTTON' && id !== 'totmannBtn')) return;

      switch (e.key) {
        case ' ':
          e.preventDefault();

          if (!totmannActive) {
            document.activeElement.blur();
            totmannBtn.dispatchEvent(
              new PointerEvent('pointerdown', { bubbles: true })
            );
          }
          break;

        case 'Enter':
          e.preventDefault();
          startBtn.dispatchEvent(
            new PointerEvent('pointerdown', { bubbles: true })
          );
          break;

        case 'Escape':
          e.preventDefault();
          stopBtn.dispatchEvent(
            new PointerEvent('pointerdown', { bubbles: true })
          );
          break;

        case 's':
          e.preventDefault();
          slider.stepUp();
          slider.dispatchEvent(new Event('input', { bubbles: true }));
          break;

        case 'l':
          e.preventDefault();
          slider.stepDown();
          slider.dispatchEvent(new Event('input', { bubbles: true }));
          break;

        case 'd':
          e.preventDefault();
          if (!totmannActive) {
            directionToggle.checked = !directionToggle.checked;
            directionToggle.dispatchEvent(new Event('change', { bubbles: true }));
          }
          break;
      }
    });

    document.addEventListener('keyup', (e) => {
      if (e.key === ' ') {
        e.preventDefault();
        totmannBtn.dispatchEvent(
          new PointerEvent('pointerup', { bubbles: true })
        );
      }
    });


    // Updaten des Status (nur Anzeige)
    function updateStatus() {
      fetch('/status')
        .then(res => res.json())
        .then(data => {
          statusDiv.classList.remove('status-aktiv', 'status-warte', 'status-stop');
          if (data.status === "aktiv") {
            statusDiv.classList.add('status-aktiv');
            statusDiv.innerText = "✅ Status: AKTIV";
          } else if (data.status === "warte") {
            statusDiv.classList.add('status-warte');
            statusDiv.innerText = "⏳ Status: WARTE AUF START";
          } else {
            statusDiv.classList.add('status-stop');
            statusDiv.innerText = "⏹ Status: STOP";
          }
        })
        .catch(() => {
          statusDiv.classList.add('status-stop');
          statusDiv.innerText = "⛔ Status: Verbindung verloren";
        });
    }


    // Drehzahl einstellen
    function adjustDutyCycle(change) {
      let newDuty = currentDuty + change;
      if (newDuty < 0) newDuty = 0;
      if (newDuty > 100) newDuty = 100;

      currentDuty = newDuty;
      slider.value = newDuty;
      valDisplay.innerText = newDuty;

      fetch('/set', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'value=' + currentDuty
      }).catch(err => console.error('Fehler bei /set:', err));

      updateSpeedDisplay();
      updateZeitFromDuty(newDuty);
    }


    // Aufnahme starten
    function startRecording() {
      recordingActive = true;
      recordingStartTime = Date.now();
      recordingActions = [];
    }

    function recordStart() {

      if (!recordingActive) return;

      const elapsed = Date.now() - recordingStartTime;

      // Falls Werte noch nicht initialisiert sind, aus UI holen
      if (currentDirection == null) {
        currentDirection = directionToggle.checked ? 'GUZ' : 'IUZ';
      }
      if (currentDuty == null) {
        currentDuty = parseInt(slider.value);
      }

      // Aufnahmeaktion hinzufügen
      recordingActions.push({
        type: 'start',
        time: elapsed,
        duty: currentDuty,
        direction: currentDirection
      });

      console.log('📀 Start-Aktion aufgezeichnet:', recordingActions.at(-1));
    }

    // --- Stop-Aktion ins Preset schreiben ---
    function recordStop() {
      if (!recordingActive) return;
      const elapsed = Date.now() - recordingStartTime;
      recordingActions.push({ type: 'stop', time: elapsed });
      console.log('🟢 Stop-Aktion aufgezeichnet:', recordingActions.at(-1));
    }

    // --- Aufnahme beenden ---
    function stopRecording() {
      if (!recordingActive) return;
      recordingActive = false;
      recordingStartTime = null;
      console.log('🟣 Aufnahme gestoppt (Array bleibt erhalten)');
    }

    // Preset speichern
    savePresetBtn.addEventListener('click', () => {
      const name = presetNameInput.value.trim();
      if (!name) {
        alert('Bitte einen gültigen Presetnamen eingeben.');
        return;
      }

      if (localStorage.getItem('preset_' + name)) {
        if (!confirm(`Preset "${name}" existiert bereits. Überschreiben?`)) {
          return;
        }
      }

      if (recordingActions.length === 0) {
        alert('Keine Aktionen zum Speichern vorhanden. Bitte zuerst Start & Stop drücken.');
        return;
      }

      recordingActions.sort((a, b) => a.time - b.time);
      localStorage.setItem('preset_' + name, JSON.stringify(recordingActions));
      loadPresets();
      presetNameInput.value = '';
      alert(`Preset "${name}" gespeichert.`);
      recordingActions = [];
    });

    // Presets laden (Dropdown aktualisieren)
    function loadPresets() {
      presetSelect.innerHTML = '<option value="">-- Preset wählen --</option>';
      for (let i = 0; i < localStorage.length; i++) {
        const key = localStorage.key(i);
        if (key.startsWith('preset_')) {
          const name = key.replace('preset_', '');
          const option = document.createElement('option');
          option.value = name;
          option.textContent = name;
          presetSelect.appendChild(option);
        }
      }
    }


    // --- Drehrichtung nur im UI setzen, nicht senden ---
    presetSelect.addEventListener('change', () => {
      const selected = presetSelect.value;
      if (!selected) return;

      const raw = localStorage.getItem('preset_' + selected);
      if (!raw) return;

      let actions;
      try {
        actions = JSON.parse(raw);
      } catch (err) {
        console.warn("Preset ungültig:", err);
        return;
      }

      // Richtung im UI anzeigen (aber nicht senden!)
      const startAction = actions.find(a => a.type === 'start' && 'direction' in a);
      if (startAction) {
        currentDirection = startAction.direction;
        directionToggle.checked = (currentDirection === 'GUZ');
        directionLabel.textContent = currentDirection;
        console.log("Preset gewählt – Richtung vorbereitet:", currentDirection);
      }
    });


    // --- Preset abspielen – Multitouch-kompatibel ---
    function handlePlayPreset() {
      if (!totmannActive) {
        alert("Start verweigert: Totmensch nicht aktiv?");
        console.warn("Totmannsignal nicht empfangen.");
        return;
      }

      const selected = presetSelect.value;
      if (!selected) {
        alert("Bitte ein Preset auswählen.");
        return;
      }

      const raw = localStorage.getItem('preset_' + selected);
      if (!raw) {
        alert("Preset nicht gefunden.");
        return;
      }

      try {
        actions = JSON.parse(raw);
      } catch (err) {
        alert("Preset ungültig.");
        return;
      }

      playPresetBtn.style.opacity = "0.75";

      startBtn.classList.add('disabled');
      stopBtn.classList.add('disabled');
      directionToggle.disabled = true;

      actions.forEach(action => {
        setTimeout(() => {
          switch (action.type) {
            case 'start':
              if ('direction' in action && currentDirection !== action.direction) {
                currentDirection = action.direction;
                directionToggle.checked = (currentDirection === 'GUZ');
                directionLabel.textContent = currentDirection;

                fetch('/direction', {
                  method: 'POST',
                  headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                  body: 'dir=' + currentDirection
                }).catch(() => { });
              }

              if ('duty' in action) {
                currentDuty = action.duty;
                slider.value = currentDuty;
                valDisplay.innerText = currentDuty;

                fetch('/set', {
                  method: 'POST',
                  headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                  body: 'value=' + currentDuty
                }).catch(() => { });

                updateSpeedDisplay();
                updateZeitFromDuty(currentDuty);
              }

              fetch('/start', { method: 'POST' }).catch(() => { });
              break;

            case 'stop':
              fetch('/stop', { method: 'POST' }).catch(() => { });
              break;
          }
        }, action.time);
      });

      const maxTime = Math.max(...actions.map(a => a.time));
      setTimeout(() => {
        playPresetBtn.style.opacity = "1";
        startBtn.classList.remove('disabled');
        stopBtn.classList.remove('disabled');
        directionToggle.disabled = false;
      }, maxTime + 500);
    }

    playPresetBtn.addEventListener('pointerdown', handlePlayPreset);

    // Presets exportieren
    document.getElementById('downloadPresetsBtn').addEventListener('click', () => {
      const exportObj = {};
      for (let i = 0; i < localStorage.length; i++) {
        const key = localStorage.key(i);
        if (key.startsWith('preset_')) {
          exportObj[key] = localStorage.getItem(key);
        }
      }

      if (Object.keys(exportObj).length === 0) {
        alert("Keine Presets zum Herunterladen vorhanden.");
        return;
      }

      const jsonStr = JSON.stringify(exportObj, null, 2);
      const blob = new Blob([jsonStr], { type: 'application/json' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = 'presets.json';
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url);
    });

    // Presets importieren
    document.getElementById('importPresetsInput').addEventListener('change', (event) => {
      const file = event.target.files[0];
      if (!file) return;

      const reader = new FileReader();
      reader.onload = (e) => {
        try {
          const imported = JSON.parse(e.target.result);
          let count = 0;
          for (const key in imported) {
            if (key.startsWith('preset_')) {
              localStorage.setItem(key, imported[key]);
              count++;
            }
          }
          if (count > 0) {
            loadPresets();
            alert(count + " Preset(s) importiert.");
          } else {
            alert("Keine gültigen Presets gefunden.");
          }
        } catch (err) {
          alert("Fehler beim Import: " + err.message);
        }
      };
      reader.readAsText(file);
    });

    // Preset löschen
    document.getElementById('deletePreset').addEventListener('click', () => {
      const selected = presetSelect.value;
      if (!selected) {
        alert("Bitte ein Preset auswählen.");
        return;
      }

      if (confirm(`Preset "${selected}" wirklich löschen?`)) {
        localStorage.removeItem('preset_' + selected);
        loadPresets();
        alert(`Preset "${selected}" gelöscht.`);
      }
    });


    setInterval(updateStatus, 500);
  </script>
</body>

</html>






)rawliteral";

void handleRoot(WebServer &server) {
  server.send_P(200, "text/html", webPage);
}
