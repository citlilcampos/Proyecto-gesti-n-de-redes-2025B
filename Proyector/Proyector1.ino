#include <WiFi.h>
#include <Espalexa.h>
#include <HardwareSerial.h>

const char* ssid = "IZZI-5F78";
const char* password = "50A5DC595F78";

#define PIN_TX_NANO 26

HardwareSerial SerialNano(2);
Espalexa espalexa;

EspalexaDevice* d_power;

bool wifiConectado = false;

void enviarComandoNano(String id_comando) {
  Serial.println(">> Enviando al Nano: " + id_comando);
  SerialNano.println(id_comando);
}

// ==== CALLBACK FORMATO ANTIGUO ====
void proyectorPowerChanged(EspalexaDevice* d) {
  uint8_t state = d->getValue();

  if (state > 0) enviarComandoNano("2");  
  else enviarComandoNano("1");
}

void conectarWiFi() {
  Serial.println("Conectando WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.println(WiFi.localIP());

  // === SOLO DISPOSITIVO 1 ===
  d_power = new EspalexaDevice("Proyector", proyectorPowerChanged);
  espalexa.addDevice(d_power);

  espalexa.begin();
  wifiConectado = true;
}

void setup() {
  Serial.begin(115200);
  SerialNano.begin(9600, SERIAL_8N1, 16, PIN_TX_NANO);

  conectarWiFi();
}

void loop() {
  if (wifiConectado) espalexa.loop();
}
