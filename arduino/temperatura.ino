#include "DHT.h"                // Librería para facilitar la lectura del sendor DHT22
#include <ESP8266WiFi.h>        // Librería para la conexión WiFI
#include <PubSubClient.h        // Librería para el cliente MQTT

#define DHTTYPE DHT22           // DHT 22  (AM2302), AM2321

const char* ssid = "MiFibra-B8C9";  // SSID de nuestra WiFi
const char* password = "P@ssw0rd";  // Password de la Wifi
const char* mqtt_server = "192.168.1.27";    // Dirección IP de la Raspberry donde está el Broker MQTT

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

// Sensor DHT
uint8_t DHTPin = D2;

// Inicializamos sensor DHT
DHT dht(DHTPin, DHTTYPE);

float Temperatura;
float Humedad;

void setup() {
  // Inicializamos puerto serie para debug
  Serial.begin(115200);
  delay(100);

  // Inicializamos pin sensor
  pinMode(DHTPin, INPUT);
  dht.begin();              

  // Comenzamos conexión a WiFi
  Serial.println("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  //chequeamos conexión a WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  //Mostramos información de la conexión
  Serial.println("");
  Serial.println("WiFi conectada..!");
  Serial.print("IP Asignada: ");  
  Serial.println(WiFi.localIP());

  // Inicializamos cliente MQTT
  client.setServer(mqtt_server, 1883);
}

void reconnect() {
  // Bucle hasta que se reconecte
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");

    // Intento de conexión
    if (client.connect("ESP8266Client")) {
      Serial.println("Conectado");
    } else {
      Serial.print("Error, rc=");
      Serial.print(client.state());
      Serial.println(" reintentando en 5 segundos");
      // Esperamos 5 segundos antes del reintento
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
 
  long now = millis();

  if (now - lastMsg > 300000) {
    lastMsg = now;
    Temperatura = dht.readTemperature();     // Obtenemos valor de la temperatura
    Humedad = dht.readHumidity();	     // Obtenemos valor de la humedad
    snprintf (msg, 75, "%2.1f , %2.1lf", Temperatura, Humedad);
    Serial.print("Publicando mensaje MQTT: ");
    Serial.println(msg);
    client.publish("casa/estacion/sensor1", msg);
  }
}
