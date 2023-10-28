#!/usr/bin/env python3
import requests
import logging
from threading import Timer
from time import sleep
import socketio
import Adafruit_DHT
import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
reader = SimpleMFRC522()
sensor = Adafruit_DHT.DHT11
# sio = socketio.Client()
# sio.connect('http://192.168.0.111:3001')
# urlSensor = 'http://192.168.0.111:3001/user/addSensor'

# sio.connect('http://192.168.0.2:3001')
# urlSensor = 'http://192.168.0.2:3001/user/addSensor'
# sio.connect('http://192.168.137.1:3001')
urlSensor = 'http://192.168.137.1:3001/user/addSensor'
pinDht11 = 4
loopSensor=60
loopRfid=0.25
waitRfid=3

format = "%(asctime)s: %(message)s"
logging.basicConfig(format=format, level=logging.INFO,
                        datefmt="%H:%M:%S")
class RepeatTimer(Timer):
    def run(self):
        while not self.finished.wait(self.interval):
            self.function(*self.args, **self.kwargs)

def readDHT(name):
    humidity, temperature = Adafruit_DHT.read_retry(sensor, pinDht11)
    logging.info("Main    :Temp={0:0.1f}*C Humidity={1:0.1f}%".format(temperature, humidity))
    temp = str(temperature)
    hum = str(humidity)
    sens = {'temp': temp, 'humidity': hum}
    s = requests.post(urlSensor, json = sens)
    logging.info("Main    : %s", s.text)
def readRfid(name):
    try:
            id, text = reader.read_no_block()
            if (id != None):
                hex_id = hex(id)
                hex_id = hex_id[2:-2]
                logging.info("Main    : %s", str(hex_id).upper())
                print(str(hex_id).upper())
                # sio.emit('id', {'id': str(hex_id).upper()})
                sleep(waitRfid)

    finally:
            GPIO.cleanup()
            
# dht = RepeatTimer(loopSensor, readDHT, args=('DHT',))
rfid = RepeatTimer(loopRfid, readRfid, args=('RFID',))
# dht.start()
rfid.start()
