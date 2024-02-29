#include "FAT32.h"
// Support functions

// Converts byte array in little-endian to unsigned long.
unsigned long littleEdian(const BYTE *arr, unsigned int n)
{
	unsigned long res = 0;
	for (int i = n - 1; i >= 0; --i)
	{

		res = (res << 8) | arr[i];
	}
	return res;
}
unsigned long littleEdian(const std::vector<BYTE> &arr)
{
	unsigned long res = 0;
	for (int i = arr.size() - 1; i >= 0; --i)
		res = (res << 8) | arr[i];

	return res;
}

// Normalize string
std::string normalization(const std::string &src)
{
	std::string str = src;
	auto it = str.begin();
	while (it != str.end() && *it == ' ') {
		it = str.erase(it);
	}

	bool prevSpace = false;

	while (it != str.end()) {
		if (*it == ' ') {
			if (!prevSpace) {
				prevSpace = true;
				++it;
			}
			else {
				it = str.erase(it);
			}
		}
		else {
			prevSpace = false;
			++it;
		}
	}

	while (!str.empty() && str.back() == ' ') {
		str.pop_back();
	}
	return str;
}

// Convert Hex to Binary
std::string hexToBin(const BYTE &hex)
{
	std::string res;
	std::string hexTable[16] = { "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111" };

	res += hexTable[hex / 16];
	res += hexTable[hex % 16];

	return res;
}


// Folder
CFolder::CFolder()
{
	name = "";
	state = "";
	size = "";
	cluster = {};
	index = 0;
}
CFolder::CFolder(const std::string &name, const std::string &state, const std::string &size, const std::vector<long> &cluster,const int& index)
{
	this->name = name;
	this->state = state;
	this->size = size;
	this->cluster = cluster;
	this->index = index;


}
// Check hidden
bool CFolder::isHidden() 
{
	return state[6] == '1';
}

// Check system
bool CFolder::isSystem() 
{
	return state[5] == '1' || state[4] == '1';
}

// Print condition
bool CFolder::canPrint()
{
	
	bool res = false;
	for (int i = 0; i < this->name.size(); ++i)
	{
		if (name[i] >= 'A' && name[i] <= 'Z' || name[i] >= 'a' && name[i] <= 'z')
			res = true;

	}

	if (state[6] == '1' ||state[5] == '1' ||  state[4] == '1')
		return false;

	return res;
}

// Print Folder inforamtion
void CFolder::print(bool isFull)
{
	// Mode:
	// 1. Full description
	// 2. Only name
	if (isFull)
	{
		printf("Name: ");
	}
	printf("%s\n", this->name.c_str());
	if (isFull)
	{
		printf("State: %s\n", binToState().c_str());
		printf("Size: %s B\n", this->size.c_str());
		printf("-------------------------------\n");
       	printf("|         Start |   Number of    \n");
       	printf("|       Cluster |   Clusters     \n");
       	printf("|   ");
       	printf("% 11u | ", cluster[0]);
       	printf("% 6u\n", cluster.size());
        
	}
}

// Check ID of Folder and its child
CFolder* CFolder::findByID(const int& id)
{
	if (this->index == id)
		return this;
	for (int i = 0; i < this->subItem.size(); ++i)
		if (this->subItem[i]->index == id)
			return this->subItem[i];
	return nullptr;
}

// Check name of Folder and its child
CFolder *CFolder::findByName(std::string fileName)
{
	if (this->name == fileName)
		return this;
	for (int i = 0; i < this->subItem.size(); ++i)
		if (this->subItem[i]->name == fileName)
			return this->subItem[i];
	return nullptr;
}

// Convert state represent from Binary to String 
std::string CFolder::binToState()
{
	std::string stateList[] = { "", "", "Archive", "Directory", "VolLabel", "System", 
							  "Hidden", "ReadOnly"};
	std::string res = "";
	for (int i = 0; i < 8; ++i)
		if (state[i] == '1')
			res += stateList[i] + ", ";
	res.pop_back();
	res.pop_back();

	return res;
}

// Check Folder
bool CFolder::isFolder()
{
	return state[3] == '1';
}
// Set Folder's children
void CFolder::setChild(std::vector<CFolder * > child)
{
	this->subItem = child;
}
CFolder::~CFolder()
{
	std::vector<CFolder*>::const_iterator it;
	for (it = subItem.begin(); it != subItem.end(); ++it)
	{
		delete* it;
		it = subItem.erase(it);

	}
}

