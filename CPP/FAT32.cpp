#include "FAT32.h"

unsigned long littleEdian(const BYTE* arr, unsigned int n)
{
	unsigned long res = 0;
	for (int i = n - 1; i >= 0; --i)
		res = (res << 8) | arr[i];
	return res;
}

unsigned long littleEdian(const std::vector<BYTE>& arr)
{
	unsigned long res = 0;
	for (int i = arr.size() - 1; i >= 0; --i)
		res = (res << 8) | arr[i];

	return res;
}

std::string normalization(const std::string& src)
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

std::string toUpercase(const std::string& src)
{
	std::string res = src;
	for (int i = 0; i < res.size(); ++i)
	{
		if (res[i] >= 'a' && res[i] <= 'z')
		{
			res[i] = res[i] - 'a' + 'A';
		}
	}
	return res;
}

std::string hexToBin(const BYTE& hex)
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
}
CFolder::CFolder(const std::string& name, const std::string& state, const std::string& size, const std::vector<long>& cluster)
{
	this->name = name;
	this->state = state;
	this->size = size;
	this->cluster = cluster;
}

bool CFolder::canPrint()
{
	bool res = false;
	for (int i = 0; i < this->name.size(); ++i)
	{
		if (name[i] >= 'A' && name[i] <= 'Z' || name[i] >= 'a' && name[i] <= 'z')
			res = true;

	}

	return res;
}

void CFolder::print(bool printSubFolder, int time)
{
	
	if (this->canPrint())
	{
		int idx = 1 << time;

		for (int i = 0; i < idx; ++i)
			printf("-");

		printf("Name: %s|\n", this->name.c_str());
		
		for (int i = 0; i < idx; ++i)
			printf(" ");

		printf("State: %s\n", this->state.c_str());
		
		//if (!this->isFolder())
		for (int i = 0; i < idx; ++i)
			printf(" ");

		printf("Size: %s\n", this->size.c_str() );
		for (int i = 0; i < idx; ++i)
			printf(" ");

		printf("Cluster: ");
		for (int i = 0; i < cluster.size() - 1; ++i)
		{
			printf("%d,",cluster[i]);
		}
		if (cluster.size() > 0)
			printf("%d", cluster[cluster.size() - 1]);
		printf("\n");
		
		if (printSubFolder)
			for (int i = 0; i < this->subItem.size(); ++i)
				this->subItem[i]->print(printSubFolder,time + 1);
	}

}

CFolder* CFolder::findByName(std::string fileName)
{
	if (this->name == fileName)
		return this;
	for (auto subFolder : this->subItem)
	{
		CFolder* found = subFolder->findByName(fileName);
		if (found != nullptr)
			return found;
	}

	return nullptr;
}

bool CFolder::isFolder()
{
	std::string folder[] = { "00010000", "00010010" };
	for (int i = 0; i < 2; ++i)
	{
		if (this->state == folder[i])
			return true;
	}

	return false;
	/*return state[3] == '1';*/
}

void CFolder::getChild(std::vector<CFolder* > child)
{
	this->subItem = child;
}

CFolder::~CFolder()
{
	//deleteSubFolder(this);
}

// FAT 32

FAT_32::FAT_32()
{

}

FAT_32::FAT_32(std::string volume)
{
	if (volume.size() > 1)
	{
		printf("[ERROR]: INVALID DISK NAME SIZE (\" %d \" instead of 1) \n", volume.size());
		exit(1);
	}

	diskName = "\\\\.\\" + toUpercase(volume) + ":";
	
	FILE* f;
	f = fopen(diskName.c_str(), "rb");
	if (f == nullptr)
	{
		fclose(f);
		printf( "[ERROR]: CANNOT OPEN %s", diskName.c_str() );
		diskName = "";
		exit(0);
	}
	fclose(f);

	this->readBootSector();
	this->readFatTable();
	this->makeRDET();

}

void FAT_32::readBootSector()
{
	FILE* f;
	f = fopen(diskName.c_str(), "rb");

	if (f == nullptr)
	{
		fclose(f);
		printf("[ERROR]: CANNOT OPEN %s\n", diskName.c_str());
		exit(0);
	}
	fseek(f, 0, SEEK_SET);
	fread(&this->bootSector, sizeof(bootSector), 1, f);
	fseek(f, 0, SEEK_SET);
	fclose(f);
}

