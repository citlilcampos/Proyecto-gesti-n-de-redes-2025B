#include <WiFi.h>
#include <Espalexa.h>
#include <ESP32Servo.h>

// Pin del servo
#define SERVO_PIN 5

// Credenciales WiFi
const char* ssid = "Lab IICA";
const char* password = "IenIyCA0";

Espalexa espalexa;
Servo servo;

// Parámetros del servo
int stopPoint = 90;
int speedValue = 60;
int vueltasConfig = 12;

// Estado actual: 0 = cerrada, 1 = abierta
int estadoPersiana = 0;

// ---- Función para detener el servo ----
void detenerServo() {
  servo.write(stopPoint);
  Serial.println("Servo detenido");
}

// ---- Función para cerrar persiana (adelante) ----
void cerrarPersiana() {
  if (estadoPersiana == 1) {
    Serial.println("La persiana ya está abierta");
    return;
  }
  
  Serial.println("Abriendo persiana...");
  
  // Girar atrás
  int pwm = stopPoint - speedValue;
  servo.write(pwm);
  
  // Calcular tiempo (700ms por vuelta a velocidad 60)
  int tiempoPorVuelta = 700;
  delay(vueltasConfig * tiempoPorVuelta);
  
  // Detener
  detenerServo();
  estadoPersiana = 1;
  
  Serial.println("Persiana abierta");
}

// ---- Función para abrir persiana (atrás) ----
void abrirPersiana() {
  if (estadoPersiana == 0) {
    Serial.println("La persiana ya está cerrada");
    return;
  }
  
  Serial.println("Cerrando persiana...");
  
  // Girar adelante
  int pwm = stopPoint + speedValue;
  servo.write(pwm);
  
  // Calcular tiempo (700ms por vuelta a velocidad 60)
  int tiempoPorVuelta = 700;
  delay(vueltasConfig * tiempoPorVuelta);
  
  // Detener
  detenerServo();
  estadoPersiana = 0;
  
  Serial.println("Persiana cerrada");
}

// ---- Callback de Alexa ----
void ventanaChanged(uint8_t state) {
  if (state) {
    // Alexa envió "encender" = abrir
    abrirPersiana();
  } else {
    // Alexa envió "apagar" = cerrar
    cerrarPersiana();
  }
}

// ---- Control por Serial ----
void controlSerial() {
  if (Serial.available()) {
    char comando = Serial.read();
    
    switch (comando) {
      case 'f': // forward = cerrar
        cerrarPersiana();
        break;
        
      case 'b': // backward = abrir
        abrirPersiana();
        break;
        
      case 's': // stop inmediato
        detenerServo();
        Serial.println("STOP manual");
        break;
        
      case '+': // aumentar velocidad
        if (speedValue < 90) speedValue += 5;
        Serial.print("Velocidad: ");
        Serial.println(speedValue);
        break;
        
      case '-': // disminuir velocidad
        if (speedValue > 5) speedValue -= 5;
        Serial.print("Velocidad: ");
        Serial.println(speedValue);
        break;
        
      case 'v': // configurar vueltas
        Serial.println("Cuantas vueltas? (1-30):");
        while (!Serial.available());
        vueltasConfig = Serial.parseInt();
        if (vueltasConfig < 1) vueltasConfig = 1;
        if (vueltasConfig > 30) vueltasConfig = 30;
        Serial.print("Vueltas configuradas: ");
        Serial.println(vueltasConfig);
        break;
        
      case '?': // menú
        Serial.println("\n=== MENU ===");
        Serial.println("f - Cerrar persiana");
        Serial.println("b - Abrir persiana");
        Serial.println("s - Stop");
        Serial.println("+ - Aumentar velocidad");
        Serial.println("- - Disminuir velocidad");
        Serial.println("v - Configurar vueltas");
        Serial.println("? - Mostrar menu");
        Serial.print("Estado: ");
        Serial.println(estadoPersiana ? "ABIERTA" : "CERRADA");
        Serial.print("Velocidad: ");
        Serial.print(speedValue);
        Serial.print(" | Vueltas: ");
        Serial.println(vueltasConfig);
        Serial.println("============\n");
        break;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Control de Persiana ESP32 + Alexa ===");
  
  // Configurar servo
  servo.attach(SERVO_PIN);
  detenerServo();
  estadoPersiana = 0; // Estado inicial: cerrada
  
  Serial.println("Servo inicializado");
  Serial.print("Velocidad: ");
  Serial.print(speedValue);
  Serial.print(" | Vueltas: ");
  Serial.println(vueltasConfig);
  
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
    
    // Registrar dispositivo para Alexa
    espalexa.addDevice("Ventana", ventanaChanged);
    espalexa.begin();
    
    Serial.println("Alexa configurada");
    Serial.println("Di: 'Alexa, enciende la ventana' para abrir");
    Serial.println("Di: 'Alexa, apaga la ventana' para cerrar");
  } else {
    Serial.println("\nNo se pudo conectar a WiFi. Se usara solo control serial.");
  }
  
  Serial.println("\nUsa '?' para ver el menu de control\n");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    espalexa.loop();
  }
  
  controlSerial();
}