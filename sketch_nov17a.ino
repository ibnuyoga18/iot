
#include <WiFi.h>
#include <WebServer.h>

// ================ WIFI CONFIGURATION ================
const char* ssid = "Mode Pesawat";
const char* password = "aaaaaaaa";

// ================ PIN CONFIGURATION =================
#define TRIG_PIN 19
#define ECHO_PIN 18
#define LDR_PIN 34
#define LED_PIN 2

// ================ SYSTEM CONFIGURATION ==============
const int LIGHT_THRESHOLD = 2000;
const float DISTANCE_THRESHOLD = 100.0;

// ================ VARIABLES =========================
WebServer server(80);

float distance = -1;
int ldrValue = 0;

bool objectDetected = false;
bool isDark = false;
bool ledStatus = false;

// ================ HTML DASHBOARD ===================
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Smart Room Monitoring</title>

<style>
body {
  font-family: Arial;
  background: #eef2f7;
  text-align: center;
  padding: 20px;
}
.container {
  max-width: 500px;
  margin: auto;
}
.card {
  background: white;
  padding: 20px;
  margin: 15px 0;
  border-radius: 12px;
  box-shadow: 0 2px 8px #ccc;
}
.value {
  font-size: 26px;
  font-weight: bold;
  color: #2563eb;
}
</style>
</head>

<body>
<div class="container">

<h2>Smart Room Monitoring</h2>
<p>ESP32 IoT Dashboard</p>

<div class="card">
<h3>Jarak Objek</h3>
<p class="value" id="distance">Loading...</p>
</div>

<div class="card">
<h3>Intensitas Cahaya (ADC)</h3>
<p class="value" id="light">Loading...</p>
</div>

<div class="card">
<h3>Status Objek</h3>
<p class="value" id="object">Loading...</p>
</div>

<div class="card">
<h3>Kondisi Ruangan</h3>
<p class="value" id="room">Loading...</p>
</div>

<div class="card">
<h3>Status LED</h3>
<p class="value" id="led">Loading...</p>
</div>

</div>

<script>
async function updateData() {
  try {
    const response = await fetch('/data');
    const data = await response.json();

    document.getElementById('distance').innerText =
      data.distance < 0 ? 'Tidak terbaca' : data.distance + ' cm';

    document.getElementById('light').innerText =
      data.light;

    document.getElementById('object').innerText =
      data.object ? 'Objek Terdeteksi' : 'Tidak Ada Objek';

    document.getElementById('room').innerText =
      data.dark ? 'Gelap' : 'Terang';

    document.getElementById('led').innerText =
      data.led ? 'MENYALA' : 'MATI';

  } catch (error) {
    console.log('Gagal mengambil data');
  }
}

setInterval(updateData, 1000);
updateData();
</script>

</body>
</html>
)rawliteral";

// ================ ULTRASONIC READING ================
float readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Timeout 30 ms
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return -1;
  }

  float result = duration * 0.0343 / 2;

  return result;
}

// ================ SENSOR PROCESSING =================
void readSensors() {
  distance = readDistance();
  ldrValue = analogRead(LDR_PIN);

  objectDetected =
    (distance > 0 && distance <= DISTANCE_THRESHOLD);

  // Asumsi nilai ADC lebih rendah saat gelap
  isDark = (ldrValue >= LIGHT_THRESHOLD);

  // LED menyala jika ada objek dan kondisi gelap
  ledStatus = objectDetected && isDark;

  digitalWrite(LED_PIN, ledStatus ? HIGH : LOW);
}

// ================ SERIAL MONITOR ====================
void printData() {
  Serial.println("===== SMART ROOM MONITORING =====");

  Serial.print("Jarak Objek: ");

  if (distance < 0) {
    Serial.println("Tidak terbaca");
  } else {
    Serial.print(distance);
    Serial.println(" cm");
  }

  Serial.print("Status Objek: ");
  Serial.println(objectDetected ? "Terdeteksi" : "Tidak Ada");

  Serial.print("Nilai LDR: ");
  Serial.println(ldrValue);

  Serial.print("Kondisi Cahaya: ");
  Serial.println(isDark ? "Gelap" : "Terang");

  Serial.print("Status LED: ");
  Serial.println(ledStatus ? "ON" : "OFF");

  Serial.println("=================================");
}

// ================ WEB SERVER ========================
void handleRoot() {
  server.send_P(200, "text/html", webpage);
}

void handleData() {
  String json = "{";

  json += "\"distance\":" + String(distance, 1) + ",";
  json += "\"light\":" + String(ldrValue) + ",";
  json += "\"object\":" + String(objectDetected ? "true" : "false") + ",";
  json += "\"dark\":" + String(isDark ? "true" : "false") + ",";
  json += "\"led\":" + String(ledStatus ? "true" : "false");

  json += "}";

  server.send(200, "application/json", json);
}

// ================ SETUP =============================
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  pinMode(LDR_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  Serial.println();
  Serial.println("Starting Smart Room Monitoring...");

  // Connect to WiFi
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure web server
  server.on("/", handleRoot);
  server.on("/data", handleData);

  server.begin();

  Serial.println("Web Server Started!");
}

// ================ MAIN LOOP =========================
void loop() {
  server.handleClient();

  static unsigned long lastSensorRead = 0;
  static unsigned long lastPrint = 0;

  // Read sensors every 100 ms
  if (millis() - lastSensorRead >= 100) {
    lastSensorRead = millis();
    readSensors();
  }

  // Print sensor data every 1 second
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    printData();
  }
}