void FAT_32::printBootSector()
{
	printf("Jump code: 0x%02X%02X%02 \n", bootSector.jumpCode[2], bootSector.jumpCode[1], bootSector.jumpCode[0]);
	printf("OEM ID: %s \n", bootSector.oemID);
	printf("Byte per sector: %d \n", littleEdian(bootSector.bytePerSector, 2));
	printf("Sector per cluster (Sc): %d \n", int(bootSector.sectorPerCluster));
	printf("Reserved sector (Sb): %d \n", littleEdian(bootSector.sectorOfBootsector, 2));
	printf("Number of FAT table (Nf): %d \n", int(bootSector.copyOfFAT));
	printf("Volume size (Sv): %d \n", littleEdian(bootSector.volumeSize, 4));
	printf("FAT size (Sf): %d \n", littleEdian(bootSector.FATSize, 4));
	printf("First sector of FAT table: %d \n",littleEdian(bootSector.sectorOfBootsector, 2));
	printf("First sector of RDET: %d \n", littleEdian(bootSector.sectorOfBootsector, 2) + littleEdian(bootSector.FATSize, 4) * (unsigned int)bootSector.copyOfFAT);
	printf("First cluster of RDET: %d \n", littleEdian(bootSector.clusterStartOfRDET, 4));
	printf("First sector of Data: %d \n", littleEdian(bootSector.sectorOfBootsector, 2) + littleEdian(bootSector.FATSize, 4) * (unsigned int)bootSector.copyOfFAT);
	printf("Data size: %d \n", littleEdian(bootSector.volumeSize, 4) - littleEdian(bootSector.sectorOfBootsector, 2) - littleEdian(bootSector.FATSize, 4) * (unsigned int)bootSector.copyOfFAT);
	printf("FAT type: ");
	int i = 0;
	for (i; i < 8; ++i)
		printf("%C", bootSector.FATType[i]);
	printf("\n");
}

long FAT_32::clusterToSector(int cluster)
{
	if (cluster < 2)
		return 0;
	long Sb = littleEdian(bootSector.sectorOfBootsector, 2);
	long Sf = littleEdian(bootSector.FATSize, 4);
	long Nf = int(bootSector.copyOfFAT);
	//Srdet = 0 in  FAT32
	long Sc = int(bootSector.sectorPerCluster);


	return (Sb + Sf * Nf + 0 + (cluster - 2) * Sc);
}

void FAT_32::readFatTable() 
{
	FILE* f;
	f = fopen(this->diskName.c_str(), "rb");
	long fatFirstCLuster = littleEdian(bootSector.sectorOfBootsector, 2);
	fseek(f, fatFirstCLuster * 512, SEEK_SET);

	BYTE temp[4] = { 1,1,1,1 };


	std::vector<std::vector<BYTE>> fatTable;
	while (temp[0] + temp[1] + temp[2] + temp[3] != 0)
	{
		fread(&temp, sizeof(temp), 1, f);
		fatTable.push_back({ temp[0], temp[1], temp[2], temp[3] });
	}
	fatMap = fatTable;
	fclose(f);
}

void FAT_32::makeRDET()
{
	long startCluster = littleEdian(bootSector.sectorOfBootsector, 2) + littleEdian(bootSector.FATSize, 4) * (unsigned int)bootSector.copyOfFAT;


	this->root.name = "Root";
	this->root.state = "00010000";
	this->root.size = "0";
	this->root.cluster = { 0,1,2 };

	readRDET(startCluster * 512, this->root);
}

void FAT_32::readRDET(long offset, CFolder& folder)
{
	std::vector<int> entries = numberOfFile(offset);

	std::vector<CFolder*> res;

	bool hasLFN = false;
	std::string name = "";
	FILE* f;
	f = fopen(diskName.c_str(), "rb");

	fseek(f, offset, SEEK_SET);

	std::vector<std::string> lfn;


	for (int i = 0; i < entries.size(); ++i)
	{
		for (int j = 0; j < entries[i]; ++j)
		{
			hasLFN = true;
			lfn.push_back(readVFAT(f));

		}

		Entry endtry;
		fread(&endtry, sizeof(endtry), 1, f);
		if (!hasLFN)
		{

			for (int i = 0; i < 8; ++i)
			{
				name += endtry.name[i];
			}
			name = normalization(name);

			name += ".";
			for (int i = 0; i < 3; ++i)
			{
				name += endtry.ext[i];
			}
			name = normalization(name);

		}
		else
		{
			for (int i = 0; i < lfn.size(); ++i)
				name += lfn[lfn.size() - 1 - i];
			name = normalization(name);


			for (int i = 0; i < name.size(); ++i)
				if (name[i] == -1)
					name[i] = ' ';
			name = normalization(name);

			name.pop_back();
		}



		std::string state = hexToBin(endtry.OB);
		long startCluster = littleEdian({ endtry.clusterLo[0], endtry.clusterLo[1], endtry.clusterHi[0], endtry.clusterHi[1] });

		std::vector<long> clusterLinkedList = clusterLinkListFrom(startCluster);
		std::string size = std::to_string(littleEdian(endtry.size, 4));
		CFolder* newFolder = new CFolder(name, state, size, clusterLinkedList);
		res.push_back(newFolder);

		lfn.clear();
		hasLFN = false;
		name = "";

	}



	fclose(f);

	folder.getChild(res);


	for (int i = 2; i < folder.subItem.size(); ++i)
	{
		if (folder.subItem[i]->isFolder())
		{
			for (int j = 0; j < folder.subItem[i]->cluster.size(); ++j)
			{
				long subOffset = clusterToSector(folder.subItem[i]->cluster[j]) * 512;
				readRDET(subOffset, *folder.subItem[i]);
			}
		}
	}
}



