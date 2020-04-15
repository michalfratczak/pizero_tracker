#!/usr/bin/python
from __future__ import print_function
import os, sys, string
import argparse
import json
import time
from datetime import datetime
utcnow = datetime.utcnow
import threading
import traceback
from pprint import pprint, pformat
import zmq
import picamera


GLOB = None
BOOT_TIME = utcnow()

def curdir():
	if 'PWD' not in os.environ or not os.environ['PWD']:
		_d = os.path.dirname(__file__)
	else:
		_d = os.environ['PWD']

	if _d == '.':
		_d = os.getcwd()

	if not _d:
		return None

	return _d


def prog_opts():
	parser = argparse.ArgumentParser(description='arg parser')
	parser.add_argument('--port', dest='port', action='store', type=int)
	parser.add_argument('--cam_dir', dest='cam_dir', action='store')
	parser.add_argument('--cam_flip_h', dest='cam_flip_h', action='store', type=int)
	parser.add_argument('--cam_flip_v', dest='cam_flip_v', action='store', type=int)

	args = parser.parse_args()
	ret = {
		'cam_flip_h': 0,
		'cam_flip_v': 0,
		'port': 6666
	}

	if args.port:				ret['port'] = 				args.port
	if args.cam_dir:			ret['cam_dir'] = 				args.cam_dir
	if args.cam_flip_h:			ret['cam_flip_h'] = 			args.cam_flip_h
	if args.cam_flip_v:			ret['cam_flip_v'] = 			args.cam_flip_v

	return ret



def DecimalDegreeConvert(ddgr):
	d = int(ddgr)
	m = int((ddgr - d) * 60)
	s = (ddgr - d - m/60) * 3600
	return (d,m,s)



def EXIF(camera, state):
	lat = 0
	lon = 0
	alt = 0
	if 'nmea' in STATE:
		lat = STATE['nmea']['lat']
		lon = STATE['nmea']['lon']
		alt = STATE['nmea']['alt']

	camera.exif_tags['IFD0.Copyright'] = 'Copyright (c) 2020 Michal Fratczak michal@cgarea.com'
	camera.exif_tags['GPS.GPSLatitude'] = '%d/1,%d/1,%d/100' % DecimalDegreeConvert(lat)
	camera.exif_tags['GPS.GPSLongitude'] = '%d/1,%d/1,%d/100' % DecimalDegreeConvert(lon)

	camera.exif_tags['GPS.GPSAltitudeRef'] = '0'
	camera.exif_tags['GPS.GPSAltitude'] = '%d/100' % int( 100 * alt )

	camera.exif_tags['GPS.GPSSpeedRef'] = 'K'
	# camera.exif_tags['GPS.GPSSpeed'] = '%d/1000' % int(3600 * state['Speed'])
	# camera.exif_tags['EXIF.UserComment'] = "GrndElev:" + str(state['GrndElev'])


STATE = {}


STATE_RUN = True
def StateLoop(port):
	REQUEST_TIMEOUT = 3000
	REQUEST_RETRIES = 1e12
	SERVER_ENDPOINT = "tcp://localhost:" + str(port)

	global STATE

	context = zmq.Context(1)

	print("Connecting to " + SERVER_ENDPOINT)
	client = context.socket(zmq.REQ)
	client.connect(SERVER_ENDPOINT)

	poll = zmq.Poller()
	poll.register(client, zmq.POLLIN)

	query_msgs = ['nmea', 'dynamics']

	retries_left = REQUEST_RETRIES
	while STATE_RUN and retries_left:
		time.sleep(1)
		for qm in query_msgs:
			# print("\n\nSending (%s)" % qm)
			client.send(qm)

			expect_reply = True
			while STATE_RUN and expect_reply:
				socks = dict(poll.poll(REQUEST_TIMEOUT))
				if socks.get(client) == zmq.POLLIN:
					reply = client.recv()
					if reply:
						try:
							reply = reply.replace("'", '"')
							STATE[qm] = json.loads(reply)
						except:
							print("Can't parse JSON for ", qm)
							# print(traceback.format_exc())
						expect_reply = False
					else:
						break
				else:
					# print("No response from server, retrying")
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


def next_path(i_base, ext = ''): # get next subdir/subfile
	if ext and ext[0] != '.':
		ext = '.' + ext
	i = 1
	ret = os.path.join(i_base, string.zfill(i, 6)) + ext
	while os.path.exists(ret):
		i += 1
		ret = os.path.join(i_base, string.zfill(i, 6)) + ext
	return ret


