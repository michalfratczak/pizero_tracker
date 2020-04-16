#include <unistd.h> // write()
#include <signal.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>

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

const char* C_RED = 		"\033[1;31m";
const char* C_GREEN = 		"\033[1;32m";
const char* C_MAGENTA = 	"\033[1;35m";
const char* C_OFF =   		"\033[0m";

bool G_RUN = true; // keep threads running


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
	if(i_msg_str == "nmea")	{
		std::string reply_str = G.nmea_get().json();
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
	else {
		zmq::message_t reply( 7 );
		memcpy( (void*) reply.data(), "UNKNOWN", 7 );
		return reply;
	}
}


void CTRL_C(int sig)
{
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


	// uBLOX thread
	//
	std::thread ublox_thread( [uBlox_i2c_fd]() {
		while(G_RUN) {
			// std::this_thread::sleep_for( std::chrono::seconds(5) );
			const vector<char> ublox_data = uBLOX_read_msg(uBlox_i2c_fd); // typical blocking time: 0/1/1.2 seconds
			const string nmea_str = NMEA_get_last_msg(ublox_data.data(), ublox_data.size());
			// cout<<nmea_str<<endl;
			if( !NMEA_msg_checksum_ok(nmea_str) ) {
				cerr<<C_RED<<"NMEA Checksum Fail: "<<nmea_str<<C_OFF<<endl;
				continue;
			}

			nmea_t current_nmea;
			// REUSE LAT,LON,ALT FROM LAST VALID SENTENCE
			const nmea_t  valid_nmea = GLOB::get().nmea_get();
			current_nmea.lat = valid_nmea.lat;
			current_nmea.lon = valid_nmea.lon;
			current_nmea.alt = valid_nmea.alt;

			if( NMEA_parse(nmea_str.c_str(), current_nmea) ) {
				// only one at a time can be valid.
				// fix_status is from RMC, fix_quality is from GGA
				const bool gps_fix_valid =
								current_nmea.fix_status  == nmea_t::fix_status_t::kValid
							|| 	current_nmea.fix_quality != nmea_t::fix_quality_t::kNoFix;
				if(gps_fix_valid) {
					GLOB::get().nmea_set(current_nmea);
					GLOB::get().gps_fix_now(); // typical time since uBlox msg read to here is under 1 millisecond
					GLOB::get().dynamics_add("alt", std::chrono::steady_clock::now(), current_nmea.alt);
				}
			}
		}
	});


    // DS18B20 temp sensor
    //
    const string ds18b20_device = find_ds18b20_device();
    if(ds18b20_device == "")
    {
		cerr<<C_RED<<"Failed ds18b20_device "<<ds18b20_device<<C_OFF<<endl;
        return 1;
    }
    cout<<"ds18b20_device "<<ds18b20_device<<endl;


	// ALL SENSORS THREAD
	//
	std::thread sensors_thread( [ds18b20_device]() {
		while(G_RUN) {
			// internal temp
			const float temp_int = read_temp_from_ds18b20(ds18b20_device);
			GLOB::get().dynamics_add("temp_int", std::chrono::steady_clock::now(), temp_int);
			this_thread::sleep_for( chrono::seconds(5) );
		}
	});


	// ZeroMQ server
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


	// READ SENSORS, CONSTRUCT TELEMETRY MESSAGE, RF SEND TELEMETRY AND IMAGE
	//
	ssdv_t ssdv_tiles;
	int msg_id = 0;
	while(G_RUN)
	{
		// print dynamics
		cout<<C_MAGENTA<<"alt "<<G.dynamics_get("alt").dVdT()<<C_OFF<<endl;
		cout<<C_MAGENTA<<"temp_int "<<G.dynamics_get("temp_int").dVdT()<<C_OFF<<endl;

		// telemetry. G.cli.msg_num sentences before SSDV
		//
		for(int __mi=0; __mi<G.cli.msg_num && G_RUN; ++__mi)
		{
			++msg_id;

			// GPS data
			const nmea_t valid_nmea = G.nmea_get();

			// TELEMETRY MESSAGE
			stringstream  tlmtr_stream;
			// Callsign, ID, UTC:
			tlmtr_stream<<G.cli.callsign;
			tlmtr_stream<<","<<msg_id;
			tlmtr_stream<<","<<valid_nmea.utc;
			// !! ONLY VALID LAT,LON,ALT ARE BEING SENT. LOOK INTO uBLOX THREAD.
			tlmtr_stream<<","<<valid_nmea.lat<<","<<valid_nmea.lon<<","<<valid_nmea.alt;
			tlmtr_stream<<","<<valid_nmea.sats<<","<<GLOB::get().gps_fix_age();
			// Sensors:
			tlmtr_stream<<","<<setprecision(1)<<fixed<<G.dynamics_get("temp_int").val();
			// CRC:
			const string msg_with_crc = string("\0",1) + "$$$" + tlmtr_stream.str() + '*' + CRC(tlmtr_stream.str());
			cout<<C_GREEN<<msg_with_crc<<C_OFF<<endl;

			// emit telemetry msg @RF
			//
			mtx2_write(radio_fd, msg_with_crc + '\n');
		}


		// send SSDV image next packet
		//
		if( 	G.gps_fix_age() < 20
			&& 	G.dynamics_get("alt").dVdT() > -5  // not falling
		)
		{
			if( !ssdv_tiles.size() && G.cli.ssdv_image.size() )
				cout<<"SSDV loaded "<<	ssdv_tiles.load_file( G.cli.ssdv_image )	<<" packets from disk."<<endl;

			if( ssdv_tiles.size() )
			{
				auto tile = ssdv_tiles.next_tile();
				if(!ssdv_tiles.size())	// delete image when done
					system( (string("rm -f ") + G.cli.ssdv_image + " || echo \"Can't delete SSDV image.\"").c_str() );
				cout<<"Send SSDV @RF"<<endl;
				mtx2_write( radio_fd, tile.data(), sizeof(tile) );
			}
		}
	}


	// RELEASE RESOURCES
	//
	cout<<"Closing sensors thread"<<endl;
	sensors_thread.join();
	cout<<"Closing uBlox I2C thread and device"<<endl;
	ublox_thread.join();
    close(uBlox_i2c_fd);
	cout<<"Closing UART Radio device"<<endl;
	close(radio_fd);
	gpioWrite (G.cli.hw_pin_radio_on, 0);
	cout<<"Closing gpio"<<endl;
	gpioTerminate();
	cout<<"Closing zmq thread"<<endl;
	zmq_thread.join(); // will return after next received message, or stuck forever if no messages comes in


    return 0;
}


int main(int argc, char** argv)
{
	return main1(argc, argv);
}