// FAT 32
// Init FAT32 object
FAT_32::FAT_32() {}

FAT_32::FAT_32(std::string volume)
{
	// Open disk and error handling
	if (volume.size() > 1)
	{
		exit(1);
	}

	diskName = "\\\\.\\" + volume + ":";

	FILE *f;
	f = fopen(diskName.c_str(), "rb");


	if (f == nullptr)
	{
		fclose(f);
		diskName = "";
		exit(0);
	}
	fclose(f);


	// Construct file system structure of disk
	this->readBootSector(); // Read Boot sector 
	this->readFatTable(); // Make FAT table
	this->makeRDET(); // Create Folder tree
	
	// Passing disk into path
	this->curPath = &root;
	std::string rootPath = "";
	rootPath += diskName[4];
	rootPath += diskName[5];

	root.name = rootPath;
	this->path.push_back(rootPath);

}
// Read boot sector of disk
void FAT_32::readBootSector()
{
	// Open disk and error handling
	FILE *f;
	f = fopen(diskName.c_str(), "rb");

	if (f == nullptr)
	{
		fclose(f);
        fprintf(stderr, "Error can not open %s", diskName.c_str());
		exit(0);
	}
	// Get boot sector data
	fseek(f, 0, SEEK_SET);
	fread(&this->bootSector, sizeof(bootSector), 1, f);
	fseek(f, 0, SEEK_SET);
	fclose(f);
}

// Print out the basic information
void FAT_32::printBootSector()
{
	printf("OEM ID: %s \n", bootSector.oemID);
	printf("Byte per sector: %d \n", littleEdian(bootSector.bytePerSector, 2));
	printf("Sector per cluster (Sc): %d \n", int(bootSector.sectorPerCluster));
	printf("Reserved sector (Sb): %d \n", littleEdian(bootSector.sectorOfBootsector, 2));
	printf("Number of FAT table (Nf): %d \n", int(bootSector.copyOfFAT));
	printf("Volume size (Sv): %d \n", littleEdian(bootSector.volumeSize, 4));
	printf("FAT size (Sf): %d \n", littleEdian(bootSector.FATSize, 4));

	// Secial information
	printf("First sector of FAT table: %d \n", littleEdian(bootSector.sectorOfBootsector, 2)); // After Sb
	
	printf("First sector of RDET: %d \n", littleEdian(bootSector.sectorOfBootsector, 2) + 
										  littleEdian(bootSector.FATSize, 4) * 
										  (unsigned int)bootSector.copyOfFAT);
	printf("First cluster of RDET: %d \n", littleEdian(bootSector.clusterStartOfRDET, 4));
	
	printf("First sector of Data: %d \n", littleEdian(bootSector.sectorOfBootsector, 2) + 
										  littleEdian(bootSector.FATSize, 4) * 
										  (unsigned int)bootSector.copyOfFAT); // Sb + Nf * Sf
	
	printf("FAT type: ");
	//  FAT32
	for (int i; i < 8; ++i)
		printf("%C", bootSector.FATType[i]);
	printf("\n");
}
// Covert cluster to sector
long FAT_32::clusterToSector(int cluster)
{
	// invalid cluster
	if (cluster < 2)
		return 0;

	long Sb = littleEdian(bootSector.sectorOfBootsector, 2);
	long Sf = littleEdian(bootSector.FATSize, 4);
	long Nf = int(bootSector.copyOfFAT);
	//Srdet = 0 in  FAT32
	long Sc = int(bootSector.sectorPerCluster);

	// Sector no i = Sb + Sf * Nf + Srdet + (k - 2) * Sc
	return (Sb + Sf * Nf + 0 + (cluster - 2) * Sc);
}

