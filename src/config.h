#ifndef __CONFIG_H
  #define __CONFIG_H

  // Development settings
  #define DEBUG
  #define SERIAL_BAUD     115200

  // Sensor settings
  #define DHTTYPE         DHT22
  #define DHTPIN          2

  // Wifi settings
  #define CLIENT_ID       "oxi_room"
  #define WIFI_SSID       "thunderbluff"
  #define WIFI_PASSWORD   "thrallhall"
  #define MQTT_SERVER     "176.9.120.237"

  // Loop settings
  #define LOOP_SLEEP      2000
  #define WORK_TIMEOUT    10000
#endif
