#!/usr/bin/python3
from __future__ import print_function
import os, sys, string
import argparse
import json
import time
import threading
import subprocess
import shlex
import shutil
import traceback
from pprint import pprint, pformat
import zmq
import picamera
import exifread
import psutil
from datetime import datetime
utcnow = datetime.utcnow


THREADS_RUN = True
GLOB = None
BOOT_TIME = utcnow()
STATE = {}
PHOTO_ARR = [] # list of taken photo files.


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


def RunCmd(cmd):
	p = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE, stderr=subprocess.PIPE, bufsize=1, shell=False )
	(stdoutdata, stderrdata) =  p.communicate()
	return stdoutdata


# load key = value file with #comments
def config_file_read(fName):
	with open(fName) as f:
		lines = f.readlines()
		lines = map(str.strip, lines)
		lines = list(filter(lambda x: not x.startswith('#'), lines))
		while '' in lines:
			lines.remove('')
		# lines = filter(lambda x: x.startswith('port') or x.startswith('cam_'), lines)
		lines = map(lambda x: x.split('#')[0], lines)
		key_vals = map( lambda x: x.split('=', 1), lines )
		key_vals = map( lambda x: [ str.strip(x[0]), str.strip(x[1]) ], key_vals )
		return key_vals


def prog_opts():
	parser = argparse.ArgumentParser(description='arg parser')
	parser.add_argument('--config', type=str, action='store')
	parser.add_argument('--port', dest='port', action='store', type=int)
	parser.add_argument('--cam_dir', dest='cam_dir', action='store')
	parser.add_argument('--cam_flip_h', dest='cam_flip_h', action='store', type=int)
	parser.add_argument('--cam_flip_v', dest='cam_flip_v', action='store', type=int)
	parser.add_argument('--cam_ssdv_res', dest='cam_ssdv_res', action='store', type=int)
	parser.add_argument('--cam_video_dur', dest='cam_video_dur', action='store', type=int)
	parser.add_argument('--cam_snapshot_interval', dest='cam_snapshot_interval', action='store', type=int)

	args = parser.parse_args()
	ret = {
		'port': 6666,
		'cam_flip_h': 0,
		'cam_flip_v': 0,
		'cam_ssdv_res': 256,
		'cam_video_dur': 15, # seconds
	}

	if args.config:
		for k,v in config_file_read( args.config ):
			ret[k] = v
			try:		ret[k] = int(v)
			except:		pass

	if args.port:			ret['port'] = args.port
	if args.cam_dir:		ret['cam_dir'] = args.cam_dir
	if args.cam_flip_h:		ret['cam_flip_h'] = args.cam_flip_h
	if args.cam_flip_v:		ret['cam_flip_v'] = args.cam_flip_v
	if args.cam_ssdv_res:	ret['cam_ssdv_res'] = args.cam_ssdv_res
	if args.cam_video_dur:	ret['cam_video_dur'] = args.cam_video_dur
	if args.cam_snapshot_interval:	ret['cam_snapshot_interval'] = args.cam_snapshot_interval

	# pprint(ret)

	if 'cam_dir' not in ret:
		print(sys.argv[0] + "No cam_dir. Exit.")
		sys.exit(1)

	return ret


def FileExif(i_path):
	try:
		with open(i_path, 'rb') as f:
			tags = exifread.process_file(f)
			tags['JPEGThumbnail'] = None
			return tags
	except:
		print("Error reading EXIF: ", i_path)


