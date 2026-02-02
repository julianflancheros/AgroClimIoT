#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h> //Servidor
#include <ESPmDNS.h> //Base de ESP
#include <DHTesp.h> //DHT
#include <BMP280_DEV.h>// Presion - temperatura
#include <OneWire.h> //Configuración de pines Arduino      
#include <DallasTemperature.h> //Lector de temperatura
#include <Firebase_ESP_Client.h>

// Secrets (WiFi + Firebase)
#include "secrets.h"

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

const char* ssid = WIFI_SSID;   //Tu red wifi
const char* password = WIFI_PASSWORD;  //Tu clave wifi
unsigned long sendDataPrevMillis = 0;

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

// Servidor Web en el puerto 80  
WebServer server(80);


// Definición de pines 
//Declaramos variables presión temperatura
float temperature, humidity, pressure, altitude;
float val = 0; //varaible de cambio para el sensor
// Declaramos el variable que almacena el pin a conectar el DHT11
int pinDHT = 26;
bool signupOK = false;
OneWire ourWire(4);                //Se establece el pin 2  como bus OneWire

//Pines para sensores de humendad
//HD-38
const int humsuelo = 33;    //Lectura del sensor
int valHumsuelo;

DallasTemperature sensors(&ourWire); //Se declara una variable u objeto para nuestro sensor
//Instanciamos el DHT
DHTesp dht;
// tipo de varaible del sensor
BMP280_DEV bme;
//Declaramos el HTML del proyecto 

// Pines de salida
const int led_uno = 5;
const int led_dos = 18;


void handleRoot() {
  char msg[3000];

  snprintf(msg, 3000,
           "<html>\
  <head>\
    <meta http-equiv='refresh' content='10'/>\
    <meta name='viewport' content='width=device-width, initial-scale=1'>\
    <link rel='stylesheet' href='https://use.fontawesome.com/releases/v5.7.2/css/all.css' integrity='sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr' crossorigin='anonymous'>\
    <title>ESP32 DHT Server</title>\
    <style>\
    html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center;}\
    h2 { font-size: 3.0rem; }\
    p { font-size: 3.0rem; }\
    .units { font-size: 1.2rem; }\
    .dht-labels{ font-size: 1.5rem; vertical-align:middle; padding-bottom: 15px;}\
    </style>\
  </head>\
  <body>\
      <h2>ESP32 DHT Server!</h2>\
      <p>\
        <i class='fas fa-thermometer-half' style='color:#ca3517;'></i>\
        <span class='dht-labels'>Temperature del ambiente</span>\
        <span>%.2f</span>\
        <sup class='units'>&deg;C</sup>\
      </p>\
      <p>\
        <i class='fas fa-tint' style='color:#00add6;'></i>\
        <span class='dht-labels'>Humedad del ambiente</span>\
        <span>%.2f</span>\
        <sup class='units'>&percnt;</sup>\
      </p>\
      <p>\
        <i class='fas fa-temperature-low' style='color: #855942;'></i>\
        <span class='dht-labels'>Temperatura del suelo </span>\
        <span>%.2f</span>\
        <sup class='units'>&deg;C</sup>\
      </p>\
      <p>\
        <i class='fas fa-temperature-high' style='color: #e7a923;'></i>\
        <span class='dht-labels'>Temperatura del Ambiente BMP </span>\
        <span>%.2f</span>\
        <sup class='units'>&deg;C</sup>\
      </p>\
      <p>\
        <i class='fas fa-weight' style='color: #2e3d56;'></i>\
        <span class='dht-labels'>Pressure </span>\
        <span>%.2f</span>\
        <sup class='units'> hPa </sup>\
      </p>\
      <p>\
        <i class='fas fa-cloud' style='color: #1764e8;'></i>\
        <span class='dht-labels'>Altura a nivel del mar </span>\
        <span>%.2f</span>\
        <sup class='units'> Metros </sup>\
      </p>\
      <p>\
        <i class='fas fa-glass-water-droplet' style='color: #1bde4c;'></i>\
        <span class='dht-labels'>Humedad del suelo </span>\
        <span>%.2f</span>\
        <sup class='units'>&percnt;</sup>\
      </p>\
      <h2>Tabla de valores</h2>\
  </body>\
</html>",
           readDHTTemperature(), readDHTHumidity(),readtemperature(),temperature, pressure, altitude, humedadsuelo()
          );
  server.send(200, "text/html", msg);
}

