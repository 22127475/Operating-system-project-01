#pragma once
#include "base.h"

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
	int index;
public:
	CFolder();
	CFolder(const std::string& name, const std::string& state, const std::string& size, const std::vector<long>& cluster,const int& index);
	bool isFolder();
	void getChild(std::vector<CFolder*>);
	std::string binToState();
	bool canPrint();
	void print(bool isFull = true);
	CFolder* findByID(const int &id);
	CFolder* findByName(std::string fileName, bool searchAll = true);
	~CFolder();
};

class FAT_32 : public Volume
{
private:
	BootSector bootSector;
	CFolder root;
	std::vector<std::string> path;
	std::string diskName;
	std::vector<std::vector<BYTE>> fatMap;
	CFolder* curPath;
	
public:

	FAT_32();
	FAT_32(std::string volume);
	void readBootSector();
	void printBootSector();
	void readFatTable();
	void printFatTable();
	std::vector<int> numberOfFile(long offset);
	std::string readVFAT(FILE* f);
	std::vector<long> clusterLinkListFrom(long startCluster);
	long clusterToSector(int cluster);
	void readRDET(long offset, CFolder& folder, int& idx);
	void makeRDET();
	void printRDET(CFolder& folder, std::string time = "", bool last = false);
	void printRDET();
	CFolder* findFolderByName(CFolder& folder, std::string folderName, bool searchAll = true);
	std::vector<BYTE> printFolderInfo(CFolder* folder);


	// Volume
	void print_base_in4();
	bool cd(string path);
	std::string csd();
	wstring pwd();
	void tree();
	void ls();
	void read(const string& name = "");
};

// support functions
unsigned long littleEdian(const BYTE* arr, unsigned int n);
unsigned long littleEdian(const std::vector<BYTE>& arr);
std::string normalization(const std::string& src);
std::string toUpercase(const std::string& src);
std::string hexToBin(const BYTE& hex);






