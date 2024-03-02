#pragma once
#include "base.h"

// Boot sector structure of FAT32
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

// Entry structure of FAT
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

// Component folder structure
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
	// Constructor
	CFolder();
	CFolder(const std::string& name, const std::string& state, const std::string& size, 
		    const std::vector<long>& cluster,const int& index);
	bool isFolder(); // Check Folder
	void setChild(std::vector<CFolder*>); // Set Folder's children
	std::string binToState(); // Convert state represent from Binary to String
	bool isHidden(); // Check hidden
	bool isSystem(); // Check system
	bool isValidName();
	bool canPrint(); // Print condition
	void print(bool isFull = true); // Print Folder inforamtion
	CFolder* findByID(const int &id); // Check ID of Folder and its child
	CFolder* findByName(std::string fileName); // Check name of Folder and its child
	// Destructor
	~CFolder();
};

class FAT_32 : public Volume
{
private:
	BootSector bootSector; // Store boot sector
	CFolder root; // Store root directory tree
	std::vector<std::string> path; // Store path
	std::string diskName; // Disk's name
	std::vector<std::vector<BYTE>> fatMap; // FAT table
	CFolder* curPath; // Current object
	
public:
	// Constructor
	FAT_32();
	FAT_32(std::string volume);
	void readBootSector(); // Read boot sector of disk
	void printBootSector(); // Print out the basic information
	void readFatTable(); // Create FAT table array
	void printFatTable(); // Print FAT table
	std::vector<int> numberOfFile(long offset); // Get number of entry and VFAT
	std::string readVFAT(FILE* f); // Get Long File Name
	std::vector<long> clusterLinkListFrom(long startCluster); // Make cluster chain
	long clusterToSector(int cluster); // Covert cluster to sector
	void readRDET(long offset, CFolder& folder, int& idx); // Read RDET and make tree
	void makeRDET(); // Init Folder tree
	void printRDET(CFolder& folder,bool printHidden = false, bool printSystem = false,
				   std::string time = "", bool last = false); // Print tree of Folder
	bool hasIndex(std::string& command); // Check index for "--index" command
	std::vector<BYTE> printFolderInfo(CFolder* folder); // Print folder fully information

	// Volume
	void print_base_in4(); // Boot sector information of disk
	bool cd(string path); // Change directory 
	std::string csd(); // Return path in wstring
	wstring pwd(); // Return path in string
	void tree(bool printHidden = false, bool printSystem = false); // Print object's tree base on condition
	void ls(bool printHidden = false, bool printSystem = false); // List all sub item of object
	void read(const string& name = ""); // Print object's data
};

// support functions

// Converts byte array in little-endian to unsigned long.
unsigned long littleEdian(const BYTE* arr, unsigned int n);
unsigned long littleEdian(const std::vector<BYTE>& arr);

std::string normalization(const std::string& src); // Normalize string
std::string hexToBin(const BYTE& hex); // Convert Hex to Binary