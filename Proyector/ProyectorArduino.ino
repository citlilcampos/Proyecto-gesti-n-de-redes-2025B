#include <SPI.h>
#include <UIPEthernet.h> 

// --- ¡¡TUS COMANDOS TELNET REALES!! ---
// Comandos Telnet que el Nano enviará
#define CMD_PROYECTOR_OFF   "C02" // (1) Apagado
#define CMD_PROYECTOR_ON    "C00" // (2) Encendido
#define CMD_PROYECTOR_VOL_UP "C09" // (3) ¡REEMPLAZA ESTO!
#define CMD_PROYECTOR_VOL_DOWN "C0A" // (4) ¡REEMPLAZA ESTO!

// --- Contraseña del Proyector ---
#define PROYECTOR_PASSWORD "0000"
// ------------------------------------------

// --- Pines del Nano ---
#define PIN_CS 10 // Pin CS para el ENC28J60 (Estándar de Arduino)

// --- Configuración de Red (Offline) ---
const char* PROYECTOR_IP = "169.254.100.100";
const int PROYECTOR_PORT = 10000;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress local_IP(169, 254, 100, 101);
IPAddress gateway(169, 254, 0, 1);
IPAddress subnet(255, 255, 0, 0);

EthernetClient ethClient;

// --- Función para conectar y enviar contraseña ---
bool conectarConPassword() {
  Serial.print("Conectando a ");
  Serial.print(PROYECTOR_IP);
  Serial.print("...");
  
  ethClient.stop(); // Asegurarse de cerrar conexiones viejas

  if (ethClient.connect(PROYECTOR_IP, PROYECTOR_PORT)) {
    Serial.println(" ¡Conectado!");
    
    // Paso 2: Enviar la contraseña inmediatamente
    Serial.print("Enviando contraseña (");
    Serial.print(PROYECTOR_PASSWORD);
    Serial.println(")...");
    
    ethClient.print(String(PROYECTOR_PASSWORD) + "\r");
    
    delay(200); // Darle tiempo para procesar
    
    // Vaciar cualquier respuesta
    while(ethClient.available()) {
      ethClient.read();
    }
    return true; // ¡Éxito!
    
  } else {
    Serial.println(" ¡Conexión fallida!");
    return false; // Falló
  }
}

// --- Función de envío de comando ---
void enviarAlProyector(String comando) {
  
  // 1. Asegurarse de que estamos conectados
  if (!ethClient.connected()) {
    Serial.println("¡Conexión perdida! Reconectando con contraseña...");
    
    if (!conectarConPassword()) {
      Serial.println("No se pudo reconectar. Comando abortado.");
      return; 
    }
  }

  // 2. Enviar el comando Telnet
  Serial.print("Enviando comando al Proyector: ");
  Serial.println(comando + "\\r");
  ethClient.print(comando + "\r");
}


void setup() {
  // Inicia Serial a 9600 baudios.
  // El Nano usa los pines D0 (RX) y D1 (TX) para esto.
  Serial.begin(9600); 
  delay(1000);
  Serial.println("\nIniciando Arduino Nano 'Músculo/Traductor' (v2)...");

  // --- Inicialización de Ethernet ---
  Ethernet.init(PIN_CS);
  Ethernet.begin(mac, local_IP, gateway, subnet);
  delay(1000);

  if (Ethernet.hardwareStatus() == EthernetNoHardware) { 
    Serial.println("ERROR: ENC28J60 no encontrado.");
    while (true) { delay(1); } 
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("ERROR: Cable de red (link) no detectado.");
    while (true) { delay(1); } 
  }

  Serial.println("¡Hardware y cable de red OK!");

  // --- Conexión Inicial (con contraseña) ---
  conectarConPassword();
  Serial.println("Listo. Esperando comandos 1-4 del ESP32...");
}

void loop() {
  Ethernet.maintain();

  // --- 1. REVISAR SI EL "CEREBRO" (ESP32) ENVIÓ UN ID ---
  if (Serial.available() > 0) {
    // Leer el ID de comando ("1", "2", "3" o "4")
    String cmd_id = Serial.readStringUntil('\n');
    cmd_id.trim();

    if (cmd_id.length() > 0) {
      Serial.print("Comando ID recibido del ESP32: ");
      Serial.println(cmd_id);
      
      // --- LÓGICA DE TRADUCCIÓN ---
      if (cmd_id == "1") {
        enviarAlProyector(CMD_PROYECTOR_OFF);
      } 
      else if (cmd_id == "2") {
        enviarAlProyector(CMD_PROYECTOR_ON);
      } 
      else if (cmd_id == "3") {
        enviarAlProyector(CMD_PROYECTOR_VOL_UP);
      } 
      else if (cmd_id == "4") {
        enviarAlProyector(CMD_PROYECTOR_VOL_DOWN);
      } 
      else {
        Serial.println("Comando ID no reconocido.");
      }
    }
  }

  // --- 2. REVISAR SI EL PROYECTOR RESPONDIÓ ALGO ---
  if (ethClient.available()) {
    Serial.print("Respuesta del Proyector: ");
    while (ethClient.available()) {
      char c = ethClient.read();
      Serial.write(c); // Imprimir tal cual
    }
    Serial.println(); 
  }
}