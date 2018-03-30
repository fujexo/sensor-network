#!/usr/bin/env python3

import yaml
import json
import time
import logging
import paho.mqtt.client as mqtt
import influxdb.client as influx

# Logging settings
logging.basicConfig(format='%(asctime)s %(message)s', level=logging.INFO)

class mqtttransport:

    def __init__(self, config_file):

        self.config_file = config_file

        self.sensors = None
        self.mqtt_topic = None
        self.configuration = None

        # Initialize client variables
        self.mqtt_client = None
        self.influx_client = None


    def read_config(self):
        """ Read the configuration file and save it into variables

        :return: None
        :rtype: bool
        """

        try:
            with open(self.config_file, 'r') as f:
                # Load yaml from file
                t = yaml.load(f)

                # Write to respective variables
                self.sensors = t['sensor_network']['sensors']
                self.configuration = t['sensor_network']['config']

                # Return True if everything is good, false if not
                if not self.sensors is None and not self.configuration is None:
                    return True
                else:
                    return False
        except Exception as e:
            print('Unknown error', e)

    def _setup_mqtt(self):
        """Initialze the mqtt client

        :return: None
        :rtype: None
        """

        # get config and set to default values if neccessary
        mqtt_host = self.configuration.get('mqtt_host', '127.0.0.1')
        mqtt_port = self.configuration.get('mqtt_port', 1883)

        # Initialize the MQTT library if not done yet
        if not self.mqtt_client:
            self.mqtt_client = mqtt.Client()
            self.mqtt_client.on_connect = self.on_connect
            self.mqtt_client.on_message = self.on_message

            # Log that we are going to connect now
            logging.info('Connectiong to MQTT Broker on %s:%s' % (mqtt_host,
                mqtt_port))
            self.mqtt_client.connect(mqtt_host, mqtt_port)

    def _setup_influx(self):
        """Initialize the influx client

        :return: None
        :rtype: None
        """

        influx_host     = self.configuration.get('influx_host', '127.0.0.1')
        influx_port     = self.configuration.get('influx_port', '8086')
        influx_user     = self.configuration.get('influx_user', '')
        influx_pass     = self.configuration.get('influx_pass', '')
        influx_database = self.configuration.get('influx_database', 'default')

        logging.info('Connecting to InfluxDB on %s:%s' % (influx_host,
            influx_port))
        self.influx_client = influx.InfluxDBClient(host=influx_host,
                port=influx_port, username=influx_user,
                password=influx_pass, database=influx_database)

        # Create the default database
        self.influx_client.create_database(influx_database)

        # Create all configured database over all sensors
        for idx, sensor in enumerate(self.sensors):
            database = self.sensors[idx].get('database', None)
            if database is not None:
                self.influx_client.create_database(database)

    def on_connect(self, client, userdata, flags, rc):
        """When the connection to the mqtt broker starts, subscribe to topic

        :return: None
        :rtype: None
        """
        client.subscribe(self.configuration.get('mqtt_topic', 'default'))

    def on_message(self, client, userdata, msg):
        """Exectued on message

        :return: None
        :rtype: None
        """

        logging.debug('Got message')

        msg_payload = json.loads(msg.payload)
        sensor_id = None

        sensor_found = False
        sensor_id = None
        for idx, sensor in enumerate(self.sensors):
            if self.sensors[idx].get('address') == msg_payload['mac_address']:
                sensor_found = True
                sensor_name = self.sensors[idx].get('name')
                sensor_t_corr = self.sensors[idx].get('temp_correction')
                sensor_h_corr = self.sensors[idx].get('humi_correction')
                sensor_database = self.sensors[idx].get('database')

        if not sensor_found:
            sensor_name = msg_payload['mac_address']
            sensor_t_corr = 0
            sensor_h_corr = 0
            sensor_database = self.configuration['influx_database']

        try:
            # Get the actual timestamp of the message
            now = int(time.time() * 1000000000)
            ts = now - int((int(msg_payload['now']) - int(msg_payload['m'])) * 1000000)

            # Prepare the json object for influxdb
            json_body = [
            {
                "measurement": "sensor_network",
                "tags": {
                    "sensor_name": sensor_name,
                    "address": msg_payload['mac_address'],
                    "sensor_db": sensor_database
                },
                "fields": {
                    "humidity": float((msg_payload['h']) / 100) + sensor_h_corr,
                    "temperature": float((msg_payload['t']) / 100) + sensor_t_corr
                },
                "time": ts,
                "time_precision": "u"
            }
            ]

            # Switch database and send the data
            self.influx_client.switch_database(sensor_database)
            res = self.influx_client.write_points(json_body)
        except Exception as e:
            print('Unknown error', e)

    def run(self):
        self.read_config()
        self._setup_mqtt()
        self._setup_influx()

        while True:
            try:
                self.mqtt_client.loop_forever()
            except Exception as e:
                print('Unknown error', e)


if __name__ == "__main__":
    transport = mqtttransport('../test')
    transport.run()


# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
