#!/usr/bin/env python3

import json
import logging
import os
from influxdb import InfluxDBClient
import paho.mqtt.client as mqtt

# InfluxDB settings
influx_host = os.environ['INFLUX_HOST']
influx_port = os.environ['INFLUX_PORT']
influx_user = os.environ['INFLUX_USER']
influx_pass = os.environ['INFLUX_PASS']
influx_daba = os.environ['INFLUX_DABA']

# MQTT settings
mqtt_host = os.environ['MQTT_HOST']
mqtt_port = int(os.environ['MQTT_PORT'])
print(mqtt_host)

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

