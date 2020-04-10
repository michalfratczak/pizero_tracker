#!/usr/bin/env python

import time
import zmq

def main():

	context = zmq.Context(1)
	socket = context.socket(zmq.REQ)
	socket.connect( 'tcp://192.168.1.22:6666' )

	while True:
		socket.send('nmea')
		msg = socket.recv()
		print msg
		time.sleep(1)

		socket.send('nmeaX')
		msg = socket.recv()
		print msg
		time.sleep(1)

if __name__ == '__main__':
	main()
	while(1):
		pass