// Create FAT table array
void FAT_32::readFatTable()
{
	FILE *f;
	f = fopen(this->diskName.c_str(), "rb");

	// Calculate start position of FAT table
	long fatFirstCLuster = littleEdian(bootSector.sectorOfBootsector, 2);
	fseek(f, fatFirstCLuster * littleEdian(bootSector.bytePerSector, 2), SEEK_SET);

	// 8 byte : 4 current bytes + 4 next bytes
	BYTE cur[4] = { 1,1,1,1 };
	BYTE next[4] = { 1,1,1,1 };

	std::vector<std::vector<BYTE>> fatTable;
	// Stop condition : read 8 zero bytes
	int condition  = cur[0] + cur[1] + cur[2] + cur[3] + next[0] + next[1] + next[2] + next[3];
 	while (condition != 0)
	{
		fread(&cur, sizeof(cur), 1, f);
		fread(&next, sizeof(next), 1, f);

		fatTable.push_back({ cur[0], cur[1], cur[2], cur[3] });
		fatTable.push_back({ next[0], next[1], next[2], next[3] });
		condition  = cur[0] + cur[1] + cur[2] + cur[3] + next[0] + next[1] + next[2] + next[3];
	}
	// Store real FAT table
	fatMap = fatTable;

	fclose(f);
}

// Init Folder tree
void FAT_32::makeRDET()
{
	// Init root information
	long startSector = littleEdian(bootSector.sectorOfBootsector, 2) + 
					   littleEdian(bootSector.FATSize, 4) * 
					   (unsigned int)bootSector.copyOfFAT;
	this->root.name = diskName;
	this->root.state = "00010000";
	this->root.size = "0";
	this->root.cluster.push_back(littleEdian(bootSector.clusterStartOfRDET, 4));
	this->root.index = 0;

	// Make Folder tree of root
	int idx = 0;
	readRDET(startSector * littleEdian(bootSector.bytePerSector, 2), this->root, idx);
	

}

// Read RDET and make tree
void FAT_32::readRDET(long offset, CFolder &folder, int& idx)
{
	// Get number of entry and VFAT
	std::vector<int> entries = numberOfFile(offset);

	// Folder entry
	std::vector<CFolder *> res;
	// Flag check has Long File Name
	bool hasLFN = false;
	// Current name
	std::string name = "";
	// Store Long File Name
	std::vector<std::string> lfn;

	// Open file at offset
	FILE *f;
	f = fopen(diskName.c_str(), "rb");
	fseek(f, offset, SEEK_SET);

	// Iterate through each entry in SDET
	for (int i = 0; i < entries.size(); ++i)
	{
		// Read all VFAT
		for (int j = 0; j < entries[i]; ++j)
		{
			hasLFN = true;
			// Get all Long File Name of a Folder
			lfn.push_back(readVFAT(f));

		}

		// Read entry
		Entry endtry;
		fread(&endtry, sizeof(endtry), 1, f);

		if (endtry.name[0] == 0xE5) // Skip deleted Folder
		{
			lfn.clear();
			hasLFN = false;
			name = "";
		}
		else
		{
			if (!hasLFN) // 8.3
			{
				// Name
				for (int i = 0; i < 8; ++i)
				{
					name += endtry.name[i];
				}
				name = normalization(name);

				// Extension
				for (int i = 0; i < 3; ++i)
				{
					name += endtry.ext[i];
				}
				name = normalization(name);


			}
			else
			{
				// Get Long File Name
				for (int i = 0; i < lfn.size(); ++i)
					name += lfn[lfn.size() - 1 - i];
				name = normalization(name);

				// Delete strange character
				for (int i = 0; i < name.size(); ++i)
					if (name[i] == -1)
						name[i] = ' ';
				name = normalization(name);
				name.pop_back();

			}

		}
		
		// Get data
		std::string state = hexToBin(endtry.OB);
		long startCluster = littleEdian({ endtry.clusterLo[0], endtry.clusterLo[1], 
										  endtry.clusterHi[0], endtry.clusterHi[1]});
		std::vector<long> clusterLinkedList = clusterLinkListFrom(startCluster);
		std::string size = std::to_string(littleEdian(endtry.size, 4));

		// Create Folder
		CFolder* newFolder = new CFolder(name, state, size, clusterLinkedList, ++idx);
		// Set '.' character for 8.3
		if (!newFolder->isFolder() && !hasLFN && name.size() > 3 )
			name.insert(name.end() - 3, '.');
		newFolder->name = name;
		if (newFolder->name.size())
		res.push_back(newFolder);

		// Reset for next iteration
		lfn.clear();
		hasLFN = false;
		name = "";
	}
		
	fclose(f);

	// Set the child folders of the current folder
	folder.setChild(res);

	// Recursively read sub items for making tree
	for (int i = 0; i < folder.subItem.size(); ++i)
	{
		if (folder.subItem[i]->isFolder() && folder.subItem[i]->canPrint())
		{
			for (int j = 0; j < folder.subItem[i]->cluster.size(); ++j)
			{
				// Get SDET offset
				long subOffset = clusterToSector(folder.subItem[i]->cluster[j]) * 
								 littleEdian(bootSector.bytePerSector, 2);
				// Recursively readRDET sub item								 
				readRDET(subOffset, *folder.subItem[i], idx);
			}
		}
	}

}

