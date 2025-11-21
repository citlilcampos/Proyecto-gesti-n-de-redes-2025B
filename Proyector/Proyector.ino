#include <SPI.h>
#include <UIPEthernet.h> // Librería para el ENC28J60

// -----------------------------------------------------------------
// CONFIGURACIÓN DE PINES
// -----------------------------------------------------------------
#define PIN_CS 5 // Pin CS (Chip Select) para el ENC28J60 (GPIO 5)
// -----------------------------------------------------------------


// --- Configuración del Proyector ---
const char* PROYECTOR_IP = "169.254.100.100";
const int PROYECTOR_PORT = 10000;

// --- Configuración de Red del ESP32 ---
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress local_IP(169, 254, 100, 101);
IPAddress gateway(169, 254, 0, 1);
IPAddress subnet(255, 255, 0, 0);

// Objeto cliente (lo mantenemos global)
EthernetClient ethClient;

// Función para intentar conectar (para no repetir código)
void conectarAlProyector() {
  Serial.print("Intentando conectar con el proyector (");
  Serial.print(PROYECTOR_IP);
  Serial.print(")...");

  // Detener cualquier conexión anterior
  ethClient.stop(); 
  
  if (ethClient.connect(PROYECTOR_IP, PROYECTOR_PORT)) {
    Serial.println(" ¡Conectado!");
    Serial.println("Conexión persistente establecida. Listo para comandos.");
    Serial.println("----------------------------------------------");
  } else {
    Serial.println(" ¡Conexión fallida!");
    Serial.println("Reintentando en 5 segundos...");
    delay(5000); // Esperar antes de reintentar
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nIniciando ESP32 con Shield ENC28J60 (Modo Persistente)...");

  // --- Inicialización de Hardware Ethernet ---
  Ethernet.init(PIN_CS);
  Ethernet.begin(mac, local_IP, gateway, subnet);
  delay(1000);

  if (Ethernet.hardwareStatus() == EthernetNoHardware) { 
    Serial.println("ERROR: No se encontró el hardware ENC28J60.");
    Serial.println("Verifica tus pines SPI y el PIN_CS.");
    while (true) { delay(1); } // Detener
  }
  
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("ERROR: El cable de red no está conectado (No hay 'Link').");
    Serial.println("¡Verifica el cable cruzado o el switch!");
    while (true) { delay(1); } // Detener
  }

  Serial.println("¡Hardware y cable de red detectados!");
  Serial.print("IP del ESP32: ");
  Serial.println(Ethernet.localIP());

  // --- Conexión Inicial ---
  // El loop() se encargará de hacer la conexión
}

void loop() {
  // Mantenimiento de red (obligatorio para UIPEthernet)
  Ethernet.maintain();

  // --- PASO 1: REVISAR SI ESTAMOS CONECTADOS ---
  if (!ethClient.connected()) {
    // Si no estamos conectados, intentar conectar
    conectarAlProyector();
    return; // Salir del loop y reintentar en el siguiente ciclo si falló
  }

  // --- PASO 2: REVISAR SI EL PROYECTOR ENVIÓ ALGO ---
  // (Leer la respuesta ANTES de enviar un nuevo comando)
  if (ethClient.available()) {
    Serial.print("Respuesta del Proyector: ");
    while (ethClient.available()) {
      char c = ethClient.read();
      Serial.write(c); // Imprimir carácter por carácter
    }
    Serial.println(); // Salto de línea al final de la respuesta
  }

  // --- PASO 3: REVISAR SI EL USUARIO ENVIÓ ALGO ---
  if (Serial.available() > 0) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();

    if (comando.length() > 0) {
      Serial.print("Enviando Comando: ");
      Serial.println(comando + "\\r");
      
      // Enviar el comando por la conexión ya abierta
      ethClient.print(comando + "\r");
    }
  }
  
  // Pequeña pausa para no saturar el loop
  delay(10); 
}