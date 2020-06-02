# -*-coding:UTF-8 -*
# !/usr/bin/python

import time
import paho.mqtt.client as mqtt

# Global Vars
TOPIC = "/iot/access-system/status"
QOS = 1
USERNAME = "pi"
PASSWORD = "raspberry"
BROKER_ADDRESS = "192.168.8.110"
PORT = 1883

PRECISION = 5
MAX_INACTIVITY_DELAY = 5

#int test_set[10]
test_set_0 = [100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
test_set_1 = [100, 100, 100, 40, -70, -94, -95, -105, -100, 100]
test_set_2 = [100, -100, 80, -100, 100, 100, 100, 100, 100, 100]
test_set_3 = [100, 100, -100, 100, 100, 100, 100, 100, 100, 100]

# /************************
# *  Contactless Sensor  *
# ************************/


def dummy_hall_sensor_read(i, test_set):
	return test_set[i]


def update_button_state(current_bs, previous_button_state, current_button_state):
	previous_button_state_2 = previous_button_state
	previous_button_state = current_button_state
	current_button_state = current_bs


def read_sens_task(with_magnet, access_system_state, previous_button_state_2, previous_button_state, timer_inactivity):
	current_hall = dummy_hall_sensor_read(1000, 8)
	print("ESP32 Hall sensor average = %d mT", current_hall)
	if current_hall <= (with_magnet + PRECISION):
		update_button_state(True)
		timer_inactivity = 0
		print("++ Button state: Pressed")
		if not access_system_state:
			access_system_state = True
			print("++ System state: Open")
			print("[i] A user opened the system.")
		elif previous_button_state_2 and not previous_button_state:
			access_system_state = False
			print("-- System state: Closed")
			print("[i] A user closed the system.")
		else:
			print("++ System state: Open")
	else:
		update_button_state(False)
		print("-- Button state: Not Pressed")
		if access_system_state:
			if timer_inactivity == MAX_INACTIVITY_DELAY:
				access_system_state = False
				timer_inactivity = 0
				print("-- System state: Closed")
				print("[!] System automatically closed due to inactivity")
			else:
				print("++ System state: Open")
				timer_inactivity += 1
		else:
			print("-- System state: Closed")


# Functions
def on_connect(client, userdata, flags, rc):
	print("[+] Connected.")
	client.subscribe(TOPIC)
	pass


def on_disconnect(client, userdata, rc):
	if rc != 0:
		print("[!] Unexpected disconnection.")
	else:
		print("[i] Goodbye.")
	pass


def on_message(client, userdata, message):
	m = str(message.payload.decode("utf-8"))
	print(m)


def on_subscribe(client, userdata, mid, granted_qos):
	print("[i] Joining the topic: {}".format(TOPIC))
	pass


def on_unsubscribe(client, userdata, mid):
	# print("Unsubscribed:", str(mid))
	pass


def init_mqtt():
	# 1. Create a Client Instance
	print("[i] Creating new instance")
	client = mqtt.Client()

	# 2. Attaching client to Callback Functions
	client.on_subscribe = on_subscribe
	client.on_connect = on_connect
	client.on_message = on_message
	client.on_disconnect = on_disconnect
	# client.on_log = on_log
	time.sleep(1)  # Sleep to prevent disorder

	# 3. Input username, password, and pub/sub topics in the terminal
	client.username_pw_set(USERNAME, PASSWORD)

	# 4. Connect to MaQiaTTo Broker in the cloud
	print("[i] Connecting to {} ...".format(BROKER_ADDRESS))
	client.connect(BROKER_ADDRESS, PORT)
	time.sleep(4)  # Sleep to prevent disorder
	return client


def client_loop(client):
	print("[i] Sensor Calibration...")
	without_magnet, with_magnet = 100, -100
	print("[+] Calibration sucessfull:")
	print("\t> Standard Value    = ;", without_magnet)
	print("\t> Value with Magnet =%d;", with_magnet)

	current_hall = without_magnet
	timer_inactivity = 0
	access_system_state = False
	current_button_state = False
	previous_button_state = False
	previous_button_state_2 = False

	# Client Loop
	client.loop_start()
	try:
		while True:
			print("++ Button: Pressed | -- System: Closed")
			# output rate in s
			time.sleep(1)

	# exit on ctrl+c
	except KeyboardInterrupt:
		print("[i] You stopped the weather station")

	client.disconnect()
	time.sleep(1)  # wait
	client.loop_stop()  # stop the loop
	pass


# /************************
# *         MAIN         *
# ************************/


def main():
	# 1. init MQTT
	client = init_mqtt()

	# 2. enter MQTT loop
	client_loop(client)
	exit(0)


if __name__ == "__main__":
	main()
	pass
