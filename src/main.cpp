/* test */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "config.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print (x)
  #define DEBUG_PRINTLN(x) Serial.println (x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

#define SENSORDATA_JSON_SIZE (JSON_OBJECT_SIZE(3))
struct SDATA {
   float       humidity;
   float       temperature;
   const char* sensor_name;
} sensor_data = {
  00.0001,
  00.0001,
  CLIENT_ID
};

char pub_topic[64];
long lastMsg = 0;
char msg[128];
int value = 0;
char conv_string[15];

// Initialize DHT sensor
// NOTE: For working with a faster than ATmega328p 16 MHz Arduino chip, like an ESP8266,
// you need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// This is for the ESP8266 processor on ESP-01
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

float humidity, temp_f, temp_c;  // Values read from sensor
// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 2000;              // interval at which to read sensor

void gettemperature() {
  // Wait at least 2 seconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor
    previousMillis = currentMillis;

    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();          // Read humidity (percent)
    temp_f = dht.readTemperature(true);     // Read temperature as Fahrenheit
    temp_c = dht.readTemperature();         // Read temperature as Fahrenheit
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temp_f) || isnan(temp_c)) {
      DEBUG_PRINTLN("Failed to read from DHT sensor!");
      return;
    }
    sensor_data.humidity    = humidity;
    sensor_data.temperature = temp_c;
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  DEBUG_PRINT("Message arrived [");
  DEBUG_PRINT(topic);
  DEBUG_PRINT("] ");
  for (int i = 0; i < length; i++) {
    DEBUG_PRINT((char)payload[i]);
  }
  DEBUG_PRINTLN();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

bool mqttReconnect() {
  // Loop until we're reconnected
  int counter = 0;
  while (!mqttClient.connected()) {
    counter++;
    if (counter > 5) {
      DEBUG_PRINTLN("Exiting MQTT reconnect loop");
      return false;
    }

    DEBUG_PRINT("Attempting MQTT connection...");

    // Create a random client ID
    String clientId = String(CLIENT_ID) + "-";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      DEBUG_PRINTLN("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("outTopic", "hello world");
      // ... and resubscribe
      mqttClient.subscribe("inTopic");
      return true;
    } else {
      DEBUG_PRINT("failed, rc=");
      DEBUG_PRINT(mqttClient.state());
      DEBUG_PRINTLN(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}

bool wifiReconnect() {
  WiFi.forceSleepWake();
  WiFi.begin();
  DEBUG_PRINT("Reconnecting to Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    DEBUG_PRINT(".");
  }
  DEBUG_PRINTLN();
}

void setup(void) {
  #ifdef DEBUG
    Serial.begin(SERIAL_BAUD); // initialize serial connection
  #endif
  dht.begin();          // initialize dht sensor

  // Connect to WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.mode(WIFI_STA); //  Force the ESP into client-only mode
  DEBUG_PRINT("\n\r \n\rWorking to connect");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");
  }

  DEBUG_PRINTLN("\nDHT Weather Reading Server");
  DEBUG_PRINT("Connected to ");
  DEBUG_PRINTLN(WIFI_SSID);
  DEBUG_PRINT("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());

  snprintf( pub_topic, 64, "sysensors/%s/temperature", CLIENT_ID );

  // Start the Pub/Sub client
  mqttClient.setServer(MQTT_SERVER, 1883);
  mqttClient.setCallback(mqttCallback);
}

void loop(void) {
  // first, get current millis
  // TODO: check if there is a buffer overflow with millis. it might get reset
  // after some days
  long now = millis();

  if (now - lastMsg > WORK_TIMEOUT) {
    long loopDrift = (now - lastMsg) - WORK_TIMEOUT;
    DEBUG_PRINTLN("Worker loop drift: " + String(loopDrift));
    lastMsg = now;

    wifiReconnect();

    // MQTT doing its stuff
    if (!mqttClient.connected()) {
      if (! mqttReconnect()) {
        // THis should not happen, but seems to...
        wifiReconnect();
      }
    }
    mqttClient.loop();

    gettemperature();       // read sensor

    StaticJsonBuffer<SENSORDATA_JSON_SIZE> jsonBuffer;
    JsonObject& root    = jsonBuffer.createObject();
    root["humidity"]    = sensor_data.humidity + 0.0001;
    root["sensor_name"] = CLIENT_ID;
    root["temperature"] = sensor_data.temperature + 0.0001;
    root.printTo(msg, 128);

    DEBUG_PRINTLN("Temperature: " + String(sensor_data.temperature));
    DEBUG_PRINTLN("Humidity: " + String(sensor_data.humidity));

    mqttClient.publish(pub_topic, msg);
    WiFi.forceSleepBegin();
    delay(100);
  }

  // calculate how long our current loop took, and fix the delay, so that the
  // drift should be minimized
  long sleep = LOOP_SLEEP - (millis() - now);
  DEBUG_PRINTLN("Sleeping for " + String(sleep)+ " millis");
  if (sleep > 0) {
    delay(sleep);
  }
}
