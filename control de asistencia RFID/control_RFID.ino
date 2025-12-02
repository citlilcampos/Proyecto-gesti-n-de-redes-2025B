#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <MFRC522.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ---- CONFIG WIFI ----
const char* ssid     = "holaxd";
const char* password = "0203040506";

String url = "https://0b805475-1f98-4c44-92cc-1339177a059e-00-186g3ktjrizvo.kirk.replit.dev/api/rfid/scan";

// ---- PANTALLA ST7789 ----
#define TFT_CS   5
#define TFT_DC   16
#define TFT_RST  17

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ---- RFID ----
#define RFID_SS   21
#define RFID_RST  22
MFRC522 rfid(RFID_SS, RFID_RST);

// ---- BUZZER & LEDS ----
#define BUZZER_PIN 27
#define LED_VERDE  14
#define LED_ROJO   12

// ---- SONIDOS ----
void beepOk() {
  tone(BUZZER_PIN, 2000, 150);
  delay(200);
  tone(BUZZER_PIN, 2600, 150);
}

void beepError() {
  tone(BUZZER_PIN, 400, 250);
  delay(50);
  tone(BUZZER_PIN, 300, 150);
}

void setup() {
  Serial.begin(115200);

  // Pantalla
  tft.init(240, 240);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);

  tft.println("Iniciando...");

  // Pines buzzer/leds
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, LOW);

  // RFID
  SPI.begin();
  rfid.PCD_Init();
  tft.println("RFID listo");

  // WIFI
  tft.println("Conectando WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  tft.println("WiFi OK");
  delay(500);

  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0,0);
  tft.println("Pasa tu tarjeta");
}

void loop() {

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(20);
    return;
  }

  // UID a string
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  Serial.println("RFID: " + uid);

  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0,0);
  tft.println("Leyendo:");
  tft.println(uid);
  tft.println("");
  tft.println("Enviando...");

  // ---- POST CORREGIDO ----
  WiFiClientSecure client;
  client.setInsecure();  // permite HTTPS sin certificado

  HTTPClient http;
  http.setTimeout(10000);  // 10 segundos
  http.begin(client, url);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "RFID-Tester/1.0");

  String json = "{\"rfidSerial\":\"" + uid + "\"}";

  Serial.println("JSON enviado:");
  Serial.println(json);

  int code = http.POST(json);
  String response = http.getString();

  Serial.println("HTTP CODE: " + String(code));
  Serial.println("RESPUESTA:");
  Serial.println(response);

  // ---- Determinar si correcto ----
  bool aprobado = false;

  if (code == 200 && response.indexOf("success") != -1) {
    aprobado = true;
  }

  // ---- UI + sonido + leds ----
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0,0);

  if (aprobado) {
    tft.setTextColor(ST77XX_GREEN);
    tft.println("ACCESO PERMITIDO");
    tft.println("");
    tft.setTextColor(ST77XX_WHITE);
    tft.println("Bienvenido");
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_ROJO, LOW);
    beepOk();

  } else {
    tft.setTextColor(ST77XX_RED);
    tft.println("ACCESO DENEGADO");
    tft.println("");
    tft.setTextColor(ST77XX_WHITE);
    tft.println(response);
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_ROJO, HIGH);
    beepError();
  }

  http.end();

  delay(2000);

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, LOW);

  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0,0);
  tft.setTextColor(ST77XX_YELLOW);
  tft.println("Pasa tu tarjeta");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}