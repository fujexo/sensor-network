#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// Read settingd from config.h
#include "config.h"

#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print (x)
  #define DEBUG_PRINTLN(x) Serial.println (x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

WiFiClient espClient;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure espClient;

// Initialize MQTT
PubSubClient mqttClient(espClient);

char pub_topic[64];
long lastMsg = 0;
char msg[10000];
int value = 0;
char conv_string[15];

// Logic switches
bool readyToUpload = false;
int valuesCounter = 0;
// Construct to save values over time
int16_t humidityValues[NUM_CACHE];
int16_t temperatureValues[NUM_CACHE];
unsigned long millisValues[NUM_CACHE];

#define SENSORDATA_JSON_SIZE (JSON_ARRAY_SIZE(3) + JSON_OBJECT_SIZE(2) + NUM_CACHE*JSON_OBJECT_SIZE(3))

// Initialize DHT sensor
// NOTE: For working with a faster than ATmega328p 16 MHz Arduino chip, like an ESP8266,
// you need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// This is for the ESP8266 processor on ESP-01
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

void readSensorData() {
  // Reading temperature for humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)

  float tmpTemperature = dht.readTemperature();
  float tmpHumidity = dht.readHumidity();
  // Check if any reads failed and exit early (to try again).
  if (isnan(tmpHumidity) || isnan(tmpHumidity)) {
    DEBUG_PRINTLN("Failed to read from DHT sensor!");
    temperatureValues[valuesCounter] = 0;         // Read temperature as Celcius
    humidityValues[valuesCounter] = 0;          // Read humidity (percent)
  } else {
    DEBUG_PRINTLN("Temperature: " + String(float(tmpTemperature) + TEMP_CORR));
    DEBUG_PRINTLN("Humidity: " + String(float(tmpHumidity) + HUMID_CORR));
    temperatureValues[valuesCounter] = int((tmpTemperature + TEMP_CORR) * 100);         // Read temperature as Celcius
    humidityValues[valuesCounter] = int((tmpHumidity + HUMID_CORR) * 100);          // Read humidity (percent)
  }
  millisValues[valuesCounter] = millis();
}

