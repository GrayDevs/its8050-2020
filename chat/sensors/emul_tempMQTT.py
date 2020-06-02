# -*-coding:UTF-8 -*
# !/usr/bin/env python

""" ITS8050 - Weather Station

This module gather sensor data from the senseHat and pull them to a cloud MQTT broker.
In our case, we use MaQiaTTo because it is free and easy to setup, but other are available (cloudMQTT, or directly community broker like mqtt.eclipse.com).

Requirement:
- Equipment: RPi, Sense HAT
- MQTT Broker Account: https://maqiatto.com/
- Python libraries: Paho, SenseHat
 """

# Libs
import paho.mqtt.client as mqtt
from sense_emu import SenseHat
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
# TOPIC = "anpino@ttu.ee/senseHAT/#"
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

def _get_temperature(sense_emu: SenseHat, fahrenheit=False):
	temperature = sense_emu.temperature # By default in Celcius
	temperature = round(temperature, 1)
	if fahrenheit:
		temperature = 1.8 * temperature + 32
	return temperature


def _get_humidity(sense_emu):
	humidity = sense_emu.humidity
	return round(humidity, 1)


def _get_pressure(sense_emu):
	pressure = sense_emu.pressure
	return round(pressure, 1)


# TODO: use **kwargs instead of passing all these attributes
def _led_display(sense_hat, temperature, humidity, pressure):
	""" Lights up the senseHAT LEDS to display temp, humidity, and pressure.
	These vars are float so if we concatenate (+) we need to convert them into string first (str()).
	"""
	sense_hat.show_message("T:" + str(temperature) + "C"
	                       + "H:" + str(humidity)
	                       + "P:" + str(pressure),
	                       scroll_speed=0.08,
	                       back_colour=[0, 0, 200])


###############
#  Paho MQTT  #
###############

def on_connect(client, userdata, flags, rc):
	logger.info("[+] Connected.")

	# just to check what we are sending
	client.subscribe(TOPIC_TEMP)
	client.subscribe(TOPIC_HUMI)
	client.subscribe(TOPIC_PRES)
	#client.subscribe(TOPIC)
	pass


def on_disconnect(client, userdata, rc):
	if rc != 0:
		logger.info("[!] Unexpected disconnection.")
	else:
		logger.info("[i] Goodbye.")
	pass


def on_message(client, userdata, message):
	m = str(message.payload.decode("utf-8"))
	if m.split(':')[0] != USERNAME:
		print(m)


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


def client_loop(client, sense_emu):
	# Client Loop
	client.loop_start()
	try:
		while True:
			t = _get_temperature(sense_emu, False)
			h = _get_humidity(sense_emu)
			p = _get_pressure(sense_emu)

			# publishing raw data on the different topics
			client.publish(TOPIC_TEMP, t, QOS)
			client.publish(TOPIC_HUMI, h, QOS)
			client.publish(TOPIC_PRES, p, QOS)

			# led_display(sense_hat, t, h, p)
			# output rate in s
			time.sleep(1)

	# exit on ctrl+c
	except KeyboardInterrupt:
		print("[i] You stopped the weather station")
	#except Exception as e:
	#	print("[-] An error occurred:", e)

	client.disconnect()
	time.sleep(1)  # wait
	client.loop_stop()  # stop the loop
	pass


def main():
	# 1. init sense_hat
	sense_emu = SenseHat()  # instantiate a sense_hat object

	# 2. init MQTT
	client = init_mqtt()

	# 3. enter MQTT loop
	client_loop(client, sense_emu)
	exit(0)


if __name__ == "__main__":
	main()
	pass
