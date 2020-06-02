# -*-coding:UTF-8 -*
# !/usr/bin/env python

""" ITS08050 - Project 4: pyMQTTChat

	=> Terminal chat client with Paho's Python MQTT implementation.

	Functionality:
	- You enter a chat room and have a username.
	- Only those users can talk to each other who are in the same chat room.
	- It's possible (preferred) to use the same code for both sides of the chat
	- actually, more than 2 participants should be possible

	Method required
	- ask username (client creation)
	Solution to make the ID unique
	créer un compteur 'i' du nombre d'utilisateur
	chaque nouvelle utilisateur sera nommé "Guest "+'i'
	or just use a list with name of all client


	- ask which MQTT Broker to connect to (default one would be raspberrypi) (IP or host_name). (connect)
	- ask which chatroom (topic) you want to be in. (
	- publish

	On the RPi:
	mosquitto_sub -u pi P raspberry -t chat/general

	https://haptik.ai/tech/real-time-messaging-using-mqtt/
	https://www.hackster.io/acellon/terminal-chat-client-8275a7

"""

# Libs
import paho.mqtt.client as mqtt
import secrets
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
TOPIC = "chat/general"
username = "GUEST_" + str(secrets.randbelow(10000)).zfill(4)


# Functions
def on_connect(client, userdata, flags, rc):
	logger.info("[+] Connected.")
	client.subscribe("/chat/general")
	pass

def on_disconnect(client, userdata, rc):
	if rc != 0:
		logger.info("[!] Unexpected disconnection.")
	else:
		logger.info("[i] Goodbye.")
	pass

def on_message(client, userdata, message):
	m = str(message.payload.decode("utf-8"))
	if m.split(':')[0] != username:
		print(m)

def on_subscribe(client, userdata, mid, granted_qos):
	logger.info("[i] Joining the chatroom: {}".format(TOPIC))
	pass

def on_unsubscribe(client, userdata, mid):
	# print("Unsubscribed:", str(mid))
	pass

def on_log(client, userdata, level, buf):
	logger.debug("[MQTT_log]: {}".format(buf))
	pass


def show_help():
	logger.info("[i] Option are: \n" +
		"/? /h                       - show the current help\n" +
	    "/j <topic> /join <topic>    - join the specified topic\n" +
	    "/which                      - show the current topic\n" +
	    "/q                          - quit")


def main():
	global TOPIC
	global username

	# 0. Init
	#broker_addr = input("broker hostname or IP@: ")
	#if broker_addr.lower() == "default":
	#	broker_addr = "raspberrypi.local"
	#BROKER_ADDRESS = broker_addr
	BROKER_ADDRESS = "192.168.8.110"
	PORT = 1883  # port 8883 for TLS/SSL

	# 1. Create a Client Instance
	logger.info("[i] Creating new instance")
	client = mqtt.Client()

	# 2. Attaching client to Callback Functions
	client.on_subscribe = on_subscribe
	client.on_connect = on_connect
	client.on_message = on_message
	client.on_unsubscribe = on_unsubscribe
	client.on_disconnect = on_disconnect
	# client.on_log = on_log
	time.sleep(1)  # Sleep to prevent disorder

	# 3. Input username, password, and pub/sub topics in the terminal
	username = input('Username: ').strip("#;: ")  # strip chars that can cause bug
	password = input('Password: ').strip()
	client.username_pw_set(username, password)

	# 4. Connect to a Broker or Server
	logger.info("[i] Connecting to {} ...".format(BROKER_ADDRESS))
	client.connect(BROKER_ADDRESS, PORT)
	time.sleep(4)  # Sleep to prevent disorder

	# Client Loop
	client.loop_start()

	while True:
		chat = input()
		if chat == '/q':
			break
		elif chat == '/?' or chat == '/h':
			show_help()
		elif chat[0:3] == '/j ' or chat[0:6] == '/join ':
			new_subtop = "/chat/" + chat[3:].strip('#;')
			client.unsubscribe(TOPIC)
			TOPIC = new_subtop
			client.subscribe(TOPIC)
		elif chat[:6] == "/which":
			logger.info("[i] Current Chatroom: {}".format(TOPIC))
		else:
			client.publish(TOPIC, username + ": " + chat)
		time.sleep(1)


	client.disconnect()
	time.sleep(1)  # wait
	client.loop_stop()  # stop the loop
	pass


if __name__ == "__main__":
	main()
	pass
