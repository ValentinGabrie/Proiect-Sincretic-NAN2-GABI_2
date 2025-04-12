#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <time.h>

const char* ssid = "Seal_team_6";
const char* password = "D3lt4F0xtr0t";
const char* logicAppUrl = "https://prod-217.westeurope.logic.azure.com:443/workflows/ca9733423bde4bac98fc29601413a63d/triggers/When_a_HTTP_request_is_received/paths/invoke?api-version=2016-10-01&sp=%2Ftriggers%2FWhen_a_HTTP_request_is_received%2Frun&sv=1.0&sig=svn3Mn4LMd1xM77zXUD5-SQHI9EPKz6M9fkxCICl8Ns"; 
const int buttonPin = 14;
const int ledPin = 27;
const int tempSensorPin = 32;
const int floodSensorPin = 33;

WebServer server(80);
Preferences prefs;

bool ledState = false;
bool floodAlertSent = false;
float temperatura = 0.0;
String buttonStatus = "necunoscut";
String floodStatus = "necunoscut";
int lastMsgIndex = 0;

// ========== HTML PAGE ==========
String htmlPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <title>ESP32 - Live Control</title>
  <style>
    body { font-family: Arial, sans-serif; max-width: 700px; margin: auto; padding: 30px; background: #f2f2f2; }
    h2, h3 { color: #333; }
    button, input[type=text] {
      padding: 10px 16px;
      font-size: 15px;
      margin: 5px;
    }
    form { margin-top: 20px; }
    .section { background: #fff; padding: 20px; margin-top: 20px; border-radius: 10px; box-shadow: 0 0 8px rgba(0,0,0,0.1); }
    ul { list-style: none; padding: 0; }
    li { margin-bottom: 8px; background: #eee; padding: 10px; border-radius: 6px; display: flex; justify-content: space-between; align-items: center; }
    .delete-btn {
      background-color: red;
      color: white;
      border: none;
      padding: 4px 10px;
      border-radius: 5px;
      cursor: pointer;
    }
  </style>
</head>
<body>
  <div class="section">
    <h2>Status ESP32</h2>
    <p>Buton Fizic: <span id="buton">...</span></p>
    <p>LED: <span id="led">...</span></p>
    <p>Temperatură: <span id="temp">...</span> °C</p>
    <p>Senzor Apă (valoare brută): <span id="flood">...</span></p>
    <form action="/toggle" method="post">
      <button type="submit">Comută LED-ul</button>
    </form>
  </div>

  <div class="section">
    <h3>Trimite un mesaj</h3>
    <form action="/mesaj" method="post">
      <input type="text" name="mesaj" placeholder="Scrie mesajul..." maxlength="100" required>
      <button type="submit">Trimite</button>
    </form>
  </div>

  <script>
    function updateStatus() {
      fetch('/status')
        .then(response => response.json())
        .then(data => {
          document.getElementById("buton").innerText = data.buton;
          document.getElementById("led").innerText = data.led;
          document.getElementById("led").style.color = data.led === "Pornit" ? "green" : "red";
          document.getElementById("temp").innerText = data.temp;
          document.getElementById("flood").innerText = data.flood;
        });
    }
    setInterval(updateStatus, 1000);
    updateStatus();
  </script>
)rawliteral";
}

// ========== LISTE ==========
// Mesaje
String genereazaListaMesaje() {
  String html = "<div class='section'><h3>Ultimele 10 mesaje:</h3><ul>";
  int idx = prefs.getInt("lastMsgIndex", 0);
  for (int i = 0; i < 10; i++) {
    int realIdx = (idx + i) % 10;
    String key = "msg" + String(realIdx);
    String mesaj = prefs.getString(key.c_str(), "");
    if (mesaj.length() > 0) {
      html += "<li>" + mesaj +
              "<form style='display:inline;' method='get' action='/sterge-msg'>" +
              "<input type='hidden' name='index' value='" + String(realIdx) + "'>" +
              "<button type='submit' class='delete-btn'>Șterge</button></form></li>";
    }
  }
  html += "</ul></div>";
  return html;
}

// Alerte
String genereazaListaAlerte() {
  String html = "<div class='section'><h3>Ultimele 10 alerte de inundație:</h3><ul>";
  int idx = prefs.getInt("lastAlertIndex", 0);
  for (int i = 0; i < 10; i++) {
    int realIdx = (idx + i) % 10;
    String key = "alert" + String(realIdx);
    String alerta = prefs.getString(key.c_str(), "");
    if (alerta.length() > 0) html += "<li>" + alerta + "</li>";
  }
  html += "</ul></div>";
  return html;
}

// ========== FUNCȚII ==========

void salveazaAlerta(int valoare) {
  time_t now = time(NULL);
  struct tm* timeinfo = localtime(&now);
  char timp[20];
  strftime(timp, sizeof(timp), "%H:%M:%S", timeinfo);
  String alerta = String(timp) + " - " + String(valoare);
  int idx = prefs.getInt("lastAlertIndex", 0);
  prefs.putString(("alert" + String(idx)).c_str(), alerta);
  prefs.putInt("lastAlertIndex", (idx + 1) % 10);
}

void trimiteAlertaFlood(int valoare) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(logicAppUrl);
    http.addHeader("Content-Type", "application/json");
    String body = "{\"event\":\"flood\", \"value\":" + String(valoare) + "}";
    http.POST(body);
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n✅ Conectat: " + WiFi.localIP().toString());
  prefs.begin("alerts", false);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  lastMsgIndex = prefs.getInt("lastMsgIndex", 0);

  // === RUTE ===
  server.on("/", []() {
    String pagina = htmlPage();
    pagina += genereazaListaAlerte();
    pagina += genereazaListaMesaje();
    pagina += "</body></html>"; // <-- ESENȚIAL!
    server.send(200, "text/html", pagina);
  });

  server.on("/toggle", HTTP_POST, []() {
    ledState = !ledState;
    digitalWrite(ledPin, ledState ? HIGH : LOW);
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/toggle", HTTP_GET, []() {
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/mesaj", HTTP_POST, []() {
    if (server.hasArg("mesaj")) {
      String mesaj = server.arg("mesaj");
      mesaj.trim();
      if (mesaj.length() > 0) {
        prefs.putString(("msg" + String(lastMsgIndex)).c_str(), mesaj);
        lastMsgIndex = (lastMsgIndex + 1) % 10;
        prefs.putInt("lastMsgIndex", lastMsgIndex);
      }
    }
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/sterge-msg", HTTP_GET, []() {
    if (server.hasArg("index")) {
      int idx = server.arg("index").toInt();
      if (idx >= 0 && idx < 10) {
        prefs.remove(("msg" + String(idx)).c_str());
      }
    }
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/status", []() {
    bool buttonPressed = digitalRead(buttonPin) == HIGH;
    buttonStatus = buttonPressed ? "APĂSAT" : "NEAPĂSAT";

    int floodRaw = analogRead(floodSensorPin);
    floodStatus = String(floodRaw);
    int raw = analogRead(tempSensorPin);
    temperatura = (raw / 4095.0) * 3.3 * 100.0;

    String json = "{\"buton\":\"" + buttonStatus +
                  "\",\"led\":\"" + (ledState ? "Pornit" : "Oprit") +
                  "\",\"temp\":\"" + String(temperatura, 1) +
                  "\",\"flood\":\"" + floodStatus + "\"}";
    server.send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  server.handleClient();

  int floodRaw = analogRead(floodSensorPin);
  if (floodRaw > 3000 && !floodAlertSent) {
    floodAlertSent = true;
    trimiteAlertaFlood(floodRaw);
    salveazaAlerta(floodRaw);
  }
  if (floodRaw < 1000) {
    floodAlertSent = false;
  }
}
