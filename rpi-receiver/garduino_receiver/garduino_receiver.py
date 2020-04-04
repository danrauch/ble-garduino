#!/usr/bin/env python3

from collections import namedtuple
from datetime import datetime
import time
import json
from json import JSONDecodeError
from influxdb import InfluxDBClient
from bluepy import btle

ValueAndType = namedtuple('ValueAndType', ['value', 'type'])

DEVICE_ADDRESS = 'F0:F8:F2:C3:6C:35'
influxdb_client = InfluxDBClient(host='127.0.0.1', port=8086, username='****',
                                 password='****', database='pyexample')


class ReceivedDataDelegate(btle.DefaultDelegate):

    def __init__(self, params):
        btle.DefaultDelegate.__init__(self)
        # map received ids from Arduino to the corresponding DB table entries and types
        self._valuemap = {'t': ValueAndType('temperature_celcius_main', float),
                          'p': ValueAndType('pressure_hpa', float),
                          'rh': ValueAndType('relative_humidity_percent', int),
                          't2': ValueAndType('temperature_celcius_sec', int),
                          's0': ValueAndType('soil_moisture_percent_0', int),
                          's1': ValueAndType('soil_moisture_percent_1', int),
                          's2': ValueAndType('soil_moisture_percent_2', int),
                          'li': ValueAndType('light_intensity_percent', int),}

    # override handleNotification to handle received data
    def handleNotification(self, hanlde, data):
        try:
            json_data = json.loads(data.decode('utf-8'))
        except JSONDecodeError as e:
            print(f'Could not decode JSON - skipping data entrys. Error {repr(e)}')
            return
        data_dict = {}
        for datakey, datavalue in json_data.items():
            valueandtype = self._valuemap[datakey]
            data_dict[valueandtype.value] = valueandtype.type(datavalue)

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


def receive_ble_data():
    with btle.Peripheral(DEVICE_ADDRESS, btle.ADDR_TYPE_PUBLIC) as peripheral:
        peripheral.setDelegate(ReceivedDataDelegate(1))
        try:
            while True:
                if peripheral.waitForNotifications(10.0):
                    # handleNotification() was called
                    continue
        except KeyboardInterrupt:
            print('exit receiver')


if __name__ == "__main__":
    receive_ble_data()
