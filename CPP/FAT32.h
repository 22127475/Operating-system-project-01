#pragma once
#include "base.h"

//unsigned long littleEdian(const BYTE* arr, unsigned int n);
//unsigned long littleEdian(const std::vector<BYTE>& arr);
//std::string hexToBin(const BYTE& hex);
//std::string normalization(const std::string& src);
//std::string toUpercase(const std::string& src);

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
	void print(bool isFull = true);
	bool isFolder();
	void getChild(std::vector<CFolder*>);
	bool canPrint();
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

public:
	CFolder* curPath;

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
	void printRDET(CFolder& folder, std::string time = "", bool last = false);
	void printRDET();
	std::vector<BYTE> printFolderInfo(CFolder* folder);
	CFolder* findFolderByName(CFolder& folder, std::string folderName, bool searchAll = true);


	// Volume
	std::string csd();

	void print_base_in4();

	bool cd(string path);
	wstring cwd();
	void ls();
	void tree();

	/*vector<BYTE> get_data(const string& name);*/
	void read(const string& name);
};








