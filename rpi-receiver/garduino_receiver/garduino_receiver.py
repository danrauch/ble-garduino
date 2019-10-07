#!/usr/bin/env python3

from bluepy import btle
from influxdb import InfluxDBClient
import time
import json
from datetime import datetime

ADDRESS = 'F0:F8:F2:C3:6C:35'
CONNECTIVITY_SERVICE_UUID = '0000ffe0-0000-1000-8000-00805f9b34fb'
STORE_DATA_CHARACTERISTICS_UUID = '0000ffe1-0000-1000-8000-00805f9b34fb'
influxdb_client = InfluxDBClient(host='127.0.0.1', port=8086, username='****',
                                 password='****', database='somedbname')


class MyDelegate(btle.DefaultDelegate):
    def __init__(self, params):
        btle.DefaultDelegate.__init__(self)      
        
    def handleNotification(self, cHandle, data):
        json_data = {}
        try:
            json_data = json.loads(data.decode('utf-8'))
        except:
            print("Could not decode JSON - skipping data entrys")
            return
        data_dict = {}     
        for key, data in json_data.items():
            if key == 't':
                data_dict['temperature_celcius_main'] = data
            if key == 'p':
                data_dict['pressure_hpa'] = data
            if key == 'rh':
                data_dict['relative_humidity_percent'] = data
            if key == 't2':
                data_dict['temperature_celcius_sec'] = data
            if key == 's0':
                data_dict['soil_moisture_percent_0'] = data
            if key == 's1':
                data_dict['soil_moisture_percent_1'] = data
            if key == 's2':
                data_dict['soil_moisture_percent_2'] = data
            if key == 'li':
                data_dict['light_intensity_percent'] = data

        current_time = datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')
        json_body = [
            {
                "measurement": "environment_sensor",
                "tags": {
                    "host": "rpi3-wifi",
                    "region": "Deggendorf"
                },
                "time": current_time,
                "fields": data_dict
            }
        ]
        influxdb_client.write_points(json_body)


def init_ble_peripheral():
    peripheral = btle.Peripheral(ADDRESS, btle.ADDR_TYPE_PUBLIC)
    peripheral.setDelegate(MyDelegate(1))
    # services = peripheral.getServiceByUUID(CONNECTIVITY_SERVICE_UUID)
    # characteristics = services.getCharacteristics(forUUID=STORE_DATA_CHARACTERISTICS_UUID)
    return peripheral

def listen_for_ble_data(peripheral):
    while True:
        if peripheral.waitForNotifications(10.0):
            # handleNotification() was called
            continue



if __name__ == "__main__":
    peripheral = init_ble_peripheral()
    listen_for_ble_data(peripheral)