// Make cluster chain
std::vector<long> FAT_32::clusterLinkListFrom(long startCluster)
{
	// Init result
	std::vector<long> res;
	res.push_back(startCluster);
	long curCluster = 0;

	// Loop until EOF 
	while (curCluster != 268435455 && curCluster >= 0 && curCluster <= fatMap.size())
	{
		// Get current cluster data
		curCluster = littleEdian({fatMap[startCluster][0], fatMap[startCluster][1], 
			 					 fatMap[startCluster][2], fatMap[startCluster][3] });
		// Store cluster
		res.push_back(curCluster);
		// Update start cluster
		startCluster = curCluster;
	}

	res.pop_back();
	return res;
}

// Print FAT table
void FAT_32::printFatTable()
{
	long curCluster = 0;
	for (int i = 0; i < fatMap.size(); ++i)
	{
		// Get current cluster
		curCluster = littleEdian({ fatMap[i][0], fatMap[i][1], fatMap[i][2], fatMap[i][3] });
		// Check if the cluster number indicates the end of file
		if (curCluster == 268435455)
			printf("%d : EOF\n", i);
		else
			printf("%d : %d\n", i, curCluster);
	}
}

// Get number of entry and VFAT
std::vector<int> FAT_32::numberOfFile(long offset)
{

	FILE *f;
	f = fopen(diskName.c_str(), "rb");
	fseek(f, offset, SEEK_SET);

	// Temporary entry to store read data
	Entry temp;
	
	// Initialize sub-counter and result vector
	temp.OB = 1;
	int sub = 0;
	
	std::vector<int> res;

	// Iterate through the entries until reaching the end marker
	while (temp.OB != 0)
	{
		fread(&temp, sizeof(temp), 1, f);

		// If VFAT increase sub counter
		if (temp.OB == 15)
		{
			sub++;
		}
		// If not push sub counter to result
		else
		{
			res.push_back(sub);
			// Reset counter
			sub = 0;
		}
	}

	fseek(f, 0, SEEK_SET);

	fclose(f);

	res.pop_back();
	return res;

}

// Get Long File Name
std::string FAT_32::readVFAT(FILE *f)
{
	std::string res = "";
	// VFAT structure
	BYTE index;
	uint16_t name1[5];
	BYTE OB;
	BYTE skip1[2];
	uint16_t name2[6];
	BYTE skip2[2];
	uint16_t name3[2];

	// Read data field from file
	fread(&index, sizeof(index), 1, f);
	fread(&name1, sizeof(name1), 1, f);
	fread(&OB, sizeof(OB), 1, f);
	fread(&skip1, sizeof(skip1), 1, f);
	fread(&name2, sizeof(name2), 1, f);
	fread(&skip2, sizeof(skip2), 1, f);
	fread(&name3, sizeof(name3), 1, f);


	// Get result from name
	for (int i = 0; i < 5; ++i)
		res += name1[i];

	for (int i = 0; i < 6; ++i)
		res += name2[i];

	for (int i = 0; i < 2; ++i)
		res += name3[i];


	return res;
}

// Print tree of Folder
void FAT_32::printRDET(CFolder &folder,bool printHidden, bool printSystem, std::string time, bool last)
{
	// Print name only
	printf("%s\n",folder.name.c_str());

	int lst = folder.subItem.size() - 1;
	// Count number of file will print depend on condition
	while (lst >= 0 && ((!printHidden && folder.isHidden()) 
                    || (!printSystem && folder.isSystem())))
    
		lst--;
	// Print sub item
	for (int i = 0; i < folder.subItem.size(); ++i)
	{
		if (folder.subItem[i]->isHidden() && !printHidden)
            continue;
        if (folder.subItem[i]->isSystem() && !printSystem)
            continue;

		printf("%s", (time + "+---").c_str());
		if (i != lst)
		{
			printRDET(*folder.subItem[i],printHidden, printSystem, (time + "|   "), false);
		}
		else
		{			
			printRDET(*folder.subItem[i],printHidden, printSystem, time + "    ", true);
		}
	}
	
	

}

