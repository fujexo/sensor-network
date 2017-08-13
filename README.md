![Travis Badge](https://travis-ci.org/fujexo/sensor-network.svg?branch=master)

# Code is WIP!!!

Please see the [Wiki](https://github.com/fujexo/sensor-network/wiki) for more information


# Hardware setup
## Wire the DHT22
10kOhm Resistor from VCC to DATA
DATA Pin to Pin D4 on a NodeMCU board

```
   _ _ _ _
  |       |
  | DHT22 |
  ||-|-|-||

VCC NC DATA GND
```

## Flash the code 

edit the settings in src/config.h
Flash the code to the board:
```
platformio run -t upload
```

# Code setup
## Create and edit your configuration
* Clone this repo to /etc/docker/compose/sensor-network
* cd /etc/docker/compose/sensor-network
* cp .env.example .env
* Edit .end to fit your needs

## Ports to open

* 3000 (Grafana - show graphs) HTTP
* 1883 (Mosquitto - incoming data) MQTT


## Configure Grafana

Login to grafana and the template examples/dht22.jason


## Systemd
Based on https://gist.github.com/mosquito/b23e1c1e5723a7fd9e6568e5cf91180f

* ln -s examples/docker-compose.service /etc/systemd/system/docker-compose@.service
* systemctl daemon-reload 
* systemctl enable docker-compose@sensor-network
* systemctl start docker-compose@sensor-network

## Add data source
DS_SENSORS
InfluxDB
URL: http://influxdb:8086

database
username
password
