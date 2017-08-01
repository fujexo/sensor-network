#!jinja|yaml

# InfluxDB
influxdb_data_dir:
  file.directory:
    - name: /var/lib/influxdb

influxdb_conf_dir:
  file.directory:
    - name: /etc/influxdb

influxdb_container:
  docker_container.running:
    - name: influxdb
    - image: influxdb
    - watch:
      - file: influxdb_conf
    - ports:
      - 8088
    - port_bindings:
      - 8086:8086
    - binds:
      - /etc/influxdb:/etc/influxdb
      - /var/lib/influxdb:/var/lib/influxdb
      - /etc/localtime:/etc/localtime:ro
    - restart_policy: always
    - detach: True
    - require:
      - file: influxdb_conf_dir
      - file: influxdb_data_dir

# MQTT Broker in Docker
mosquitto_container:
  docker_container.running:
    - name: mosquitto
    - image: eclipse-mosquitto
    - port_bindings:
      - 1883:1883
    - binds:
      - /etc/localtime:/etc/localtime:ro
    - restart_policy: always
    - detach: True

# Dooblr for MQTT > InfluxDB 
dooblr_conf_dir:
  file.directory:
    - name: /etc/dooblr

dooblr_container:
  docker_container.running:
    - name: dooblr
    - image: makerslocal/dooblr
    #- image: dooblr:latest
    - binds:
      - /etc/dooblr:/root/.dooblr
      - /etc/localtime:/etc/localtime:ro
    - restart_policy: always
    - detach: True
    - require:
      - file: dooblr_conf_dir

# Grafana to Display the data
grafana_conf_dir:
  file.directory:
    - name: /etc/grafana

grafana:
  docker_container.running:
    - name: grafana
    - image: grafana/grafana
    - port_bindings:
      - 3000:3000
    - binds:
      - /etc/grafana:/etc/grafana
      - /var/lib/grafana:/var/lib/grafana
      - /etc/localtime:/etc/localtime:ro
    - restart_policy: always
    - detach: True
    - require:
      - file: grafana_conf_dir
