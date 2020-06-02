#-*-coding:UTF-8 -*
#!/usr/bin/env python

import time
import unittest
import paho.mqtt.client as mqtt


def simulate_traffic(broker_address="rasberrypi.local", chatroom="chat/general", n=5):
	""" Simulate traffic in a given chatroom, coming from a test_client
	:param broker_address: <string>
	:param chatroom: <string>
	:param n: <int> - number of message to send
	:return: None
	"""
	test_client = mqtt.Client("TEST_USER", clean_session=True)
	test_client.connect(broker_address)
	for i in range(n):
		test_client.publish(chatroom, "Here's a test message n°"+str(i))
		time.sleep(2)
	return None


class MyTestCase(unittest.TestCase):
	def test_something(self):
		self.assertEqual(True, False)

		# for test sake
		# logger.debug("[~] Broker name: {}".format(broker_address))
		# simulate_traffic(broker_address, topic, 5)


if __name__ == '__main__':
	unittest.main()
