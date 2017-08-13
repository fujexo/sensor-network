![Travis Badge](https://travis-ci.org/fujexo/sensor-network.svg?branch=master)

# Code is WIP!!!

Please see the [Wiki](https://github.com/fujexo/sensor-network/wiki) for more information


# Hardware setup
## Wire the DHT22
10kOhm Resistor from VCC to DATA
DATA Pin to Pin D4 on a NodeMCU board

```
   _ _ __ _ _
  |          |
  |  DHT 22  |
  |          |
  ||--|--|--||

VCC DATA NC GND
```

## Flash the code to the sensors

Choose the correct board in platform.ini. Examples are "nodemcu" or "d1_mini".
Edit the settings in src/config.h (example in src/config.h.example). Each
sensor has to have a distinguished name which has to match in the Grafana .json).
Flash the code to the board with the "PlatformIO: Upload" button in the gui or
do it via cli with:
```
platformio run -t upload
```


# Code setup
## Create and edit your configuration

* Clone this repo to /etc/docker/compose/sensor-network
* cd /etc/docker/compose/sensor-network
* cp .env.example .env
* Edit .end to fit your needs

## Ports to open if you have use a firewall

* 3000 (Grafana - show graphs) HTTP
* 1883 (Mosquitto - incoming data) MQTT

## Configure Grafana

Login to grafana and the template examples/dht22.jason

## Add data source in Grafana (after logging in with the credentials in .env)

Name: DS_SENSORS (has to match the .jason datasource)
Type: InfluxDB
URL: http://influxdb:8086
Access: proxy
Database: same as INFLUX_DABA in .env
User: same as INFLUX_USER in .env
Password: same as INFLUX_PASS in .env


# Tips and Tricks
## Systemd

Based on https://gist.github.com/mosquito/b23e1c1e5723a7fd9e6568e5cf91180f

* ln -s examples/docker-compose.service /etc/systemd/system/docker-compose@.service
* systemctl daemon-reload
* systemctl enable docker-compose@sensor-network
* systemctl start docker-compose@sensor-network
