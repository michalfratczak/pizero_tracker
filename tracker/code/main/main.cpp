#include <unistd.h> // write()
#include <signal.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>
#include <future>
#include <atomic>
#include <functional>
#include <math.h>

#include "pigpio.h"
#include <zmq.hpp>

#include "mtx2/mtx2.h"
#include "nmea/nmea.h"
#include "ublox/ublox_cmds.h"
#include "ublox/ublox.h"
#include "ds18b20/ds18b20.h"
#include "ssdv_t.h"
#include "cli.h"
#include "GLOB.h"
#include "gps_distance_t.h"
#include "async_log_t.h"
#include "pulse_t.h"

const char* C_RED = 		"\033[1;31m";
const char* C_GREEN = 		"\033[1;32m";
const char* C_MAGENTA = 	"\033[1;35m";
const char* C_OFF =   		"\033[0m";

bool G_RUN = true; // keep threads running

// value written to /dev/watchdog
// '1' enables watchdog
// 'V' disables watchdog
std::atomic_char G_WATCHDOG_V{'1'};

char _hex(char Character)
{
	char _hexTable[] = "0123456789ABCDEF";
	return _hexTable[int(Character)];
}

// UKHAS Sentence CRC
std::string CRC(std::string i_str)
{
	using std::string;

	unsigned int CRC = 0xffff;
	// unsigned int xPolynomial = 0x1021;

	for (size_t i = 0; i < i_str.length(); i++)
	{
		CRC ^= (((unsigned int)i_str[i]) << 8);
		for (int j=0; j<8; j++)
		{
			if (CRC & 0x8000)
			    CRC = (CRC << 1) ^ 0x1021;
			else
			    CRC <<= 1;
		}
	}

	string result;
	result += _hex((CRC >> 12) & 15);
	result += _hex((CRC >> 8) & 15);
	result += _hex((CRC >> 4) & 15);
	result += _hex(CRC & 15);

	return result;
}


zmq::message_t make_zmq_reply(const std::string& i_msg_str)
{
	auto& G = GLOB::get();
	if(i_msg_str == "nmea_current")	{
		std::string reply_str = G.nmea_current().json();
		reply_str.pop_back();
		reply_str += ",'fixAge':" + std::to_string(G.gps_fix_age()) + "}";
		zmq::message_t reply( reply_str.size() );
		memcpy( (void*) reply.data(), reply_str.c_str(), reply_str.size() );
		return reply;
	}
	else if(i_msg_str == "nmea_last_valid") {
		std::string reply_str = G.nmea_last_valid().json();
		reply_str.pop_back();
		reply_str += ",'fixAge':" + std::to_string(G.gps_fix_age()) + "}";
		zmq::message_t reply( reply_str.size() );
		memcpy( (void*) reply.data(), reply_str.c_str(), reply_str.size() );
		return reply;
	}
	else if(i_msg_str == "dynamics")	{
		std::string reply_str("{");
		for(auto& dyn_name : G.dynamics_keys())
			reply_str += "'" + dyn_name + "':" + G.dynamics_get(dyn_name).json() + ",";
		reply_str.pop_back(); // last comma
		reply_str += "}";
		zmq::message_t reply( reply_str.size() );
		memcpy( (void*) reply.data(), reply_str.c_str(), reply_str.size() );
		return reply;
	}
	else if(i_msg_str == "flight_state")	{
		std::string reply_str("{'flight_state':");
		switch( G.flight_state_get() )
		{
			case flight_state_t::kUnknown:		reply_str += "'kUnknown'";		break;
			case flight_state_t::kStandBy:		reply_str += "'kStandBy'";		break;
			case flight_state_t::kAscend:		reply_str += "'kAscend'";		break;
			case flight_state_t::kDescend:		reply_str += "'kDescend'";		break;
			case flight_state_t::kFreefall:		reply_str += "'kFreefall'";		break;
			case flight_state_t::kLanded:		reply_str += "'kLanded'";		break;
		}
		reply_str += "}";
		zmq::message_t reply( reply_str.size() );
		memcpy( (void*) reply.data(), reply_str.c_str(), reply_str.size() );
		return reply;
	}
	else {
		zmq::message_t reply( 7 );
		memcpy( (void*) reply.data(), "UNKNOWN", 7 );
		return reply;
	}
}


