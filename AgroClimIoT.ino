#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHTesp.h>
#include <BMP280_DEV.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Firebase_ESP_Client.h>

// Secrets (WiFi + Firebase)
#include "secrets.h"

// Librerías auxiliares de Firebase
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ==========================================
// CONFIGURACIÓN DEL USUARIO (EDITAR AQUÍ)
// ==========================================

// 1. Configuración de Red
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// 2. Configuración Firebase

// 3. Identificación del Nodo (Importante para el Mapa de Calor)
#define NODE_ID "NODO_AGRO_01" 

// 4. Configuración de Energía (Deep Sleep)
// true = Modo Campo (Envía y duerme, WebServer desactivado)
// false = Modo Pruebas (Siempre encendido, WebServer activo)
const bool MODO_DEEP_SLEEP = false; 
const int TIME_TO_SLEEP_MIN = 15;   // Tiempo en minutos para dormir

// 5. Calibración del Sensor de Humedad de Suelo
// Valores analógicos del ESP32 (0 - 4095)
// RECOMENDACIÓN: Mide el sensor al aire (seco) y pon ese valor en AirValue.
// Luego sumérgelo en agua y pon ese valor en WaterValue.
const int AirValue = 3500;   // Valor cuando está totalmente seco (ajustar)
const int WaterValue = 1000; // Valor cuando está totalmente mojado (ajustar)

// ==========================================
// DEFINICIÓN DE PINES Y OBJETOS
// ==========================================

// Pines
const int pinDHT = 26;       // [cite: 7]
const int pinOneWire = 4;    // [cite: 8]
const int pinHumSuelo = 33;  // [cite: 8]
const int led_uno = 5;       // [cite: 10]
const int led_dos = 18;      // [cite: 11]

// Objetos Globales
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
WebServer server(80);
OneWire ourWire(pinOneWire);
DallasTemperature sensors(&ourWire);
DHTesp dht;
BMP280_DEV bme;

// Variables de sensores
float tempAmbiente, humAmbiente, presion, altitud, tempSuelo;
int porcentajeHumSuelo;
bool wifiConnected = false;
unsigned long sendDataPrevMillis = 0;

// Conversión de tiempo para Deep Sleep
#define uS_TO_S_FACTOR 1000000ULL  

// ==========================================
// FUNCIONES AUXILIARES
// ==========================================

// Lectura de Humedad del Suelo con Calibración
int leerHumedadSuelo() {
  int valAnalog = analogRead(pinHumSuelo);
  // Mapeo inverso: Mayor valor analógico suele ser menor humedad (seco)
  int porcentaje = map(valAnalog, AirValue, WaterValue, 0, 100);
  
  // Limitar los valores entre 0 y 100
  if(porcentaje > 100) porcentaje = 100;
  if(porcentaje < 0) porcentaje = 0;
  
  return porcentaje;
}

// Lectura de Temperatura DS18B20 (Suelo)
float leerTempSuelo() {
  sensors.requestTemperatures();
  float t = sensors.getTempCByIndex(0);
  if (t == DEVICE_DISCONNECTED_C || t < -100) return -127; // Error
  return t;
}

// Conexión WiFi Robusta (Con Timeout)
void conectarWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  
  unsigned long startAttemptTime = millis();
  
  // Intentar conectar por 15 segundos máximo
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado. IP: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
  } else {
    Serial.println("\nFallo al conectar WiFi.");
    wifiConnected = false;
  }
}

// Envío Profesional JSON a Firebase
void enviarDatosFirebase() {
  if (Firebase.ready() && wifiConnected) {
    
    FirebaseJson json;
    
    // Empaquetamos todos los datos en un objeto JSON
    // Esto es vital para que React Native los lea fácil
    json.set("temp_ambiente", tempAmbiente);
    json.set("hum_ambiente", humAmbiente);
    json.set("temp_suelo", tempSuelo);
    json.set("hum_suelo", porcentajeHumSuelo);
    json.set("presion", presion);
    json.set("altitud", altitud);
    // Timestamp del servidor
    json.set("timestamp", "sv-timestamp"); 

    String path = "sensores/" + String(NODE_ID); // Ruta: sensores/NODO_AGRO_01

    Serial.println("Enviando JSON a: " + path);
    
    if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
      Serial.println("¡Datos enviados con éxito! (PASSED)");
      Serial.println("PATH: " + fbdo.dataPath());
    } else {
      Serial.println("Error enviando: " + fbdo.errorReason());
    }
  }
}

