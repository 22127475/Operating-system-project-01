#include <cstdint>
#pragma once

struct BootSector
{
	uint8_t jumpCode[3];
	uint8_t oemID[8];
	uint8_t bytePerSector[2];
	uint8_t sectorPerCluster; //S_c
	uint8_t sectorOfBootsector[2]; // S_b
	uint8_t copyOfFAT; // N_f
	uint8_t entryOfRDET[2];
	/*
	uint8_t sectorOfVol[2];
	uint8_t mediaType;
	uint8_t sectorOfFAT[2];
	uint8_t sectorOfTrack[2];
	uint8_t head[2];
	uint8_t _will_change_name[4];
	*/
	uint8_t skip0[13];
	uint8_t volumeSize[4]; // S_v
	uint8_t FATSize[4]; //S_f
	uint8_t skip_1[4];
	uint8_t clusterStartOfRDET[4];
	uint8_t skip_2[34];
	uint8_t FATType[8];
	uint8_t bootOS[420];
	uint8_t endSign[2];
};

struct VFAT
{
	uint8_t index;
	uint16_t name1[5]; // 5 utf16
	uint8_t OB;
	uint8_t skip1[2];
	uint16_t name2[6];// 6 utf16
	uint8_t skip2[2];
	uint16_t name3[2];// 2 utf16
};

struct Entry
{
	uint8_t name[8];
	uint8_t ext[3];
	uint8_t OB;
	uint8_t skip1[8];
	uint8_t clusterHi[2];
	uint8_t skip2[4];
	uint8_t clusterLo[2];
	uint8_t size[4];
};