#!/usr/bin/python
from __future__ import print_function
import os, sys, string
import argparse
import json
import time
import datetime
import threading
import traceback
from pprint import pprint, pformat
import zmq
import picamera


GLOB = None


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
	parser.add_argument('--cam_dir', dest='cam_dir', action='store')
	parser.add_argument('--cam_interval', dest='cam_interval', action='store', type=int)
	parser.add_argument('--cam_flip_h', dest='cam_flip_h', action='store', type=int)
	parser.add_argument('--cam_flip_v', dest='cam_flip_v', action='store', type=int)

	args = parser.parse_args()
	ret = {
		'cam_flip_h': 0,
		'cam_flip_v': 0
	}

	if args.cam_dir:			ret['cam_dir'] = 				args.cam_dir
	if args.cam_interval:		ret['cam_interval'] = 		    args.cam_interval
	if args.cam_flip_h:			ret['cam_flip_h'] = 			args.cam_flip_h
	if args.cam_flip_v:			ret['cam_flip_v'] = 			args.cam_flip_v

	return ret



def DecimalDegreeConvert(ddgr):
	d = int(ddgr)
	m = int((ddgr - d) * 60)
	s = (ddgr - d - m/60) * 3600
	return (d,m,s)



def EXIF(camera, state):
	camera.exif_tags['IFD0.Copyright'] = 'Copyright (c) 2020 Michal Fratczak michal@cgarea.com'
	camera.exif_tags['GPS.GPSLatitude'] = '%d/1,%d/1,%d/100' % DecimalDegreeConvert(state['lat'])
	camera.exif_tags['GPS.GPSLongitude'] = '%d/1,%d/1,%d/100' % DecimalDegreeConvert(state['lon'])

	camera.exif_tags['GPS.GPSAltitudeRef'] = '0'
	camera.exif_tags['GPS.GPSAltitude'] = '%d/100' % int( 100 * state['alt'] )

	camera.exif_tags['GPS.GPSSpeedRef'] = 'K'
	# camera.exif_tags['GPS.GPSSpeed'] = '%d/1000' % int(3600 * state['Speed'])
	# camera.exif_tags['EXIF.UserComment'] = "GrndElev:" + str(state['GrndElev'])


STATE = {
		'lat': 52,
		'lon': 21.1,
		'alt': 150
	}


STATE_RUN = True
def StateLoop():
	global STATE
	while(STATE_RUN):
		STATE['alt'] += 5
		print('.')
		time.sleep(1)


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

	global CAM_RUN
	while(CAM_RUN):

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
		video_start = datetime.datetime.utcnow()
		snapshot_time = datetime.datetime.utcnow()
		while (datetime.datetime.utcnow() - video_start).total_seconds() < video_duration_secs:
			EXIF(CAMERA, STATE)
			# STATE['alt'] += 5
			CAMERA.annotate_text = 'Alt: %d m' % STATE['alt']
			print(STATE['alt'])
			# print( STATE['alt'] )
			if (datetime.datetime.utcnow() - snapshot_time).total_seconds() > snapshot_interval_secs:
				print("Photo LO")
				CAMERA.capture( next_path(photo_lo_dir, 'jpg'), use_video_port = True )
				snapshot_time = datetime.datetime.utcnow()
			time.sleep(1)
		CAMERA.stop_recording()

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
		state_process = threading.Thread( target=StateLoop )
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
