# -*-coding:UTF-8 -*
# !/usr/bin/env python

""" ITS8050 - Test module for Weather Station

Requirement:
- Equipment: RPi, Sense HAT
- MQTT Broker Account: https://maqiatto.com/
- Python libraries: Paho, SenseHat
 """

# Libs
import paho.mqtt.client as mqtt
import random
import time

import logging
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)
console_handler = logging.StreamHandler()
console_handler.setLevel(logging.DEBUG)
# formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
formatter = logging.Formatter('%(message)s')
console_handler.setFormatter(formatter)
logger.addHandler(console_handler)

# Global Vars
TOPIC_TEMP = "anpino@ttu.ee/senseHAT/temperature"
TOPIC_HUMI = "anpino@ttu.ee/senseHAT/humidity"
TOPIC_PRES = "anpino@ttu.ee/senseHAT/pressure"
QOS = 1
USERNAME = "anpino@ttu.ee"
BROKER_ADDRESS = "maqiatto.com"
PORT = 1883


###############
#  sense_hat   #
###############

def get_temperature(fahrenheit=False):
	temperature = 20
	temperature = round(temperature, 1)
	if fahrenheit:
		temperature = 1.8 * temperature + 32
	return temperature


def get_humidity():
	humidity = random.uniform(30, 30.5)
	return round(humidity, 1)


def get_pressure():
	pressure = random.uniform(10, 10.5)
	return round(pressure, 1)


###############
#  Paho MQTT  #
###############

def on_connect(client, userdata, flags, rc):
	logger.info("[+] Connected.")
	#client.subscribe("anpino@ttu.ee/senseHAT/#")
	client.subscribe(TOPIC_TEMP)
	client.subscribe(TOPIC_HUMI)
	client.subscribe(TOPIC_PRES)

	pass


def on_disconnect(client, userdata, rc):
	if rc != 0:
		logger.info("[!] Unexpected disconnection.")
	else:
		logger.info("[i] Goodbye.")
	pass


def on_message(client, userdata, message):
	m = str(message.payload.decode("utf-8"))
	logger.info(m)


def on_subscribe(client, userdata, mid, granted_qos):
	logger.info("[i] Subscribing to the different topics")
	pass


def init_mqtt():
	# 1. Create a Client Instance
	logger.info("[i] Creating new instance")
	client = mqtt.Client()

	# 2. Attaching client to Callback Functions
	client.on_subscribe = on_subscribe
	client.on_connect = on_connect
	client.on_message = on_message
	client.on_disconnect = on_disconnect
	# client.on_log = on_log
	time.sleep(1)  # Sleep to prevent disorder

	# 3. Input username, password, and pub/sub topics in the terminal
	client.username_pw_set(USERNAME, "MaQiaTTo")

	# 4. Connect to MaQiaTTo Broker in the cloud
	logger.info("[i] Connecting to {} ...".format(BROKER_ADDRESS))
	client.connect(BROKER_ADDRESS, PORT)
	time.sleep(4)  # Sleep to prevent disorder
	return client


def client_loop(client):
	# Client Loop
	client.loop_start()
	try:
		while True:
			t = get_temperature(False)
			h = get_humidity()
			p = get_pressure()

			client.publish(TOPIC_TEMP, t, QOS)
			client.publish(TOPIC_HUMI, h, QOS)
			client.publish(TOPIC_PRES, p, QOS)

			# led_display(sense_hat, t, h, p)

			# output rate in s
			time.sleep(1)

	# exit on ctrl+c
	except KeyboardInterrupt:
		print("[i] You stopped the weather station")

	client.disconnect()
	time.sleep(1)  # wait
	client.loop_stop()  # stop the loop
	pass


def main():
	# 2. init MQTT
	client = init_mqtt()

	# 3. enter MQTT loop
	client_loop(client)
	exit(0)


if __name__ == "__main__":
	main()
	pass