def ConvertToSSDV(i_file, o_file, resolution, callsign, image_id):
	if not os.path.isfile(i_file):
		print("ConvertToSSDV: not a file ", i_file)

	alt = 0
	try:
		exif = FileExif(i_file)
		alt= str(exif['GPS GPSAltitude']) # 1149/10
		alt = eval(alt)
	except:
		pass

	rx = 16 * int(resolution/16)
	ry = 16 * int(resolution*2/3/16)
	ann = 'Alt ' + str(alt)

	resize_cmd = ""
	resize_cmd += "convert %s " % i_file
	resize_cmd += "-resize %dx%d^ -gravity center -extent %dx%d " % (rx,ry,rx,ry)
	# disable annotation because we use low-res snapshots that are already annotated
	# resize_cmd += "-gravity north -stroke '#333C' -strokewidth 2 -annotate +0+10 '%s' " % ann
	# resize_cmd += "-stroke  none   -fill white    -annotate +0+10 '%s' " % ann
	resize_cmd += o_file + '_toSSDV.jpg'
	print(resize_cmd)
	print( RunCmd(resize_cmd) )

	ssdv_cmd = '/boot/ssdv -e -c %s -i %d %s %s' % (callsign, image_id, o_file + '_toSSDV.jpg', o_file)
	print(ssdv_cmd)
	print( RunCmd(ssdv_cmd) )

	os.remove(o_file + '_toSSDV.jpg')


def DecimalDegreeConvert(ddgr):
	d = int(ddgr)
	m = int((ddgr - d) * 60)
	s = (ddgr - d - m/60) * 3600
	return (d,m,s)


def SetCameraExif(camera, state):
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


def StateLoop(port):
	REQUEST_TIMEOUT = 3000
	REQUEST_RETRIES = 1e30
	SERVER_ENDPOINT = "tcp://localhost:" + str(port)

	global STATE


	print("Connecting to " + SERVER_ENDPOINT)
	context = zmq.Context(1)
	client = context.socket(zmq.REQ)
	client.connect(SERVER_ENDPOINT)
	poll = zmq.Poller()
	poll.register(client, zmq.POLLIN)
	query_msgs = ['nmea', 'dynamics']


	retries_left = REQUEST_RETRIES
	while THREADS_RUN and retries_left:
		time.sleep(1)
		for qm in query_msgs:
			# print("\n\nSending (%s)" % qm)
			client.send_string(qm)

			expect_reply = True
			while THREADS_RUN and expect_reply:
				socks = dict(poll.poll(REQUEST_TIMEOUT))
				if socks.get(client) == zmq.POLLIN:
					reply = client.recv()
					if reply:
						try:
							json_str = reply.decode("utf-8").replace("'", '"')
							reply_data = json.loads( json_str )
							STATE[qm] = reply_data
						except:
							print("Can't parse JSON for ", qm)
							print(reply)
							print(traceback.format_exc())
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



def SSDV_DeliverLoop(callsign, out_ssdv_path, res):
	'''
	picks last image from PHOTO_ARR, converts to SSDV and copies to output
	'''
	global PHOTO_ARR

	# SSDV Image ID. Try getting last used number from file
	image_id = 0
	try:
		with open('./camera.ssdv_id') as fh:
			image_id = 1 + int(fh.read())
			image_id = image_id % 256
	except:
		pass

	while(THREADS_RUN):
		time.sleep(5)

		if not PHOTO_ARR:
			continue

		if os.path.isfile(out_ssdv_path):
			continue

		ssdv_in = PHOTO_ARR.pop()
		print("Exporting new SSDV image:", ssdv_in)
		ConvertToSSDV( ssdv_in, out_ssdv_path, res, callsign, image_id)
		image_id += 1
		image_id = image_id % 256
		try:
			with open('./camera.ssdv_id', 'w') as fh:
				fh.write(str(image_id))
		except:
			print(traceback.format_exc())
			print('Failed writing last SSDV ID to file.')
			pass


