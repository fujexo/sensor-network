#!/usr/bin/env python3

import json
import logging
import os
import sys
import traceback
from influxdb import InfluxDBClient
import paho.mqtt.client as mqtt

import time
time.sleep(30)

# Stat counter
write_counter = 0

# Logging settings
logging.basicConfig(format='%(asctime)s %(message)s', level=logging.INFO)

# Get settings from environment
if_host = os.environ.get('INFLUX_HOST')
if_port = int(os.environ.get('INFLUX_PORT'))
if_user = os.environ.get('INFLUX_USER')
if_daba = os.environ.get('INFLUX_DABA')
if_pass = os.environ.get('INFLUX_PASS')
mq_host = os.environ.get('MQTT_HOST')
mq_port = int(os.environ.get('MQTT_PORT'))

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

    res = influx_client.write_points(json_body)
#    write_counter += 1
#    if write_counter % 1000 == 0:
#        logging.info('Wrote %s sets of data to influxdb (res: %s)' % (write_counter, res))
    
logging.info('Connecting to influx on %s:%s as %s to db %s' % (if_host, if_port, if_user, if_daba))
influx_client = InfluxDBClient(host=if_host, port=if_port, username=if_user, password=if_pass, database=if_daba)
influx_client.create_database(if_daba)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

logging.info('Connecting to MQTT on %s:%s' % (mq_host, mq_port))
client.connect(mq_host, mq_port, 60)
#client.connect("localhost", 1883, 60)

while True:
    try:
        client.loop_forever()
    except:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        lines = traceback.format_exception(exc_type, exc_value, exc_traceback)
        emsg = ''.join('' + line for line in lines)
        logging.warning('Caught exception in mqtt.Client().loop_forever(): \n%s' % (emsg))

