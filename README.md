![Travis Badge](https://travis-ci.org/fujexo/sensor-network.svg?branch=master)

# Code is WIP!!!

Please see the [Wiki](https://github.com/fujexo/sensor-network/wiki) for more information



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


## Ports to open

* 3000 (Grafana - show graphs) HTTP
* 8086 (Mosquitto - incoming data) MQTT


## Configure Grafana

Login to grafana and the template grafana/examples/dht22.jason

