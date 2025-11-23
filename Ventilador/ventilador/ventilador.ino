#include <WiFi.h>
#include <Espalexa.h>
#include <WebServer.h>
#include <Preferences.h>

// Pines de relevadores (activos en LOW)
#define RELAY1  13
#define RELAY2  14
#define RELAY3  27

// Pines de botones
#define BTN3 32
#define BTN2 35
#define BTN1 34

// LED indicador WiFi
#define LED_WIFI 2

// WiFi por defecto (solo si no hay guardado)
const char* default_ssid = "Gestion_de_Redes_2025B";
const char* default_pass = "maquinadefuego69";

// WiFi Access Point (cuando no conecta)
const char* ap_ssid = "CONFIG_VENTILADOR";
const char* ap_password = "12345678";

// Objetos
Espalexa espalexa;
WebServer server(80);
Preferences pref;

// Estado de ventilador
int ventiladorActivo = 0;
int lastBtn1 = HIGH, lastBtn2 = HIGH, lastBtn3 = HIGH;

// -------------------------------------------------------
// Encender/apagar ventiladores
// -------------------------------------------------------
void setVentilador(int num) {

  if (num != 0 && ventiladorActivo != 0 && ventiladorActivo != num) {
    digitalWrite(RELAY1, HIGH);
    digitalWrite(RELAY2, HIGH);
    digitalWrite(RELAY3, HIGH);
    delay(200);
  }

  ventiladorActivo = num;

  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, HIGH);

  switch (num) {
    case 1: digitalWrite(RELAY1, LOW); break;
    case 2: digitalWrite(RELAY2, LOW); break;
    case 3: digitalWrite(RELAY3, LOW); break;
  }
}

// -------------------------------------------------------
// Callbacks Alexa
// -------------------------------------------------------
void ventilador1Changed(uint8_t state) { setVentilador(state ? 1 : 0); }
void ventilador2Changed(uint8_t state) { setVentilador(state ? 2 : 0); }
void ventilador3Changed(uint8_t state) { setVentilador(state ? 3 : 0); }

// -------------------------------------------------------
// Control por botones
// -------------------------------------------------------
void controlBotones() {
  int b1 = digitalRead(BTN1);
  int b2 = digitalRead(BTN2);
  int b3 = digitalRead(BTN3);

  if (b1 == LOW && lastBtn1 == HIGH)
    setVentilador(ventiladorActivo == 3 ? 0 : 3);

  if (b2 == LOW && lastBtn2 == HIGH)
    setVentilador(ventiladorActivo == 2 ? 0 : 2);

  if (b3 == LOW && lastBtn3 == HIGH)
    setVentilador(ventiladorActivo == 1 ? 0 : 1);

  lastBtn1 = b1;
  lastBtn2 = b2;
  lastBtn3 = b3;
}

// -------------------------------------------------------
// Parpadeo LED según estado WiFi
// -------------------------------------------------------
void actualizarLED() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;

  unsigned long intervalo = (WiFi.status() == WL_CONNECTED) ? 150 : 600;

  if (millis() - lastBlink > intervalo) {
    ledState = !ledState;
    digitalWrite(LED_WIFI, ledState);
    lastBlink = millis();
  }
}

// -------------------------------------------------------
// Página web para configuración WiFi
// -------------------------------------------------------
void handleRoot() {
  String html = "<html><body>"
                "<h2>Configurar WiFi del ESP32</h2>"
                "<form action=\"/save\" method=\"POST\">"
                "SSID:<br><input name=\"ssid\" length=32><br>"
                "Password:<br><input name=\"pass\" length=64><br><br>"
                "<input type=\"submit\" value=\"Guardar\">"
                "</form></body></html>";

  server.send(200, "text/html", html);
}

// Guardar credenciales y reiniciar
void handleSave() {
  String ssidNuevo = server.arg("ssid");
  String passNuevo = server.arg("pass");

  pref.begin("wifi", false);
  pref.putString("ssid", ssidNuevo);
  pref.putString("pass", passNuevo);
  pref.end();

  server.send(200, "text/html",
              "<h3>Datos guardados. El ESP32 se reiniciará...</h3>");

  delay(2000);
  ESP.restart();
}

// -------------------------------------------------------
// Iniciar Access Point de configuración
// -------------------------------------------------------
void iniciarAccessPoint() {
  Serial.println("Iniciando modo AP...");
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.print("AP lista. IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
}

// -------------------------------------------------------
// SETUP
// -------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  setVentilador(0);

  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);

  pinMode(LED_WIFI, OUTPUT);

  // Leer credenciales guardadas
  pref.begin("wifi", true);
  String savedSSID = pref.getString("ssid", "");
  String savedPASS = pref.getString("pass", "");
  pref.end();

  // Intentar conexión
  WiFi.mode(WIFI_STA);

  if (savedSSID != "") {
    Serial.println("Intentando conectar a red guardada...");
    WiFi.begin(savedSSID.c_str(), savedPASS.c_str());
  } else {
    Serial.println("Intentando conectar a red por defecto...");
    WiFi.begin(default_ssid, default_pass);
  }

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 8000) {
    delay(250);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado a WiFi!");
    Serial.println(WiFi.localIP());

    espalexa.addDevice("Ventilador 1", ventilador1Changed);
    espalexa.addDevice("Ventilador 2", ventilador2Changed);
    espalexa.addDevice("Ventilador 3", ventilador3Changed);
    espalexa.begin();

  } else {
    Serial.println("\nNo se pudo conectar. Entrando en modo AP...");
    iniciarAccessPoint();
  }
}

// -------------------------------------------------------
// LOOP
// -------------------------------------------------------
void loop() {

  if (WiFi.getMode() == WIFI_AP) {
    server.handleClient();
  }

  if (WiFi.status() == WL_CONNECTED) {
    espalexa.loop();
  }

  controlBotones();
  actualizarLED();
}
