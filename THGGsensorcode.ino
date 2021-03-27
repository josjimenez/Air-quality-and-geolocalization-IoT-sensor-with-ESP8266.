#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include "WifiLocation.h"

#define DHTTYPE DHT11     // DHT 11
#define GOOGLE_KEY "AIzaSyAZJxnr14cLMIOwbxGUsLMljvKR0Kghr3A" // API Google Geolocation Key
#define LOC_PRECISION 7 // Latitude and longitude precision
       

// Change the credentials below, so your ESP8266 connects to your router
const char* ssid = "Galaxy A51DFBF";
const char* password = "fsxt5176";

// Change the variable to your MQTT Broker, so it connects to it
const char* mqttServer = "iot.fr-par.scw.cloud";
const int mqttPort = 1883;
const char* mqttUser = "b92a3283-28fe-4572-a385-9c680344315e";
const char* mqttPassword = "";

// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espClient;
PubSubClient client(espClient);

// Calling Google's API
WifiLocation location(GOOGLE_KEY);
location_t loc; //Data structure which returns the WifiLocation library

// DHT Sensor - GPIO 5 = D1 on ESP-12E NodeMCU board
const int DHTPin = 5;

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);



// Lamp - LED - GPIO 2 = D4 on ESP-12E NodeMCU board
const int lamp = 2;


// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

// Don't change the function below. This functions connects your ESP8266 to your router
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}


// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something

void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();


  // Feel free to add more if statements to control more GPIOs with MQTT

//  // If a message is received on the topic room/lamp, you check if the message is either on or off. Turns the lamp GPIO according to the message
  if(topic=="room/lamp"){
      Serial.print("Changing Room lamp to ");
      if(messageTemp == "on"){
        digitalWrite(lamp, HIGH);
        Serial.print("On");
         
      }
          
      else if(messageTemp == "off"){
        digitalWrite(lamp, LOW);
        Serial.print("Off");
      }
  }
  Serial.println();
}

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("room/lamp");
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// The setup function sets your ESP GPIOs to Outputs, starts the serial communication at a baud rate of 115200
// Sets your mqtt broker and sets the callback function
// The callback function is what receives messages and actually controls the LEDs
void setup() {
  pinMode(lamp, OUTPUT);
  
  dht.begin();
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP8266Client", mqttUser, mqttPassword );

if (digitalRead(lamp)==HIGH){
  now = millis();
  // Publishes new temperature, humidity, gas level and location every 15 seconds
  if (now - lastMeasure > 15000) {
    lastMeasure = now;

float gassensor=analogRead(A0);
float gas=gassensor/1023*100;

if (isnan(gas))
{
  Serial.println("Error while reading from the MQ-135 sensor");
  return;
}
static char Gass[9];
 dtostrf(gas, 6, 2, Gass);
client.publish("room/gas", Gass);

Serial.print("Gas level:");
Serial.println(gas);
    
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Computes temperature values in Celsius
    float hic = dht.computeHeatIndex(t, h, false);
   static char temperatureTemp[7];
    dtostrf(hic, 6, 2, temperatureTemp);
    
    // Uncomment to compute temperature values in Fahrenheit 
    // float hif = dht.computeHeatIndex(f, h);
    // static char temperatureTemp[7];
    // dtostrf(hic, 6, 2, temperatureTemp);
    
    static char humidityTemp[7];
    dtostrf(h, 6, 2, humidityTemp);

    // Publishes Temperature and Humidity values
    client.publish("room/temperature", temperatureTemp);
    client.publish("room/humidity", humidityTemp);
    
    Serial.print("Humidity: ");
    Serial.println(h);
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println(" *C ");
    Serial.print("Heat index: ");
    Serial.print(hic);
    Serial.println(" *C ");
    // Serial.print(hif);
    // Serial.println(" *F");

   
     
     // We obtain the WiFi Geolocation
  loc = location.getGeoFromWiFi();
    
   static char LatitudeData[7];
    dtostrf(loc.lat, 2, 7, LatitudeData);
   static char LongitudeData[10];
    dtostrf(loc.lon, 2, 7, LongitudeData);

    client.publish("room/latitude", LatitudeData);
    client.publish("room/longitude",LongitudeData);
 
  // We display the info in the serial
  Serial.println("Location request data");
//  Serial.println(location.getSurroundingWiFiJson());
  Serial.println("Latitude: " + String(loc.lat, 7));
  Serial.println("Longitude: " + String(loc.lon, 7));
  Serial.println("Accuracy: " + String(loc.accuracy));
  Serial.println("    Data sent to Scaleway");
   Serial.println("");
  }
}
else {
  now = millis();
  
  if (now - lastMeasure > 2000) {
    lastMeasure = now;

  Serial.println("Measuring and publishing desactivated");
  }
  } 
}
