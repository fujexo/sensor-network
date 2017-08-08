#!/usr/bin/env python3

import json
import logging
from influxdb import InfluxDBClient
import paho.mqtt.client as mqtt

# InfluxDB settings
influx_host = "localhost"
influx_port = 8086
influx_user = "username"
influx_pass = "password"
influx_daba = "database"

# MQTT settings
mqtt_host = "localhost"
mqtt_port = 1883

# Logging settings
logging.basicConfig(format='%(asctime)s %(message)s', level=logging.INFO)

def on_connect(client, userdata, flags, rc):
    client.subscribe("sysensors/+/temperature")

def on_message(client, userdata, msg):
    json_data = json.loads(msg.payload)

    json_body = [
    {
        "measurement": "sensors_all",
        "tags": {
            "sensor_name": json_data['sensor_name'],
        },
        "fields": {
            "humidity": json_data['humidity'],
            "temperature": json_data['temperature'],
        }
    }
    ]

    logging.info('Writing data to influxdb')
    influx_client.write_points(json_body)
    
influx_client = InfluxDBClient(host=influx_host, port=influx_port, username=influx_user, password=influx_pass, database=influx_daba)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(mqtt_host, mqtt_port, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
while True:
    try:
        client.loop_forever()
    except:
        pass

