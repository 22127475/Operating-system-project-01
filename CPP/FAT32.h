#pragma once
#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
//
//#include <iostream>

using BYTE = unsigned char;

unsigned long littleEdian(const BYTE* arr, unsigned int n);
unsigned long littleEdian(const std::vector<BYTE>& arr);
std::string hexToBin(const BYTE& hex);
std::string normalization(const std::string& src);
std::string toUpercase(const std::string& src);

struct BootSector
{
	BYTE jumpCode[3];
	BYTE oemID[8];
	BYTE bytePerSector[2];
	BYTE sectorPerCluster; //S_c
	BYTE sectorOfBootsector[2]; // S_b
	BYTE copyOfFAT; // N_f
	BYTE entryOfRDET[2];
	BYTE skip0[13];
	BYTE volumeSize[4]; // S_v
	BYTE FATSize[4]; //S_f
	BYTE skip_1[4];
	BYTE clusterStartOfRDET[4];
	BYTE skip_2[34];
	BYTE FATType[8];
	BYTE bootOS[420];
	BYTE endSign[2];
};

struct Entry
{
	BYTE name[8];
	BYTE ext[3];
	BYTE OB;
	BYTE skip1[8];
	BYTE clusterHi[2];
	BYTE skip2[4];
	BYTE clusterLo[2];
	BYTE size[4];
};

class CFolder
{
public:
	std::string name;
	std::string state;
	std::string size;
	std::vector<long> cluster;
	std::vector<CFolder* > subItem;
public:
	CFolder();
	CFolder(const std::string& name, const std::string& state, const std::string& size, const std::vector<long>& cluster);
	void print(bool printSubFolder = true, int time = 0);
	bool isFolder();
	void getChild(std::vector<CFolder*>);
	bool canPrint();
	CFolder* findByName(std::string fileName);
	~CFolder();

};

class FAT_32
{
private:
	BootSector bootSector;
	CFolder root;
	std::string diskName;
	std::vector<std::vector<BYTE>> fatMap;

public:
	FAT_32();
	FAT_32(std::string volume);
	void readBootSector();
	void printBootSector();

	long clusterToSector(int cluster);
	std::vector<long> clusterLinkListFrom(long startCluster);
	void readFatTable();
	void printFatTable();

	std::vector<int> numberOfFile(long offset);
	std::string readVFAT(FILE* f);
	//std::vector<CFolder*> readRDET(long offset);
	void readRDET(long offset, CFolder& folder);
	void makeRDET();
	//void printRDET(const CFolder& folder, int time);
	void printRDET();
	void printFolderInfo(CFolder& folder);
	void findFolderByName(std::string folderName);
};