std::vector<long> FAT_32::clusterLinkListFrom(long startCluster)
{
	std::vector<long> res;
	res.push_back(startCluster);
	long curCluster = 0;
	while (curCluster != 268435455 && curCluster >= 0 && curCluster <= fatMap.size())
	{
		curCluster = littleEdian({ fatMap[startCluster][0], fatMap[startCluster][1], fatMap[startCluster][2], fatMap[startCluster][3] });
		startCluster = curCluster;
		res.push_back(curCluster);
	}
	res.pop_back();
	return res;
}



void FAT_32::printFatTable()
{
	long clusterNo = 0;
	for (int i = 0; i < fatMap.size(); ++i)
	{
		clusterNo = littleEdian({ fatMap[i][0], fatMap[i][1], fatMap[i][2], fatMap[i][3] });
		if (clusterNo == 268435455)
			printf("%d : EOF\n", i);
		else
			printf("%d : %d\n", i, clusterNo);
	}
}

std::vector<int> FAT_32::numberOfFile(long offset)
{
	FILE* f;
	f = fopen(diskName.c_str(), "rb");
	fseek(f, offset, SEEK_SET);
	Entry temp;
	temp.OB = 1;
	int sub = 0;
	std::vector<int> res;

	while (temp.OB != 0)
	{
		fread(&temp, sizeof(temp), 1, f);
		if (temp.OB == 15)
		{
			sub++;
		}
		else
		{
			res.push_back(sub);
			sub = 0;
		}
	}

	fseek(f, 0, SEEK_SET);

	fclose(f);
	res.pop_back();


	return res;

}

std::string FAT_32::readVFAT(FILE* f)
{
	std::string res = "";
	BYTE index;
	uint16_t name1[5];
	BYTE OB;
	BYTE skip1[2];
	uint16_t name2[6];
	BYTE skip2[2];
	uint16_t name3[2];


	fread(&index, sizeof(index), 1, f);
	fread(&name1, sizeof(name1), 1, f);
	fread(&OB, sizeof(OB), 1, f);
	fread(&skip1, sizeof(skip1), 1, f);
	fread(&name2, sizeof(name2), 1, f);
	fread(&skip2, sizeof(skip2), 1, f);
	fread(&name3, sizeof(name3), 1, f);


	for (int i = 0; i < 5; ++i)
		res += name1[i];

	for (int i = 0; i < 6; ++i)
		res += name2[i];

	for (int i = 0; i < 2; ++i)
		res += name3[i];


	return res;
}


void FAT_32::printRDET()
{
	root.print();

}

void FAT_32::printFolderInfo(CFolder& folder)
{
	if (folder.isFolder())
	{
		folder.print(false);
		int numberOfCluster = folder.cluster.size();
		if (numberOfCluster <= 3 && numberOfCluster > 0)
		{
			for (int i = 0; i < numberOfCluster - 1; ++i)
				printf("%d, ", folder.cluster[i]);
			printf("%d", folder.cluster[numberOfCluster - 1]);
		}
		else
		{
			for (int i = 0; i < 3; ++i)
				printf("%d, ", folder.cluster[i]);
			printf("...,%d", folder.cluster[numberOfCluster - 1]);

		}


		printf("\n");

		for (auto subFolder : folder.subItem)
		{
			subFolder->print(false);
			printf("Sector: ");
			
			int numberOfCluster = subFolder->cluster.size();
			if (numberOfCluster <= 3 && numberOfCluster > 0)
			{
				for (int i = 0; i < numberOfCluster - 1; ++i)
					printf("%d, ", subFolder->cluster[i]);
				printf("%d", subFolder->cluster[numberOfCluster - 1]);
			}
			else
			{
				for (int i = 0; i < 3; ++i)
					printf("%d, ", subFolder->cluster[i]);
				printf("...,%d", subFolder->cluster[numberOfCluster - 1]);

			}
				

			printf("\n");
		}
	}
	else
	{
		/*std::string ext = "";
		int idx = folder.name.size() - 1;
		for (idx; folder.name[idx] != '.'; --idx)
		{
		}

		for (int i = idx + 1; i <= idx + 3; ++i)
			ext += folder.name[i];

		if (ext != "txt")
		{
			printf("Open: \n\t %s \nwith another app \n",folder.name.c_str());
			return;
		}*/

		folder.print(false);


		long startOffset = clusterToSector(folder.cluster[0]) * 512;
		long lastOffset = clusterToSector(folder.cluster[folder.cluster.size() - 1] + 1 + 1) * 512;

		std::string content = "";
		FILE* f;
		f = fopen(diskName.c_str(), "rb");
		fseek(f, startOffset, SEEK_SET);

		BYTE character;
		for (startOffset; startOffset <= lastOffset; ++startOffset)
		{
			fread(&character, sizeof(BYTE), 1, f);
			if (character == 0)
				break;
			content += character;
		}
		fclose(f);

		printf( "Content: \n");
		printf("%s\n", content.c_str());

	}
}

void FAT_32::findFolderByName(std::string folderName)
{

	CFolder* res = this->root.findByName(folderName);
	if (res == nullptr)
	{
		printf("[ERROR] CANNOT FIND \"%s\"\n", folderName.c_str());
		exit(0);
	}
	
	printFolderInfo(*res);
}