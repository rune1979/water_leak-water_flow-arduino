#!/usr/bin/env python3
import gatt
import time
import re
#import thinkspeak_update as ts

manager = gatt.DeviceManager(adapter_name='hci0')



def pullrequest():
    device = AnyDevice(mac_address='c5:71:ca:af:97:18', manager=manager)
    try:
        device.connect()
        manager.run()
        device.disconnect()
        return text
    except:
        manager.stop()
        device.disconnect()
        return "FAILED"

class AnyDevice(gatt.Device):

    def services_resolved(self):
        super().services_resolved()

        device_information_service = next(
            s for s in self.services
            if s.uuid == '6e400001-b5a3-f393-e0a9-e50e24dcca9e')
        
        
        _characteristic = next(
            c for c in device_information_service.characteristics
            if c.uuid == '6e400003-b5a3-f393-e0a9-e50e24dcca9e')

        _characteristic.enable_notifications()
        _characteristic.read_value()

        
    def characteristic_value_updated(self, characteristic, value):
        #As the returned characteristics are broken we look for a whole match.. 
        match = re.search(r' (.*?)r', str(value))
        if match:
            global text
            text = match.group().strip(' n\\r')
            text = text.split(',')
            time.sleep(2)
            manager.stop()

    def connect_failed(self, error):
        super().connect_failed(error)
        print("[%s] Connection failed: %s" % (self.mac_address, str(error)))
        time.sleep(1)
        device.disconnect()
        device.connect()