def next_path(i_base, ext = ''): # get next subdir/subfile
	if ext and ext[0] != '.':
		ext = '.' + ext
	i = 1
	ret = os.path.join(i_base, str(i).zfill(6)) + ext
	while os.path.exists(ret):
		i += 1
		ret = os.path.join(i_base, str(i).zfill(6)) + ext
	return ret


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
	CAMERA.exposure_mode = 'auto'
	CAMERA.meter_mode = 'matrix'
	CAMERA.hflip = opts['cam_flip_h']
	CAMERA.vflip = opts['cam_flip_v']
	CAMERA.still_stats = True
	# CAMERA.start_preview()
	# camera.bitrate = CFG['video_bitrate']

	global STATE
	global PHOTO_ARR

	snapshot_time = datetime.fromtimestamp(0)

	alt = 0
	dAlt = 0
	dAltAvg = 0
	global THREADS_RUN
	while(THREADS_RUN):

		disk_use_percent = int( psutil.disk_usage('/').percent )
		if disk_use_percent > 95:
			CAMERA.stop_preview()
			print("Free disk space left: ", disk_use_percent, '% . Waiting.')
			time.sleep(60)
			continue

		CAMERA.start_preview()

		# full res photo
		print("Photo HI")
		CAMERA.resolution = (2592, 1944)
		SetCameraExif(CAMERA, STATE)
		CAMERA.annotate_text = ''
		CAMERA.capture( next_path(photo_hi_dir, 'jpeg'))

		# video clip
		print("Video")
		video_duration_secs = int( opts['cam_video_dur'] )
		snapshot_interval_secs = int( opts['cam_snapshot_interval'] )
		CAMERA.resolution = (1280, 720)
		CAMERA.start_recording( next_path(video_dir, 'h264'))

		# wait and update annotation and EXIF
		video_start = utcnow()
		while seconds_since(video_start) < video_duration_secs:
			if not THREADS_RUN:
				break

			disk_use_percent = int( psutil.disk_usage('/').percent )
			if disk_use_percent > 95:
				CAMERA.stop_preview()
				print("Free disk low - abort camera.")
				break

			if 'dynamics' in STATE and 'alt' in STATE['dynamics']:
				alt = 	STATE['dynamics']['alt']['val']
				dAlt = 	STATE['dynamics']['alt']['dVdT']
				dAltAvg = 	STATE['dynamics']['alt']['dVdT_avg']
				# print(alt, dAlt, dAltAvg)

			SetCameraExif(CAMERA, STATE)
			CAMERA.annotate_text = 'Alt: %d/%.01f m' % (int(alt), dAlt)

			if seconds_since(snapshot_time) > snapshot_interval_secs:
				print("Photo LO")
				PHOTO_ARR.append( next_path(photo_lo_dir, 'jpg') )
				CAMERA.capture( PHOTO_ARR[-1] , use_video_port = True )
				snapshot_time = utcnow()

			time.sleep(1)
		CAMERA.stop_recording()

	print('CAMERA.close()')
	CAMERA.close()


def main():
	global THREADS_RUN
	global GLOB
	GLOB = prog_opts()
	pprint(GLOB)

	GLOB['cam_dir'] = GLOB['cam_dir'].replace('.', curdir())
	if not os.path.isdir(GLOB['cam_dir']):
		os.makedirs(GLOB['cam_dir'])

	session_dir = next_path(GLOB['cam_dir'])
	cam_process = None
	state_process = None
	try:
		cam_process = threading.Thread( target = lambda: CameraLoop(session_dir, GLOB) )
		cam_process.start()
		state_process = threading.Thread( target= lambda: StateLoop(GLOB['port']) )
		state_process.start()
		ssdv_process = threading.Thread( target= lambda: SSDV_DeliverLoop( GLOB['callsign'], GLOB['ssdv'], GLOB['cam_ssdv_res']) )
		ssdv_process.start()
		while(1):
			time.sleep(1)
	except KeyboardInterrupt:
		print("CTRL+C")
		THREADS_RUN = False
		if cam_process:			cam_process.join()
		if state_process:		state_process.join()
		if ssdv_process:		ssdv_process.join()
	except:
		print(traceback.format_exc())
		THREADS_RUN = False
		if cam_process:			cam_process.join()
		if state_process:		state_process.join()
		if ssdv_process:		ssdv_process.join()


if __name__ == "__main__":
	try:
		import setproctitle
		setproctitle.setproctitle('camera.py')
	except:
		pass

	while 1:
		try:
			main()
		except KeyboardInterrupt:
			break
		except:
			print( traceback.format_exc() )
			time.sleep(1)
	# ConvertToSSDV(sys.argv[-2], sys.argv[-1], 512, 'fro', 0)