CAM_RUN = True
def CameraLoop(session_dir, opts):

	def seconds_since(since):
		return (utcnow()-since).total_seconds()

	# media dir
	#
	photo_lo_dir = os.path.join(session_dir, 'photo_lo')
	photo_hi_dir = os.path.join(session_dir, 'photo_hi')
	video_dir = os.path.join(session_dir, 'video')
	os.makedirs(photo_lo_dir)
	os.makedirs(photo_hi_dir)
	os.makedirs(video_dir)
	print('photo_lo_dir', photo_lo_dir)
	print('photo_hi_dir', photo_hi_dir)
	print('video_dir', video_dir)

	# setup camera
	#
	CAMERA = picamera.PiCamera()
	CAMERA.start_preview()
	CAMERA.exposure_mode = 'auto'
	CAMERA.meter_mode = 'matrix'
	CAMERA.hflip = opts['cam_flip_h']
	CAMERA.vflip = opts['cam_flip_v']
	CAMERA.still_stats = True
	CAMERA.start_preview()
	# camera.bitrate = CFG['video_bitrate']

	global STATE

	alt = 0
	dAlt = 0
	b_stdby_mode = True # in this mode, just one small picture is recorded/sended
	stdby_file = os.path.join(session_dir, 'stdby.jpeg')
	global CAM_RUN
	while(CAM_RUN):

		# in this mode, just one small picture is recorded/sended
		if b_stdby_mode:
			print('b_stdby_mode')
			CAMERA.resolution = (8*16, 4*16)
			EXIF(CAMERA, STATE)
			CAMERA.annotate_text = str( int(seconds_since(BOOT_TIME) / 60) )
			CAMERA.capture( stdby_file )
			time.sleep(5)
			continue
		else:
			os.system( 'rm -f %s || echo "Cant remove stdby.jpeg"' % stdby_file )

		# full res photo
		print("Photo HI")
		CAMERA.resolution = (2592, 1944)
		EXIF(CAMERA, STATE)
		CAMERA.annotate_text = ''
		CAMERA.capture( next_path(photo_hi_dir, 'jpeg'))

		# video clip
		print("Video")
		video_duration_secs = 10
		snapshot_interval_secs = 3
		CAMERA.resolution = (1280, 720)
		CAMERA.start_recording( next_path(video_dir, 'h264'))

		# wait and update annotation and EXIF
		video_start = utcnow()
		snapshot_time = utcnow()
		while seconds_since(video_start) < video_duration_secs:
			if not CAM_RUN:
				break

			if 'dynamics' in STATE and 'alt' in STATE['dynamics']:
				alt = 	STATE['dynamics']['alt']['val']
				dAlt = 	STATE['dynamics']['alt']['dVdT']
			print(alt, dAlt)

			EXIF(CAMERA, STATE)
			CAMERA.annotate_text = 'Alt: %d/%.01f m' % (int(alt), dAlt)

			if seconds_since(snapshot_time) > snapshot_interval_secs:
				print("Photo LO")
				CAMERA.capture( next_path(photo_lo_dir, 'jpg'), use_video_port = True )
				snapshot_time = utcnow()

			time.sleep(1)

		CAMERA.stop_recording()

	print('CAMERA.close()')
	CAMERA.close()


def main():
	global GLOB
	GLOB = prog_opts()
	pprint(GLOB)

	GLOB['cam_dir'] = GLOB['cam_dir'].replace('.', curdir())
	if not os.path.isdir(GLOB['cam_dir']):
		raise RuntimeError( "Not a dir " + GLOB['cam_dir'] )

	session_dir = next_path(GLOB['cam_dir'])
	cam_process = None
	state_process = None
	try:
		cam_process = threading.Thread( target = lambda: CameraLoop(session_dir, GLOB) )
		cam_process.start()
		state_process = threading.Thread( target= lambda: StateLoop(GLOB['port']) )
		state_process.start()
		while(1):
			time.sleep(1)
	except KeyboardInterrupt:
		global CAM_RUN
		CAM_RUN = False
		global STATE_RUN
		STATE_RUN = False
		if cam_process:			cam_process.join()
		if state_process:		state_process.join()
	except:
		print(traceback.format_exc())
		global CAM_RUN
		CAM_RUN = False
		global STATE_RUN
		STATE_RUN = False
		if cam_process:			cam_process.join()
		if state_process:		state_process.join()



if __name__ == "__main__":
	main()
