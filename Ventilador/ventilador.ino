#include <WiFi.h>
#include <Espalexa.h>

// Pines de los relevadores (activos en LOW)
#define RELAY1  13
#define RELAY2  14
#define RELAY3  27

// Pines de botones
#define BTN3 32   // Ventilador 1
#define BTN2 35   // Ventilador 2
#define BTN1 34   // Ventilador 3

// Credenciales WiFi
const char* ssid = "Gestion_de_Redes_2025B";
const char* password = "maquinadefuego69";

Espalexa espalexa;

// ---- Estado de los ventiladores ----
int ventiladorActivo = 0;  // 0 = apagado, 1 = V1, 2 = V2, 3 = V3

// Estados anteriores de los botones (para detectar pulsación)
int lastBtn1 = HIGH;
int lastBtn2 = HIGH;
int lastBtn3 = HIGH;

// ---- Función para actualizar relevadores ----
void setVentilador(int num) {
  // Si se quiere cambiar a otro ventilador distinto del activo, apaga todo primero
  if (num != 0 && ventiladorActivo != 0 && ventiladorActivo != num) {
    digitalWrite(RELAY1, HIGH);
    digitalWrite(RELAY2, HIGH);
    digitalWrite(RELAY3, HIGH);
    delay(200); // Pequeña pausa para asegurar apagado
  }

  ventiladorActivo = num;

  // Apagar todos
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, HIGH);

  switch (num) {
    case 1: digitalWrite(RELAY1, LOW); Serial.println("Ventilador 1 encendido"); break;
    case 2: digitalWrite(RELAY2, LOW); Serial.println("Ventilador 2 encendido"); break;
    case 3: digitalWrite(RELAY3, LOW); Serial.println("Ventilador 3 encendido"); break;
    default: Serial.println("Todos los ventiladores apagados"); break;
  }
}

// ---- Callbacks de Alexa ----
void ventilador1Changed(uint8_t state) {
  if (state) setVentilador(1);
  else setVentilador(0);
}

void ventilador2Changed(uint8_t state) {
  if (state) setVentilador(2);
  else setVentilador(0);
}

void ventilador3Changed(uint8_t state) {
  if (state) setVentilador(3);
  else setVentilador(0);
}

// ---- Control local con botones (toggle) ----
void controlBotones() {
  int b1 = digitalRead(BTN1);
  int b2 = digitalRead(BTN2);
  int b3 = digitalRead(BTN3);

  // --- Detección de flanco ---
  if (b1 == LOW && lastBtn1 == HIGH) {
    if (ventiladorActivo == 3) setVentilador(0);  // Si ya estaba encendido, apaga
    else setVentilador(3);                        // Si no, enciende
  }

  if (b2 == LOW && lastBtn2 == HIGH) {
    if (ventiladorActivo == 2) setVentilador(0);
    else setVentilador(2);
  }

  if (b3 == LOW && lastBtn3 == HIGH) {
    if (ventiladorActivo == 1) setVentilador(0);
    else setVentilador(1);
  }

  // Guardar estados anteriores
  lastBtn1 = b1;
  lastBtn2 = b2;
  lastBtn3 = b3;
}

void setup() {
  Serial.begin(115200);

  // Configurar relevadores
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  setVentilador(0); // Estado inicial apagado

  // Configurar botones con pull-up
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);

  // Conexión WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi...");
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado a WiFi");
    Serial.print("IP local: ");
    Serial.println(WiFi.localIP());

    // Registrar dispositivos para Alexa
    espalexa.addDevice("Ventilador 1", ventilador1Changed);
    espalexa.addDevice("Ventilador 2", ventilador2Changed);
    espalexa.addDevice("Ventilador 3", ventilador3Changed);
    espalexa.begin();
  } else {
    Serial.println("\nNo se pudo conectar a WiFi. Se usará solo control local.");
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    espalexa.loop();
  }
  controlBotones();
}