// Print folder fully information
std::vector<BYTE> FAT_32::printFolderInfo(CFolder *folder)
{
	// Print folder fully information
	printf("--------------Info-------------\n");
	folder->print();
	long startSector = clusterToSector(folder->cluster[0]);
	long lastSector = startSector + int(bootSector.sectorPerCluster) * folder->cluster.size() - 1;
	printf("-------------------------------\n");
        	printf("|         Start |   Number of    \n");
        	printf("|        Sector |   Sectors      \n");
        	printf("|   ");
        	printf("% 11u | ", startSector);
            printf("% 6u\n", folder->cluster.size() * int(bootSector.sectorPerCluster));
	printf("------------CONTENT------------\n");


	std::vector<BYTE> res;
	// Print sub item tree if object is folder
	if (folder->isFolder())
	{
		tree();
	}
	// Get object data if not
	else
	{
		// Get extension
		std::string ext = "";
		int idx = folder->name.size();
		for (idx; folder->name[idx] != '.'; --idx)
		{
		}

		for (int i = idx + 1; i <= idx + 3; ++i)
			ext += folder->name[i];

		
		// Extension error handling
		if (ext != "txt" && ext != "TXT")
		{
			printf("Please use the appropriate reader to read this file.");
			return res;
		}

		// Get start and end offset
		long startOffset = startSector * littleEdian(bootSector.bytePerSector, 2);
		long lastOffset = (lastSector + 1) * littleEdian(bootSector.bytePerSector, 2);
		FILE *f;
		f = fopen(diskName.c_str(), "rb");
		fseek(f, startOffset, SEEK_SET);

		BYTE character;
		/// Read data from start to end offset and store to result
		for (startOffset; startOffset <= lastOffset; ++startOffset)
		{
			fread(&character, sizeof(BYTE), 1, f);
			if (character == 0)
				break;
			res.push_back(character);
		}
		
		fclose(f);
	}

	return res;
}

//Volume

// Boot sector information of disk
void FAT_32::print_base_in4()
{
	Volume::print_base_in4();
	printf("%C:\\ \n", this->diskName[4]);
	this->printBootSector();
}



// Check index for "--index" command
bool FAT_32::hasIndex(std::string& command)
{
	unsigned int index = 0;
	std::string temp = "";
	// Get first word in string
	while(command[index] != ' ')
	{
		temp += command[index++];
	}
	// If has --index/-i command agrument
	if (temp == "--index" || temp == "-i")
	{
		temp = "";
		// Get number after index
		for (int i = index + 1; i < command.size(); ++i)
		{
			temp += command[i];
		}
		// If it is number
		if (isNumber(temp))
		{
			index = std::stoi(temp);
			CFolder* tempPath = curPath;
			// Find index of sub item
			if (tempPath->index != index)
			{
				for (CFolder* subFolder : tempPath->subItem)
				{
					if (subFolder->index == index)
					{
						// Set input string to item
						command = subFolder->name;
						break;
					}
				}
			}
		}
		// If it is not number
		else
		{
        	fprintf(stderr, "Error: Invalid index");
			return false;

		}
		
	}
	// If not has --index/-i
	else
		return false;
	return true;
}

