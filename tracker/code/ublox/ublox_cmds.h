#pragma once

#include <cinttypes>
#include <cstddef>
#include <vector>

extern uint8_t UBX_CMD_EnableOutput_ACK_ACK[16];
extern uint8_t UBX_CMD_EnableOutput_ACK_NAK[16];
extern uint8_t UBX_CMD_NAV5_Airbororne1G[44];
extern uint8_t UBX_CMD_NAV5_Pedestrian[44];

// Disable GSV - GNSS Satellites in View
extern uint8_t UBX_CMD_GSV_OFF[16];
// Disable GLL - Latitude and longitude, with time of position fix and status
extern uint8_t UBX_CMD_GLL_OFF[16];
// Disable GSA - GNSS DOP and Active Satellites
extern uint8_t UBX_CMD_GSA_OFF[16];
// Disable VTG - Course over ground and Ground speed
extern uint8_t UBX_CMD_VTG_OFF[16];



void UBX_CHECKSUM(uint8_t* Buffer, const size_t BufferSz, uint8_t* CK_A, uint8_t* CK_B);

// o_cmd is user-owned array of size 10
void UBX_MAKE_PACKET_ACK(const uint8_t i_class, const uint8_t i_id, uint8_t* o_cmd);

// o_cmd is user-owned array of size 10
void UBX_MAKE_PACKET_NAK(const uint8_t i_class, const uint8_t i_id, uint8_t* o_cmd);

bool UBX_PACKET_EQ(uint8_t* i_data, const uint8_t* p_ref, const size_t ref_sz);

bool UBX_PACKET_EQ(const std::vector<char>& i_data, const uint8_t* p_ref, const size_t ref_sz);

