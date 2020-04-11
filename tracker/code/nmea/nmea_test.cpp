#include <iostream>
#include <string.h>

#include "nmea.h"

int main()
{

    using namespace std;

    char msg[] = "$this is multiple $message string in one*xy";

    size_t msg_in;
    size_t msg_out;
    NMEA_get_last_msg(msg, strlen(msg), msg_in, msg_out);

    char msg2[300] = {'\0'};
    memcpy(msg2, msg+msg_in, msg_out-msg_in+1);

    cout<<msg[msg_in]<<" "<<msg[msg_out]<<endl;
    cout<<msg2<<endl;

    cout<<NMEA_get_last_msg( msg, strlen(msg) )<<endl;

    char nmea_msg_ok[] = "xxxx$yyyyy$GNGGA,170814.00,,,,,0,00,99.99,,,,,,*73xxxx";
    string nmea_msg_str = NMEA_get_last_msg( nmea_msg_ok, strlen(nmea_msg_ok) );
    if(nmea_msg_str != "")
        cout<<nmea_msg_str<<" "<<NMEA_msg_checksum_ok(nmea_msg_str)<<endl;
    else
        cout<<"empty nmea_msg_str"<<endl;

    char nmea_msg_empty[] = "xxxxGNGGA,170814.00,,,,,0,00,99.99,,,,,,*73xxxx";
    string nmea_msg_empty_str = NMEA_get_last_msg( nmea_msg_empty, strlen(nmea_msg_empty) );
    if(nmea_msg_empty_str != "")
        cout<<nmea_msg_empty_str<<" "<<NMEA_msg_checksum_ok(nmea_msg_empty_str)<<endl;
    else
        cout<<"nmea msg not found"<<endl;

    cout<<"\n\nnmea_t copy test"<<endl;
    nmea_t A, B;
    A.utc[0] = '1';
    A.utc[1] = '2';
    A.utc[2] = '3';
    A.utc[3] = '4';
    A.utc[4] = '5';
    A.utc[5] = '6';
    cout<<A.str()<<endl;
    cout<<B.str()<<endl;
    B = A;
    cout<<B.str()<<endl;

    cout<<"\n\nnmea_t::seconds"<<endl;

    cout<<A.seconds()<<endl;

}
