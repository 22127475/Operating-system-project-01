
#include "FAT32.h"

unsigned long littleEdian(const BYTE *arr, unsigned int n)
{
	unsigned long res = 0;
	for (int i = n - 1; i >= 0; --i)
		res = (res << 8) | arr[i];
	return res;
}

unsigned long littleEdian(const std::vector<BYTE> &arr)
{
	unsigned long res = 0;
	for (int i = arr.size() - 1; i >= 0; --i)
		res = (res << 8) | arr[i];

	return res;
}

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

std::string toUpercase(const std::string &src)
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
}
CFolder::CFolder(const std::string &name, const std::string &state, const std::string &size, const std::vector<long> &cluster)
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

	if (state[5] == '1')
		return false;

	return res;
}

void CFolder::print(bool isFull)
{

	if (this->canPrint())
	{
		if (isFull)
		{
			printf("Name: ");
		}
		printf("%s\n", this->name.c_str());

		if (isFull)
		{

			/*for (int i = 0; i < idx; ++i)
				printf(" ");*/

			printf("State: %s\n", this->state.c_str());

			//if (!this->isFolder())
			/*for (int i = 0; i < idx; ++i)
				printf(" ");*/

			printf("Size: %s\n", this->size.c_str());
			//for (int i = 0; i < idx; ++i)
			//	printf(" ");

			printf("Cluster: ");
			for (int i = 0; i < cluster.size() - 1; ++i)
			{
				printf("%d,", cluster[i]);
			}
			if (cluster.size() > 0)
				printf("%d", cluster[cluster.size() - 1]);
			printf("\n");
		}



	}

}


//void CFolder::print(bool isFull, bool printSubFolder, std::string time, bool last)
//{
//
//	
//
//
//}

