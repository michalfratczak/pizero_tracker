#!/usr/bin/env python
from __future__ import print_function
import time
import json
import traceback
from pprint import pprint
import zmq


def process_reply(msg):
	if "dynamics" in msg or "nmea" in msg:
		msg = msg.replace("'", '"')
		try:
			data = json.loads(msg)
			return data
		except:
			print(traceback.format_exc())
		# time.sleep(1)


def main2():
	REQUEST_TIMEOUT = 3000
	REQUEST_RETRIES = 1e12
	SERVER_ENDPOINT = "tcp://192.168.1.22:6666"

	context = zmq.Context(1)

	print("Connecting to server")
	client = context.socket(zmq.REQ)
	client.connect(SERVER_ENDPOINT)

	poll = zmq.Poller()
	poll.register(client, zmq.POLLIN)

	query_msgs = ['nmea', 'dynamics', 'invalid']

	retries_left = REQUEST_RETRIES
	while retries_left:
		time.sleep(1)
		for qm in query_msgs:
			print("\n\nSending (%s)" % qm)
			client.send(qm)

			expect_reply = True
			while expect_reply:
				socks = dict(poll.poll(REQUEST_TIMEOUT))
				if socks.get(client) == zmq.POLLIN:
					reply = client.recv()
					if reply:
						print("OK")
						pprint( process_reply(reply) )
						expect_reply = False
					else:
						break
				else:
					print("No response from server, retrying")
					# Socket is confused. Close and remove it.
					client.setsockopt(zmq.LINGER, 0)
					client.close()
					poll.unregister(client)
					retries_left -= 1
					if retries_left == 0:
						print("Server seems to be offline, abandoning")
						break
					print("Reconnecting and resending (%s)" % qm)
					# Create new connection
					client = context.socket(zmq.REQ)
					client.connect(SERVER_ENDPOINT)
					poll.register(client, zmq.POLLIN)
					client.send(qm)

	context.term()


if __name__ == '__main__':
	try:
		main2()
	except KeyboardInterrupt:
		pass
	except:
		print(traceback.format_exc())

	# while(1):		pass
