#pragma once
using BYTE = unsigned char;

class App
{
private:
	BootSector bootSector;
	CFolder root;
	std::string diskName;
	std::vector<std::vector<BYTE>> fatMap;

public:

	void scanDisk(std::string& diskName);
	unsigned long littleEdian(BYTE* arr, unsigned int n);
	unsigned long littleEdian(std::vector<BYTE> arr);
	std::string hexToBin(BYTE hex);
	void mainloop();

	void readBootSector();
	void printBootSector();

	long clusterToSector(int cluster);
	std::vector<long> clusterLinkListFrom(long startCluster);
	void readFatTable();
	void printFatTable();

	void normalization(std::string&);
	std::vector<int> numberOfFile(long offset);
	std::string readVFAT(FILE* f);
	std::vector<CFolder*> readRDET(long offset);
	void readRDET(long offset, CFolder& folder);
	void makeRDET();
	void printRDET(CFolder& folder, int time);
	void printRDET();
	void printFolderInfo(CFolder& folder);
	void findFolderByName();
};