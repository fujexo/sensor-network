#!/usr/bin/env python3

import json
import logging
import os
import sys
import traceback
from influxdb import InfluxDBClient
import paho.mqtt.client as mqtt

# Logging settings
logging.basicConfig(format='%(asctime)s %(message)s', level=logging.INFO)

class MqttTransport:

    def __init__(self):
        # Config things
        self.pagination = 1000

        # Stat counter
        self.write_counter = 0

        # Get settings from environment
        self.if_host = os.environ.get('INFLUX_HOST', "influxdb")
        self.if_port = int(os.environ.get('INFLUX_PORT', 8086))
        self.if_user = os.environ.get('INFLUX_USER')
        self.if_daba = os.environ.get('INFLUX_DABA')
        self.if_pass = os.environ.get('INFLUX_PASS')
        self.mq_host = os.environ.get('MQTT_HOST', "mosquitto")
        self.mq_port = int(os.environ.get('MQTT_PORT', 1883))

        # Transport clients
        self.influx_client = None
        self.mqtt_client = None

    def on_connect(self, client, userdata, flags, rc):
        client.subscribe("sysensors/+/temperature")

    def setup_mqtt_client(self):
        if not self.mqtt_client:
            self.mqtt_client = mqtt.Client()
            self.mqtt_client.on_connect = self.on_connect
            self.mqtt_client.on_message = self.on_message

            logging.info('Connecting to MQTT on %s:%s' % (self.mq_host,
                                                          self.mq_port))
            self.mqtt_client.connect(self.mq_host, self.mq_port, 60)

    def setup_influx_client(self):
        logging.info('Connecting to influx on %s:%s as %s to db %s' % (self.if_host,
                                                                       self.if_port,
                                                                       self.if_user,
                                                                       self.if_daba))
        self.influx_client = InfluxDBClient(host=self.if_host,
                                            port=self.if_port,
                                            username=self.if_user,
                                            password=self.if_pass,
                                            database=self.if_daba)
        self.influx_client.create_database(self.if_daba)

    def on_message(self, client, userdata, msg):
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

        res = self.influx_client.write_points(json_body)
        self.write_counter += 1
        if self.write_counter % self.pagination == 0:
            logging.info('Wrote %s sets of data to influxdb (res: %s)' % (self.write_counter, res))

    def run(self):
        self.setup_mqtt_client()
        self.setup_influx_client()
        logging.info('Starting to work')

        while True:
            try:
                self.mqtt_client.loop_forever()
            except:
                exc_type, exc_value, exc_traceback = sys.exc_info()
                lines = traceback.format_exception(exc_type, exc_value, exc_traceback)
                emsg = ''.join('' + line for line in lines)
                logging.warning('Caught exception in mqtt.Client().loop_forever(): \n%s' % (emsg))


if __name__ == "__main__":
    transport = MqttTransport()
    transport.run()