// Change directory 
bool FAT_32::cd(std::string path)
{
	// Split path
	std::vector <std::string > inPath = splitString(path);

	// Check if has --index/-i 
	if (inPath.size() == 1)
	{
		hasIndex(inPath[0]);
	}
	
	// Special input
	if (inPath[0] == ".")
		return true;
	if (inPath[0] == "..")
	{
		if (this->path.size() == 0 ||  this->path.size() == 1)
			return true;
		unsigned int back = 0;
		
		// Count the number of ".." to go back
		for (auto dots : inPath)
			if (dots == "..")
				back++;

		// If enough elements in path
		if (this->path.size() > back)
		{
			for (int i = 0; i < back; ++i)
			{
				this->path.pop_back(); // Pop 'back' elements from the path
			}

		}
		// If not back to disk
		else
		{
			// Save disk name
			std::string first = this->path[0];
			// Clear  path
			this->path.clear();
			//Set path to disk name
			this->path.push_back(first);
		}
		// Update input path to current path
		inPath = this->path;
	}

	// Copy of current object 
	CFolder* temp = curPath;
	// Copy of path
	std::vector<std::string> temPath = this->path;

	std::string disk = this->path[0];
	
	// If not start by disk name
	if (inPath[0] != disk)
	{
		// Traverse through each path in the input path
		for (auto pathName : inPath)
		{
			// Find the object by name in the current object
			temp = temp->findByName(pathName);

			// If not found or it not a folder return false
			if (temp == nullptr || !temp->isFolder())
			{
				printf("\rError: No such directory found");

				return false;
			}
			// Update tempPath
			temPath.push_back(pathName);

		}
	}
	else
	{
		temp = &root;
		temPath.clear();
		temPath.push_back(this->path[0]);
		for (int i = 1; i < inPath.size(); ++i)
		{
			// Find the object by name in the current object
			temp = temp->findByName(inPath[i]);
			// If not found or it not a folder return false
			if (temp == nullptr || !temp->isFolder())
			{
				printf("\rError: No such directory found");

				return false;
			}
			// Update tempPath
			temPath.push_back(inPath[i]);

		}

	}

    // Update the current path and current object
	this->path = temPath;
	this->curPath = temp;


	return true;



}

// Return path in wstring
std::wstring FAT_32::pwd()
{
	std::string str = csd();
	return std::wstring(str.begin(), str.end());
}

// Return path in string
std::string FAT_32::csd()
{
	std::string currentPath = "";
	for (std::string pathName : path)
		currentPath += pathName + "\\";
	if (path.size() != 1)
		currentPath.pop_back();

	return currentPath;
}

// List all sub item of object
void FAT_32::ls(bool printHidden, bool printSystem)
{
	// Mode
	//d: directory
	//a: archive
	//r: read-only
	//h: hidden
	//s: system
	int index[] = { 3,2,7,6,5};
	char type[] = { 'd', 'a', 'r','h','s'};
	
	Volume::ls();
	// Iterate through sub item
	for (auto subFolder : this->curPath->subItem)
	{
		// Print depend on input condition
		if (!printHidden &&  subFolder->isHidden())
			continue;
		if (!printSystem &&  subFolder->isSystem())
			continue;
		if (subFolder->name.size())
		{
			char mode[] = "------";
			for (int i = 0; i < 5; ++i)
			{
				// Check state and change mode base on it
				if (subFolder->state[index[i]] == '1')
					mode[i] = type[i];
			}
			// Check volume label
			if (subFolder->state[4] == '1')
				mode[4] = 's';

			// Print Mode | Index | Name
			printf("%s\t", mode);
			printf("%04u\t", subFolder->index);
			printf("%s\n", subFolder->name.c_str());
		}
			
		
	}
}

// Print object's tree base on condition
void FAT_32::tree(bool printHidden, bool printSystem)
{
	printRDET(*curPath,printHidden, printSystem);
}


// Print object's data
void FAT_32::read(const std::string& name)
{
	std::string tempName = name;
	// If not input name read current object
	if (tempName.size() == 0)
	{
		tempName = curPath->name;
	}
	// delete quotation-marks
	if (tempName.front() == '\"' && tempName.back() == '\"')
	{
		tempName.erase(0, 1);
		tempName.pop_back();
	}

	std::vector<BYTE> res;

	// Check if has --index/-i
	hasIndex(tempName);
	
	// If current object name is equal to input name
	if (curPath->name == tempName)
	{
		// Store content
		res = printFolderInfo(curPath);
	}
	// If not check in sub item
	else
	{
		for (auto subFolder : curPath->subItem)
		{
			if (subFolder->name == tempName)
			{
				if (subFolder->name != "." && subFolder->name != "..")
				{
					
					if (!cd(subFolder->name))
					{
						printf("\r");
					}
				}
				// Store content
				res = printFolderInfo(subFolder);
				if (subFolder->name != "." && subFolder->name != ".." && subFolder->isFolder())
					cd("..");
			}
		}
	}
	

	// Print out content
	for (char content : res)
		printf("%c", content);
	
}