void watchdog_reset()
{
	char v = G_WATCHDOG_V.load();
	// std::cout<<"watchdog_reset "<<v<<std::endl;
	// return;

	FILE* f = fopen("/dev/watchdog", "w");
	if(f) {
		fwrite(&v, sizeof(char), 1, f);
		fclose(f);
	}
}

void watchdog_disable()
{
	G_WATCHDOG_V.store('V'); // disable
	char v = G_WATCHDOG_V.load();
	std::cout<<"watchdog_disable "<<v<<std::endl;
	// return;

	FILE* f = fopen("/dev/watchdog", "w");
	if(f) {
		fwrite(&v, sizeof(char), 1, f);
		fclose(f);
	}
}


void CTRL_C(int sig)
{
	watchdog_disable();
	std::cout<<"CTRL+C"<<std::endl;
	G_RUN = false;
}


int main1(int argc, char** argv)
{
    using namespace std;

	// command line interface
	CLI(argc, argv);

	// globals
	// G.cli is command line interface
	auto& G = GLOB::get();
	cout<<G.str()<<endl;

	if( !G.cli.callsign.size() ) {
		cerr<<C_RED<<"ERROR:\n\tNo Callsign."<<C_OFF<<endl;
		return 1;
	}

	if( !G.cli.freqMHz ) {
		cerr<<C_RED<<"ERROR:\n\tNo frequency."<<C_OFF<<endl;
		return 1;
	}

	if(G.cli.baud == baud_t::kInvalid) {
		cerr<<C_RED<<"ERROR:\n\tNo baud."<<C_OFF<<endl;
		return 1;
	}

	if( !G.cli.hw_pin_radio_on ) {
		cerr<<C_RED<<"ERROR:\n\tNo hw_pin_radio_on."<<C_OFF<<endl;
		return 1;
	}

	if( !G.cli.hw_radio_serial.size() ) {
		cerr<<C_RED<<"ERROR:\n\tNo hw_radio_serial."<<C_OFF<<endl;
		return 1;
	}

	if( !G.cli.hw_ublox_device.size() ) {
		cerr<<C_RED<<"ERROR:\n\tNo hw_ublox_device."<<C_OFF<<endl;
		return 1;
	}

	async_log_t LOG;
	LOG.logs_dir(G.cli.logsdir);
	LOG.log("main.log", "___START___");

	pulse_t PULSE;

	LOG.log("main.log", "sudo modprobe w1-gpio");
	system("sudo modprobe w1-gpio");

	if (gpioInitialise() < 0)
	{
		cerr<<C_RED<<"pigpio initialisation failed"<<C_OFF<<endl;
		return 1;
	}

	// register this after gpioInitialise()
	signal(SIGINT, CTRL_C);
	signal(SIGTERM, CTRL_C);


    // RADIO
    //
	LOG.log("main.log", "radio init");
	gpioSetPullUpDown( 	G.cli.hw_pin_radio_on, PI_PUD_DOWN );
	gpioSetMode( 		G.cli.hw_pin_radio_on, PI_OUTPUT );
	gpioWrite ( 		G.cli.hw_pin_radio_on, 1 );
	mtx2_set_frequency( G.cli.hw_pin_radio_on, G.cli.freqMHz );
	int radio_fd = 0;
	while(radio_fd<1) {
		cout<<"Opening Radio UART "<<G.cli.hw_radio_serial<<endl;
		radio_fd = mtx2_open( G.cli.hw_radio_serial, G.cli.baud );
		sleep(3);
	}


    // uBLOX I2C start and config
    //
	LOG.log("main.log", "uBLOX init");
    int uBlox_i2c_fd = 0;
	while(uBlox_i2c_fd<1) {
		cout<<"Opening uBlox I2C "<<G.cli.hw_ublox_device<<endl;
		uBlox_i2c_fd = uBLOX_i2c_open( G.cli.hw_ublox_device, 0x42 );
		sleep(3);
	}
	write(uBlox_i2c_fd, UBX_CMD_EnableOutput_ACK_ACK, sizeof(UBX_CMD_EnableOutput_ACK_ACK));
	write(uBlox_i2c_fd, UBX_CMD_EnableOutput_ACK_NAK, sizeof(UBX_CMD_EnableOutput_ACK_NAK));
	sleep(3);
	uBLOX_write_msg_ack( uBlox_i2c_fd, UBX_CMD_GSV_OFF, sizeof(UBX_CMD_GSV_OFF) );
	uBLOX_write_msg_ack( uBlox_i2c_fd, UBX_CMD_GLL_OFF, sizeof(UBX_CMD_GLL_OFF) );
	uBLOX_write_msg_ack( uBlox_i2c_fd, UBX_CMD_GSA_OFF, sizeof(UBX_CMD_GSA_OFF) );
	uBLOX_write_msg_ack( uBlox_i2c_fd, UBX_CMD_VTG_OFF, sizeof(UBX_CMD_VTG_OFF) );
    while (!uBLOX_write_msg_ack(uBlox_i2c_fd, UBX_CMD_NAV5_Airbororne1G, sizeof(UBX_CMD_NAV5_Airbororne1G)))
        cout<<C_RED<<"Retry Setting uBLOX Airborne1G Model"<<C_OFF<<endl;
	sleep(1);


	// uBLOX loop
	//
	auto ublox_loop = [uBlox_i2c_fd, &LOG, &PULSE]() {
		while(G_RUN) {
			PULSE.ping("uBLOX");

			const vector<char> ublox_data = uBLOX_read_msg(uBlox_i2c_fd); // typical blocking time: 0/1/1.2 seconds
			const string nmea_str = NMEA_get_last_msg(ublox_data.data(), ublox_data.size());
			// cout<<nmea_str<<endl;
			if( !NMEA_msg_checksum_ok(nmea_str) ) {
				cerr<<C_RED<<"NMEA Checksum Fail: "<<nmea_str<<C_OFF<<endl;
				continue;
			}
			LOG.log("nmea.log", nmea_str);

			nmea_t current_nmea;
			if( NMEA_parse(nmea_str.c_str(), current_nmea) /*and current_nmea.valid()*/ ) {
				// RMC has no altitude, copy from last known value
				if( current_nmea.nmea_msg_type_ == nmea_t::nmea_msg_type_t::kRMC )
					current_nmea.alt = GLOB::get().nmea_current().alt;

				GLOB::get().nmea_set(current_nmea);

				if(current_nmea.valid())
					GLOB::get().dynamics_add("alt", std::chrono::steady_clock::now(), current_nmea.alt);
				// cout<<C_MAGENTA<<"alt "<<GLOB::get().dynamics_get("alt").str()<<C_OFF<<endl;
			}
			// else - parse error or no lock (even no time)
		}
	};

	// fake GPS loop - usefull for testing
	//
	auto fake_gps_loop = [uBlox_i2c_fd]() {
		cout<<"Using FAKE GPS Coordinates !!!"<<endl;
		while(G_RUN) {
			const nmea_t  valid_nmea = GLOB::get().nmea_current();
			nmea_t  current_nmea;
			current_nmea.lat = valid_nmea.lat;
			current_nmea.lon = valid_nmea.lon;
			current_nmea.alt = valid_nmea.alt;
			current_nmea.fix_status = nmea_t::fix_status_t::kValid;

			if( GLOB::get().runtime_secs_ > 30 ) {
				current_nmea.lat = GLOB::get().cli.lat + 0.0001 * (GLOB::get().runtime_secs_ - 30);
				current_nmea.lon = GLOB::get().cli.lon + 0.0001 * (GLOB::get().runtime_secs_ - 30);
				current_nmea.alt = GLOB::get().cli.alt + 35000.0 * abs(sin(3.1415 * float(GLOB::get().runtime_secs_-30) / 300));
			}
			else {
				current_nmea.lat = GLOB::get().cli.lat;
				current_nmea.lon = GLOB::get().cli.lon;
				current_nmea.alt = GLOB::get().cli.alt;
			}
			GLOB::get().nmea_set(current_nmea);
			GLOB::get().gps_fix_now(); // typical time since uBlox msg read to here is under 1 millisecond
			GLOB::get().dynamics_add("alt", std::chrono::steady_clock::now(), current_nmea.alt);

			this_thread::sleep_for(chrono::seconds(1));
		}
	};


	// GPS thread. uBLOX or faked
	//
	LOG.log("main.log", "GPS thread start");
	function<void()> gps_loop(ublox_loop);
	if(G.cli.testgps)
		gps_loop = function<void()>(fake_gps_loop);
	std::thread ublox_thread( gps_loop );


    // DS18B20 temp sensor
    //
    const string ds18b20_device = find_ds18b20_device();
    if(ds18b20_device == "")
    {
		cerr<<C_RED<<"Failed ds18b20_device "<<ds18b20_device<<C_OFF<<endl;
        return 1;
    }
    cout<<"ds18b20_device "<<ds18b20_device<<endl;
	LOG.log("main.log", "ds18b20_device " + ds18b20_device);


	// ALL SENSORS THREAD
	//
	LOG.log("main.log", "SENSORS thread start");
	std::thread sensors_thread( [ds18b20_device, &PULSE]() {
		while(G_RUN) {
			// internal temp
			const float temp_int = read_temp_from_ds18b20(ds18b20_device);
			GLOB::get().dynamics_add("temp_int", std::chrono::steady_clock::now(), temp_int);
			this_thread::sleep_for( chrono::seconds(15) );
		}
	});

	// Flight State Thread
	// try guessing one of these states: kUnknown, kStandBy, kAscend, kDescend, kFreefall, kLanded
	//
	LOG.log("main.log", "flight state thread start");
	std::thread flight_state_thread( [&PULSE]() {
		const auto START_TIME = chrono::steady_clock::now(); // used to measure running time
		while(G_RUN) {
			PULSE.ping("flight_state");
			this_thread::sleep_for( chrono::seconds(1) );

			GLOB::get().runtime_secs_ = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - START_TIME).count();
			const nmea_t nmea = GLOB::get().nmea_last_valid();
			const auto dist_from_home = calc_gps_distance( nmea.lat, nmea.lon, nmea.alt,
										GLOB::get().cli.lat,  GLOB::get().cli.lon, 0);
			const auto alt = GLOB::get().dynamics_get("alt");
			const auto dAltAvg = alt.dVdT_avg();

			flight_state_t flight_state = flight_state_t::kUnknown;
			if(nmea.valid()) {
				if( abs(dist_from_home.dist_line_) < 200 and abs(dAltAvg) <= 3 )
					flight_state = flight_state_t::kStandBy;
				else if( abs(dist_from_home.dist_line_) > 2000 and abs(dAltAvg) <= 3 and alt.val() < 2000 )
					flight_state = flight_state_t::kLanded;
				else if(dAltAvg < -15)
					flight_state = flight_state_t::kFreefall;
				else if(dAltAvg < 0)
					flight_state = flight_state_t::kDescend;
				else if(dAltAvg >= 0)
					flight_state = flight_state_t::kAscend;
			}
			// cout<<alt.val()<<" "<<alt.dVdT()<<" "<<alt.dVdT_avg()<<" "<<dist_from_home.dist_line_<<endl;
			GLOB::get().flight_state_set( flight_state );
		}
	});

	// log saving thread
	//
	LOG.log("main.log", "log save thread start");
	std::thread log_save_thread([&LOG]() {
        while(G_RUN) {
            this_thread::sleep_for(chrono::seconds(10));
            LOG.save();
        }
    });


	// PULSE watch thread
	// if any of other threads doesn't ping at least once 10 seconds
	// stop reseting hardware watchdog -> REBOOT
	//
	const bool use_watchdog = G.cli.watchdog;
	std::thread pulse_watch_thread([&LOG, &PULSE, use_watchdog]() {
		if(not use_watchdog)
			return;
		LOG.log("main.log", "PULSE watch thread start");
        while(G_RUN) {
			this_thread::sleep_for(chrono::seconds(3));
			auto age_proc = PULSE.get_oldest_ping_age();
			float age_secs = float(std::get<0>(age_proc)) / 1e6;
			std::string proc = std::get<1>(age_proc);
			if(age_secs<15) {
				watchdog_reset();
			} else {
				cout<<"PULSE: WATCHDOG RESET HOLD !!! process:"<<proc<<" age:"<<age_secs<<endl;
				LOG.log("pulse.log", proc + " " + std::to_string(age_secs));
				LOG.save();
			}
		}
		watchdog_disable();
    });


	// ZeroMQ server
	LOG.log("main.log", "ZeroMQ thread start");
	zmq::context_t zmq_context(1);
    zmq::socket_t zmq_socket(zmq_context, ZMQ_REP);
    zmq_socket.bind ( string("tcp://*:" + to_string(G.cli.port)).c_str() );
	std::thread zmq_thread( [&zmq_socket]() {
		while(G_RUN) {
			zmq::message_t msg;
			auto res = zmq_socket.recv(msg); // using recv_result_t = std::optional<size_t>;
			if(!res.has_value())
				continue;
			string msg_str( (char*)msg.data(), msg.size() );
			zmq_socket.send( make_zmq_reply(msg_str), zmq::send_flags::none );
		}
	});


	// read last emited message ID and resume from that number
	int msg_id = 0;
	FILE* msgid_fh = fopen("./tracker.msgid", "r");
	if(msgid_fh) {
		fscanf(msgid_fh, "%d", &msg_id);
		msg_id += 10; // on power failure, last msg_id could be not written to disk
		fclose(msgid_fh);
		cout<<"Resume message ID "<<msg_id<<endl;
	}
	msgid_fh = fopen("./tracker.msgid", "w");

	// READ SENSORS, CONSTRUCT TELEMETRY MESSAGE, RF SEND TELEMETRY AND IMAGE
	//
	LOG.log("main.log", "main loop");
	ssdv_t ssdv_packets;
	while(G_RUN) {
		// TELEMETRY MESSAGE
		//
		for(int __mi=0; __mi<G.cli.msg_num and G_RUN; ++__mi) {
			++msg_id;
			PULSE.ping("main");

			stringstream  tlmtr_stream;

			// Callsign, ID, UTC:
			const nmea_t current_nmea = G.nmea_current();
			tlmtr_stream<<G.cli.callsign;
			tlmtr_stream<<","<<msg_id;
			tlmtr_stream<<","<<current_nmea.utc;

			// !! ONLY VALID LAT,LON,ALT ARE BEING SENT. LOOK INTO uBLOX THREAD.
			tlmtr_stream<<","<<current_nmea.lat<<","<<current_nmea.lon<<","<<current_nmea.alt;
			tlmtr_stream<<","<<current_nmea.sats<<","<<GLOB::get().gps_fix_age();

			// runtime
			tlmtr_stream<<","<<static_cast<int>(GLOB::get().runtime_secs_);

			// Sensors:
			tlmtr_stream<<","<<setprecision(1)<<fixed<<G.dynamics_get("temp_int").val();

			// average dAlt/dT
			tlmtr_stream<<","<<setprecision(1)<<fixed<<G.dynamics_get("alt").dVdT_avg();

			// flight state
			switch( G.flight_state_get() )
			{
				case flight_state_t::kUnknown:		tlmtr_stream<<",U";		break;
				case flight_state_t::kStandBy:		tlmtr_stream<<",S";		break;
				case flight_state_t::kAscend:		tlmtr_stream<<",A";		break;
				case flight_state_t::kDescend:		tlmtr_stream<<",D";		break;
				case flight_state_t::kFreefall:		tlmtr_stream<<",F";		break;
				case flight_state_t::kLanded:		tlmtr_stream<<",L";		break;
			}

			// CRC:
			const string msg_with_crc = string("\0",1) + "$$$" + tlmtr_stream.str() + '*' + CRC(tlmtr_stream.str());
			cout<<C_GREEN<<msg_with_crc<<C_OFF<<endl;

			LOG.log("sentences.log", msg_with_crc);

			// emit telemetry msg @RF
			auto mtx2_write_future = std::async( std::launch::async, [&]{
												 mtx2_write(radio_fd, msg_with_crc + '\n'); } );
			while( mtx2_write_future.wait_for(chrono::milliseconds(500)) != future_status::ready )
				PULSE.ping("main");

			// write last emited message ID
			if(msgid_fh) {
				rewind(msgid_fh);
				fprintf(msgid_fh, "%08d", msg_id);
			}
		}


		// send SSDV image next packet
		//
		if( G.flight_state_get() != flight_state_t::kFreefall and G.flight_state_get() != flight_state_t::kLanded ) {
			if( !ssdv_packets.size() and G.cli.ssdv_image.size() )
				cout<<"SSDV loaded "<<	ssdv_packets.load_file( G.cli.ssdv_image )	<<" packets from disk."<<endl;

			if( ssdv_packets.size() ) {
				PULSE.ping("main");
				auto tile = ssdv_packets.next_packet();
				cout<<"Send SSDV @RF. Left tiles: "<<ssdv_packets.size()<<endl;
				auto mtx2_write_future = std::async( std::launch::async, [&]{
													 mtx2_write( radio_fd, tile.data(), sizeof(tile) ); } );
				while( mtx2_write_future.wait_for(chrono::milliseconds(500)) != future_status::ready )
					PULSE.ping("main");

				if(!ssdv_packets.size())	// delete image when done
					system( (string("rm -f ") + G.cli.ssdv_image + " || echo \"Can't delete SSDV image.\"").c_str() );
			}
		}
	}


	// RELEASE RESOURCES
	//
	watchdog_disable();
	LOG.log("main.log", "release resources, close threads");

	if(msgid_fh)
		fclose(msgid_fh);

	cout<<"Closing sensors thread"<<endl;
	if( sensors_thread.joinable() )
		sensors_thread.join();

	cout<<"Closing uBlox I2C thread and device"<<endl;
	if( ublox_thread.joinable() )
		ublox_thread.join();
    close(uBlox_i2c_fd);

	cout<<"Closing flight state thread"<<endl;
	if( flight_state_thread.joinable() )
		flight_state_thread.join();

	cout<<"Closing UART Radio device"<<endl;
	close(radio_fd);
	gpioWrite (G.cli.hw_pin_radio_on, 0);

	cout<<"Closing gpio"<<endl;
	gpioTerminate();

	cout<<"Closing logs thread"<<endl;
	if(log_save_thread.joinable())
		log_save_thread.join();

	cout<<"Closing zmq thread"<<endl;
	if( zmq_thread.joinable() )
		zmq_thread.join(); // will return after next received message, or stuck forever if no messages comes in

	LOG.log("main.log", "saving last logs.");
	LOG.save();

    return 0;
}


int main(int argc, char** argv)
{
	return main1(argc, argv);
}