bool mqttReconnect() {
  // Create a client ID based on the MAC address
  String clientId = String("Sensor-Network") + "-";
  clientId += String(WiFi.macAddress());

    // Loop until we're reconnected
  int counter = 0;
  while (!mqttClient.connected()) {
    counter++;
    if (counter > 5) {
      DEBUG_PRINTLN("Exiting MQTT reconnect loop");
      return false;
    }

    DEBUG_PRINT("Attempting MQTT connection...");

    // Attempt to connect
    if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      DEBUG_PRINTLN("connected");
      // Once connected, publish an announcement...
      //mqttClient.publish("outTopic", "hello world");
      // ... and resubscribe
      //mqttClient.subscribe("inTopic");
      return true;
    } else {
      DEBUG_PRINT("failed, rc=");
      DEBUG_PRINT(mqttClient.state());
      DEBUG_PRINTLN(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
  return false;
}

bool wifiConnect() {
  int retryCounter = CONNECT_TIMEOUT * 10;
  WiFi.forceSleepWake();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.mode(WIFI_STA); //  Force the ESP into client-only mode
  delay(100);
  DEBUG_PRINT("Reconnecting to Wifi ");
  while (WiFi.status() != WL_CONNECTED) {
    retryCounter--;
    if (retryCounter <= 0) {
      DEBUG_PRINTLN(" timeout reached!");
      return false;
    }
    delay(100);
    DEBUG_PRINT(".");
  }
  DEBUG_PRINTLN(" done");
  return true;
}

void setup(void) {
  #ifdef DEBUG
    Serial.begin(SERIAL_BAUD); // initialize serial connection
    // delay for the serial monitor to start
    delay(3000);
  #endif
  dht.begin();          // initialize dht sensor

  DEBUG_PRINTLN("\nDHT Weather Reading Server");
  DEBUG_PRINTLN("My MAC: " + String(WiFi.macAddress()));

  // subscript to the mac address (private) topic
  // char topic[64];
  strcat(pub_topic, "/sensor-network/");
  String clientMac = WiFi.macAddress();
  strcat(pub_topic, clientMac.c_str());
  strcat(pub_topic, "/temperature");
  // mqttClient.subscribe(topic, 1);

  // Start the Pub/Sub client
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  // initializing arrays
  for (int i = 0; i < NUM_CACHE; i++) {
    humidityValues[i] = 0;
    temperatureValues[i] = 0;
    millisValues[i] = 0;
  }

  // Put the Wifi to sleep again
  // WiFi.forceSleepBegin();
  delay(1);
}

void loop(void) {
  // first, get current millis
  // TODO check if there is a buffer overflow with millis. it might get reset
  // after some days
  long now = millis();
  if (now - lastMsg > LOOP_SLEEP) {
    long loopDrift = (now - lastMsg) - LOOP_SLEEP;
    DEBUG_PRINTLN("----------------------------------------------------------");
    DEBUG_PRINTLN("My MAC: " + String(WiFi.macAddress()));
    DEBUG_PRINTLN("Worker loop drift: " + String(loopDrift));
    lastMsg = now;

    readSensorData();       // read sensor
    DEBUG_PRINTLN("Loop index: " + String(valuesCounter + 1));

    // Cache overflow protection
    valuesCounter++; // increase for next loop
    if (valuesCounter >= NUM_CACHE) valuesCounter = 0;

    // e the data once we reached the specified amount of values (or more)
    if (valuesCounter % UPLOAD_EVERY == 0) {
      DEBUG_PRINTLN(">>> It is upload time!");
      readyToUpload = false;

      // Check if the wifi is connected
      if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("Calling wifiConnect() as it seems to be required");
        wifiConnect();
      }

      // MQTT doing its stuff if the wifi is connected
      if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINT("Is the MQTT Client already connected? ");
        if (!mqttClient.connected()) {
          DEBUG_PRINTLN("No, let's try to reconnect");
          if (! mqttReconnect()) {
            // This should not happen, but seems to...
            DEBUG_PRINTLN(" > failed! Exiting the upload loop");
          } else {
            DEBUG_PRINTLN(" > success");
            readyToUpload = true;
          }
        } else {
          DEBUG_PRINTLN("Yes");
          readyToUpload = true;
        }
      }

      // if readyToUpload, letste go!
      if (readyToUpload) {
        DEBUG_PRINTLN("MQTT Loop");
        mqttClient.loop();

        DEBUG_PRINT("Sending the JSON data ");
        for (int i = 0; i <= NUM_CACHE; i++) {
          if (temperatureValues[i] && humidityValues[i] && humidityValues[i] != 0) {
            DEBUG_PRINT(".");
            StaticJsonDocument<SENSORDATA_JSON_SIZE> jsonBuffer;
            jsonBuffer["id"] = String(WiFi.macAddress());
            jsonBuffer["h"] = humidityValues[i];
            jsonBuffer["t"] = temperatureValues[i];
            jsonBuffer["m"] = millisValues[i];
            jsonBuffer["now"] = millis();
            serializeJson(jsonBuffer, msg);
            if (mqttClient.publish(pub_topic, msg)) {
              // Publishing values successful, removing them from cache
              humidityValues[i] = 0;
              temperatureValues[i] = 0;
              millisValues[i] = 0;
            }

            // #ifdef DEBUG
            // DEBUG_PRINTLN("JSON data generated looks like: ");
            // root.printTo(Serial);
            // DEBUG_PRINTLN();
            // #endif
          }
        }
        DEBUG_PRINTLN(" done");
      }

      // Put the Wifi to sleep again
      // WiFi.disconnect();
      // delay(100);
      // WiFi.mode(WIFI_OFF);
      // WiFi.forceSleepBegin();
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
}