CFolder *CFolder::findByName(std::string fileName, bool searchAll)
{
	if (this->name == fileName)
		return this;
	if (searchAll)
	{
		for (auto subFolder : this->subItem)
		{
			CFolder *found = subFolder->findByName(fileName, searchAll);
			if (found != nullptr)
				return found;
		}

	}
	else
	{
		for (auto subFolder : this->subItem)
		{
			if (subFolder->name == fileName)
				return subFolder;
		}
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

void CFolder::getChild(std::vector<CFolder * > child)
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

	FILE *f;
	f = fopen(diskName.c_str(), "rb");
	if (f == nullptr)
	{
		fclose(f);
		printf("[ERROR]: CANNOT OPEN %s", diskName.c_str());
		diskName = "";
		exit(0);
	}
	fclose(f);

	this->readBootSector();
	this->readFatTable();
	this->makeRDET();

	this->curPath = &root;

	std::string rootPath = "";
	rootPath += diskName[4];
	rootPath += diskName[5];
	rootPath += "\\";


	root.name = rootPath;
	this->path.push_back(rootPath);

}

void FAT_32::readBootSector()
{
	FILE *f;
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
	printf("First sector of FAT table: %d \n", littleEdian(bootSector.sectorOfBootsector, 2));
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
	FILE *f;
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
	long startSector = littleEdian(bootSector.sectorOfBootsector, 2) + littleEdian(bootSector.FATSize, 4) * (unsigned int)bootSector.copyOfFAT;


	this->root.name = diskName;
	this->root.state = "00010000";
	this->root.size = "0";
	this->root.cluster.push_back(littleEdian(bootSector.clusterStartOfRDET, 4));

	readRDET(startSector * 512, this->root);
}

void FAT_32::readRDET(long offset, CFolder &folder)
{
	std::vector<int> entries = numberOfFile(offset);

	std::vector<CFolder *> res;

	bool hasLFN = false;
	std::string name = "";
	FILE *f;
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
		CFolder *newFolder = new CFolder(name, state, size, clusterLinkedList);
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
	FILE *f;
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

std::string FAT_32::readVFAT(FILE *f)
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

void FAT_32::printRDET(CFolder &folder, std::string time, bool last)
{

	folder.print(false);

	int lst = folder.subItem.size() - 1;
	while (lst >= 0 && !folder.subItem[lst]->canPrint())
		lst--;
	for (int i = 0; i < folder.subItem.size(); ++i)
	{
		if (!folder.subItem[i]->canPrint())
			continue;
		if (time != "")
			printf("|");
		if (i != lst)
		{
			//printf("   ");
			// printf("%s", (time + char(195) + char(196)).c_str());
			// printRDET(*folder.subItem[i], (time + char(179) + " "), false);
			printf("%s", (time + time + time + "+---").c_str());
			printRDET(*folder.subItem[i], (time + " "), false);
		}
		else
		{
			// printf("%s", (time + char(192) + char(196)).c_str());

			printf("%s", (time + time + time + "\\---").c_str());
			printRDET(*folder.subItem[i], time + "  ", true);
		}
	}

}

void FAT_32::printRDET()
{
	printRDET(root);

}

std::vector<BYTE> FAT_32::printFolderInfo(CFolder *folder)
{
	printf("Input: %s\n", folder->name.c_str());
	std::vector<BYTE> res;
	if (folder->isFolder())
	{
		tree();
	}
	else
	{

		std::string ext = "";
		int idx = folder->name.size() - 1;
		for (idx; folder->name[idx] != '.'; --idx)
		{
		}

		for (int i = idx + 1; i <= idx + 3; ++i)
			ext += folder->name[i];

		if (ext != "txt" || ext != "TXT")
		{
			printf("Open: \n\t %s \nwith another app \n", folder->name.c_str());
			return res;
		}

		//folder->print(false);


		long startOffset = clusterToSector(folder->cluster[0]) * 512;
		long lastOffset = clusterToSector(folder->cluster[folder->cluster.size() - 1] + 1 + 1) * 512;

		//std::string content = "";
		FILE *f;
		f = fopen(diskName.c_str(), "rb");
		fseek(f, startOffset, SEEK_SET);

		BYTE character;
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

CFolder *FAT_32::findFolderByName(CFolder &folder, std::string folderName, bool searchAll)
{

	CFolder *res = folder.findByName(folderName, searchAll);
	if (res == nullptr)
	{
		printf("[ERROR] CANNOT FIND \"%s\"\n", folderName.c_str());
		exit(0);
	}

	return res;

	//printFolderInfo(*res);


}







void FAT_32::print_base_in4()
{
	printf("%C:\\ \n", this->diskName[4]);
	printf("Boot sector: \n");
	this->printBootSector();
	/*printf("RDET: \n");
	this->printRDET();*/

}

bool FAT_32::cd(std::string path)
{
	//split path -> vector
	std::vector <std::string > inPath;
	std::string temp = "";
	for (int i = 0; i < path.size(); ++i)
	{
		temp += path[i];
		if (path[i] == '\\')
		{
			temp.pop_back();
			inPath.push_back(temp);
			temp = "";
		}
	}
	if (temp.size() > 0)
		inPath.push_back(temp);

	if (inPath[0] == ".")
		return true;
	if (inPath[0] == "..")
	{
		if (this->path.size() == 1 || this->path.size() == 0)
			return true;
		this->path.pop_back();
		inPath = this->path;
	}


	if (inPath[0].back() == ':')
		inPath[0] += '\\';

	CFolder *tempPath = this->curPath;
	std::vector<std::string> currentPath = this->path;

	if (inPath[0] == this->root.name)
	{
		currentPath.clear();
		CFolder *tempPath = &root;

		for (int i = 0; i < inPath.size(); ++i)
		{
			tempPath = tempPath->findByName(inPath[i], false);
			if (tempPath == nullptr || !tempPath->isFolder())
			{
				printf("[ERROR]: CANNOT FOUND PATH \"%s\"", path.c_str());
				exit(0);
			}
			currentPath.push_back(inPath[i]);

		}
		if (tempPath == nullptr || !tempPath->isFolder())
		{
			printf("[ERROR]: CANNOT FOUND PATH \"%s\"", path.c_str());
			exit(0);
		}


		curPath = tempPath;
		this->path = currentPath;


	}
	else
	{
		for (int i = 0; i < inPath.size(); ++i)
		{
			tempPath = tempPath->findByName(inPath[i], false);
			if (tempPath == nullptr || !tempPath->isFolder())
			{
				printf("[ERROR]: CANNOT FOUND PATH \"%s\"", path.c_str());
				exit(0);
			}
			currentPath.push_back(inPath[i]);

		}
		if (tempPath == nullptr || !tempPath->isFolder())
		{
			printf("[ERROR]: CANNOT FOUND PATH \"%s\"", path.c_str());
			exit(0);
		}

		curPath = tempPath;
		this->path = currentPath;



	}

	printf("\n");

	return true;
}
std::wstring FAT_32::cwd()
{
	std::string str = csd();
	return std::wstring(str.begin(), str.end());
}

std::string FAT_32::csd()
{
	std::string currentPath = "";
	for (std::string pathName : path)
		currentPath += pathName + "\\";
	currentPath.pop_back();
	return currentPath;
}

void FAT_32::ls()
{
	int index[] = { 3,2,7,6,5 };
	char type[] = { 'd', 'a', 'r','h','s' };
	printf("Mode          Name\n");
	printf("----          ----\n");

	for (auto subFolder : this->curPath->subItem)
	{
		if (subFolder->canPrint())
		{
			char mode[] = "------";
			for (int i = 0; i < 5; ++i)
				if (subFolder->state[index[i]] == '1')
					mode[i] = type[i];
			printf("%s", mode);
			printf("        ");
			printf("%s\n", subFolder->name.c_str());
		}


	}
}
void FAT_32::tree()
{
	printRDET(*curPath);
}

std::vector<BYTE> FAT_32::get_data(const std::string &name)
{
	std::vector<BYTE> res;
	if (curPath->name == name)
		res = printFolderInfo(curPath);

	for (auto subFolder : curPath->subItem)
	{
		if (subFolder->name == name)
		{
			printf("Found!\n");
			res = printFolderInfo(subFolder);
		}

	}


	return res;
}