void setup() {
  //Impreción de datos
  Serial.begin(9600);
  Serial.println("Iniciando wifi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth); 
  Firebase.reconnectWiFi(true);

  if (MDNS.begin("esp32")){
    Serial.println("MDNS responder started");
  }
  server.on("/", handleRoot);

  server.begin();
  Serial.println("HTTP server started");

  //Inicializamos el dht --------------------------------------
  dht.setup(pinDHT, DHTesp::DHT11);
  Serial.println("Sensor de temperatura y humedad iniciado");
  //Inicializamos el sensor presion-temperatura---------------
  bme.begin(BMP280_I2C_ALT_ADDR);
  bme.setTimeStandby(TIME_STANDBY_2000MS);     // Set the standby time to 2 seconds
  bme.startNormalConversion();

  //Sensor de HD-38
  pinMode(humsuelo, INPUT);

  //Pines de salida
  pinMode(led_uno, OUTPUT);
  pinMode(led_dos, OUTPUT);
}

void loop() {
  bme.getMeasurements(temperature, pressure, altitude);
  server.handleClient();
  delay(2);//allow the cpu to switch to other tasks

  //Serial.println(readtemperature());

  if (readtemperature() >= 30){
    digitalWrite(led_uno, HIGH);
  }
  else {
    digitalWrite(led_uno, LOW);
  }

  //Serial.println(humedadsuelo());

  if (humedadsuelo() <= 30){
    digitalWrite(led_dos, HIGH);
  }
  else {
    digitalWrite(led_dos, LOW);
  }



  //sensors.requestTemperatures();   //Se envía el comando para leer la temperatura
  //float temp= sensors.getTempCByIndex(0); //Se obtiene la temperatura en ºC

  //Serial.print("Temperatura= ");
  //Serial.print(temp);
  //Serial.println("°C"3);
  //delay(100);

  //bme.getMeasurements(temperature, pressure, altitude);
  //Serial.println(temperature);
  //Serial.println(pressure);
  //Serial.println(altitude);
  
  //Serial.println(readbmp(1));
  //Serial.println(readbmp(2));
  //Serial.println(readbmp(3));

  //delay(800);

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 24000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    // Write an Int number on the database path test/int
    // Firebase.RTDB.setTimestamp(&fbdo, "timestamp");
    
    // if (Firebase.RTDB.setFloat(&fbdo, "timestamp_it/soilHumedity", moisture_percentage)){
//    if (Firebase.RTDB.setFloat(&fbdo, "timestamp/soilHumedity", moisture_percentage)){
//      Serial.println("PASSED tempe");
//      Serial.println("PATH: " + fbdo.dataPath());
//      Serial.println("TYPE: " + fbdo.dataType());
//    }
//    else {
//      Serial.println("FAILED tempe");
//      Serial.println("REASON: " + fbdo.errorReason());
//    }
    // Sensor de temperatura de ambiente
    // Write an Int number on the database path test/int
    if (Firebase.RTDB.setFloat(&fbdo, "timestamp/temperature", readDHTTemperature())){
      Serial.println("PASSED tempe");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED tempe");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    // Sensor de humedad
    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setFloat(&fbdo, "timestamp/humidity", readDHTHumidity())){
      Serial.println("PASSED humi");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED Humi");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //-------------- Temperatura del suelo---------------------------------
    if (Firebase.RTDB.setFloat(&fbdo, "timestamp/temperature_suelo", readtemperature())){
      Serial.println("PASSED tempe");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED tempe");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //-------------- Temperatura del Ambiente BMP ---------------------------------
    if (Firebase.RTDB.setFloat(&fbdo, "timestamp/temperature_ambiente", temperature)){
      Serial.println("PASSED tempe");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED tempe");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //------------------------- Pressure ------------------------------------------
    if (Firebase.RTDB.setFloat(&fbdo, "timestamp/pressure", pressure)){
      Serial.println("PASSED tempe");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED tempe");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //--------------------- Altura a nivel del mar --------------------------------
    if (Firebase.RTDB.setFloat(&fbdo, "timestamp/altitude", altitude)){
      Serial.println("PASSED tempe");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED tempe");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //------------------------ Humedad del suelo ----------------------------------
    if (Firebase.RTDB.setFloat(&fbdo, "timestamp/humedad_suelo", humedadsuelo())){
      Serial.println("PASSED tempe");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED tempe");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

// ---------------------------------------DHT------------------------------
float readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds
  // Read temperature as Celsius (the default)
  float t = dht.getTemperature();
  if (isnan(t)) {    
    Serial.println("Failed to read from DHT sensor!");
    return -1;
  }
  else {
    //Serial.println(t);
    return t;
  }
}

float readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds
  float h = dht.getHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return -1;
  }
  else {
    //Serial.println(h);
    return h;
  }
}
//----------------------------Temperatura----------------------------------------
float readtemperature() {
  // Sensor readings may also be up to 2 seconds
  // Read temperature as Celsius (the default)
  sensors.requestTemperatures();   //Se envía el comando para leer la temperatura
  float temp= sensors.getTempCByIndex(0); //Se obtiene la temperatura en ºC

  //Serial.print("Temperatura= ");
  //Serial.print(temp);
  //Serial.println(" °C");

  if (isnan(temp)) {    
    Serial.println("Failed to read from  Sensor de temperatura de suelo");
    return -1;
  }
  else {
    //Serial.println(temp);
    return temp;
  }
}

float humedadsuelo(){
  // Sensor readings may also be up to 2 seconds
  // Read temperature as Celsius (the default)
  valHumsuelo = map(analogRead(humsuelo), 4092, 0, 0, 100);

  if (isnan(valHumsuelo)) {
    Serial.println("Failed to read from  Sensor de temperatura de suelo");
    return -1;
  }
  else {
    //Serial.println(valHumsuelo);
    return valHumsuelo;
  }
}