// HTML del Servidor Web (Solo funciona si NO está en Deep Sleep)
void handleRoot() {
  char msg[3000];
  snprintf(msg, 3000,
           "<html>\
  <head>\
    <meta http-equiv='refresh' content='10'/>\
    <meta name='viewport' content='width=device-width, initial-scale=1'>\
    <title>ESP32 Agro Monitor</title>\
    <style>html { font-family: Arial; text-align: center;} h2 {font-size: 2rem;} .card {padding: 20px; box-shadow: 2px 2px 12px #aaa; display: inline-block;}</style>\
  </head>\
  <body>\
      <h2>Nodo: %s</h2>\
      <div class='card'>\
        <p>Temp Ambiente: %.2f C</p>\
        <p>Hum Ambiente: %.2f %%</p>\
        <p>Temp Suelo: %.2f C</p>\
        <p>Hum Suelo: %d %%</p>\
        <p>Presion: %.2f hPa</p>\
      </div>\
  </body>\
</html>",
           NODE_ID, tempAmbiente, humAmbiente, tempSuelo, porcentajeHumSuelo, presion
          );
  server.send(200, "text/html", msg);
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(9600);
  
  // 1. Inicializar Sensores
  dht.setup(pinDHT, DHTesp::DHT11);
  sensors.begin();
  bme.begin(BMP280_I2C_ALT_ADDR); // [cite: 26]
  bme.setTimeStandby(TIME_STANDBY_2000MS);
  bme.startNormalConversion();
  pinMode(led_uno, OUTPUT);
  pinMode(led_dos, OUTPUT);
  pinMode(pinHumSuelo, INPUT);

  // 2. Conectar WiFi
  conectarWiFi();

  // 3. Inicializar Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // 4. Iniciar Servidor Web (y mDNS)
  if (MDNS.begin("esp32")) { Serial.println("MDNS started"); }
  server.on("/", handleRoot);
  server.begin();

  // 5. Lectura Inicial de Datos
  // Esperamos un momento a que los sensores se estabilicen
  delay(2000); 
  
  // Leer DHT
  tempAmbiente = dht.getTemperature();
  humAmbiente = dht.getHumidity();
  if (isnan(tempAmbiente)) tempAmbiente = 0.0; // Manejo de error básico

  // Leer BMP280
  bme.getMeasurements(tempAmbiente, presion, altitud); // Nota: BMP sobreescribe tempAmbiente si se desea, o usar variable aparte

  // Leer Sensores de Suelo
  tempSuelo = leerTempSuelo();
  porcentajeHumSuelo = leerHumedadSuelo();

  // Control de Actuadores (Lógica local)
  if (tempSuelo >= 30) digitalWrite(led_uno, HIGH);
  else digitalWrite(led_uno, LOW);

  if (porcentajeHumSuelo <= 30) digitalWrite(led_dos, HIGH);
  else digitalWrite(led_dos, LOW);

  // 6. Lógica de Deep Sleep vs Loop Continuo
  if (MODO_DEEP_SLEEP) {
    // Si estamos en modo campo, enviamos y dormimos
    Serial.println("Modo Agro-IoT: Enviando datos y durmiendo...");
    enviarDatosFirebase();
    
    Serial.printf("Entrando a Deep Sleep por %d minutos...\n", TIME_TO_SLEEP_MIN);
    Serial.flush(); 
    
    // Configurar temporizador de despertar
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_MIN * 60 * uS_TO_S_FACTOR);
    
    // Apagar periféricos que no se usen (opcional para ahorro extremo)
    esp_deep_sleep_start();
  } 
  else {
    Serial.println("Modo Pruebas: Servidor Web Activo. NO se dormirá.");
  }
}

// ==========================================
// LOOP
// ==========================================
void loop() {
  // Si MODO_DEEP_SLEEP es true, nunca llegaremos aquí porque el ESP32 se apaga en el setup.
  
  // Si estamos en modo pruebas (false), mantenemos el servidor web:
  server.handleClient();
  
  // Actualización periódica sin bloquear (cada 10 seg para refrescar sensores en modo pruebas)
  if (millis() - sendDataPrevMillis > 10000) {
    sendDataPrevMillis = millis();
    
    // Releer sensores para la web
    tempAmbiente = dht.getTemperature();
    humAmbiente = dht.getHumidity();
    bme.getMeasurements(tempAmbiente, presion, altitud);
    tempSuelo = leerTempSuelo();
    porcentajeHumSuelo = leerHumedadSuelo();
    
    // Opcional: Enviar a Firebase también en modo pruebas
    enviarDatosFirebase(); 
  }
}