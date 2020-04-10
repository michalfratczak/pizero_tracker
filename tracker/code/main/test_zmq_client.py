#!/usr/bin/env python

import time
import zmq

def main():

	context = zmq.Context(1)
	socket = context.socket(zmq.REQ)
	socket.connect( 'tcp://192.168.1.22:6666' )

	query_msgs = ['nmea', 'temp', 'invalid']

	while True:
		for qm in query_msgs:
			socket.send(qm)
			msg = socket.recv()
			print msg
			time.sleep(1)


if __name__ == '__main__':
	main()
	while(1):
		pass
