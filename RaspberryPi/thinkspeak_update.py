import py_gatt_BLE as blue
import requests
import time

def thinkspeak(list_a):
    url = 'https://api.thingspeak.com/update?api_key=YOUR_API_KEY_HERE&field1=' + list_a[0] +'&field2=' + list_a[1]
    requests.get(url)
    

while True:
    to_think = blue.pullrequest()
    if to_think == "FAILED":
        time.sleep(120)
    else:
        thinkspeak(to_think)
    blue.time.sleep(15)
