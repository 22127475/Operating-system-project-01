#include "FAT32.h"
// Support functions
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
bool CFolder::isHidden()
{
	return state[6] == '1';
}
bool CFolder::isSystem()
{
	return state[5] == '1' || state[4] == '1';
}
bool CFolder::canPrint(bool printHidden, bool printSystem)
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

	/*bool res = true;
	for (int i = 0; i < this->name.size(); ++i)
	{
		if (name[i] >= 'A' && name[i] <= 'Z' || name[i] >= 'a' && name[i] <= 'z')
		{
			res = true;
		}

	}

	if(!printHidden && this->isHidden())
		res = res && false;
	if(!printSystem && this->isSystem())
		res = res && false;
	
	return res;*/
}
void CFolder::print(bool isFull)
{

	
		if (isFull)
		{
			printf("Name: ");
		}
		printf("%s\n", this->name.c_str());
		if (isFull)
		{
			printf("State: %s\n", binToState().c_str());
			printf("Size: %s B\n", this->size.c_str());
			//printf("Cluster: %d | %d", cluster[0], cluster.size());
			printf("-------------------------------\n");
        	printf("|         Start |   Number of    \n");
        	printf("|       Cluster |   Clusters     \n");
        	printf("|   ");
        	printf("% 11u | ", cluster[0]);
            printf("% 6u\n", cluster.size());
        
			//printf("\n");
		}
	

}
CFolder* CFolder::findByID(const int& id)
{
	/*if (this->index == id)
		return this;
	return nullptr;*/
	if (this->index == id)
		return this;
	for (int i = 0; i < this->subItem.size(); ++i)
		if (this->subItem[i]->index == id)
			return this->subItem[i];
	return nullptr;
}


CFolder *CFolder::findByName(std::string fileName, bool searchAll)
{
	if (this->name == fileName)
		return this;
	for (int i = 0; i < this->subItem.size(); ++i)
		if (this->subItem[i]->name == fileName)
			return this->subItem[i];
	return nullptr;
}
std::string CFolder::binToState()
{
	std::string stateList[] = { "","","Archive","Directory","VolLabel","System","Hidden","ReadOnly" };
	std::string res = "";
	for (int i = 0; i < 8; ++i)
		if (state[i] == '1')
			res += stateList[i] + ", ";
	res.pop_back();
	res.pop_back();

	return res;
}
bool CFolder::isFolder()
{
	return state[3] == '1';
}
void CFolder::getChild(std::vector<CFolder * > child)
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
FAT_32::FAT_32() {}
FAT_32::FAT_32(std::string volume)
{
	if (volume.size() > 1)
	{
		exit(1);
	}

	diskName = "\\\\.\\" + toUpercase(volume) + ":";

	FILE *f;
	f = fopen(diskName.c_str(), "rb");

	if (f == nullptr)
	{
		fclose(f);
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
		//printf("Error can not open %s", diskName.c_str());
        fprintf(stderr, "Error can not open %s", diskName.c_str());

		exit(0);
	}
	fseek(f, 0, SEEK_SET);
	fread(&this->bootSector, sizeof(bootSector), 1, f);
	fseek(f, 0, SEEK_SET);
	fclose(f);
}
void FAT_32::printBootSector()
{
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
	fseek(f, fatFirstCLuster * littleEdian(bootSector.bytePerSector, 2), SEEK_SET);

	BYTE cur[4] = { 1,1,1,1 };
	BYTE next[4] = { 1,1,1,1 };

	std::vector<std::vector<BYTE>> fatTable;
	int condition  = cur[0] + cur[1] + cur[2] + cur[3] + next[0] + next[1] + next[2] + next[3];
 	while (condition != 0)
	{
		fread(&cur, sizeof(cur), 1, f);
		fread(&next, sizeof(next), 1, f);

		fatTable.push_back({ cur[0], cur[1], cur[2], cur[3] });
		fatTable.push_back({ next[0], next[1], next[2], next[3] });
		condition  = cur[0] + cur[1] + cur[2] + cur[3] + next[0] + next[1] + next[2] + next[3];
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
	this->root.index = 0;

	int idx = 0;
	readRDET(startSector * littleEdian(bootSector.bytePerSector, 2), this->root, idx);
	

}
void FAT_32::readRDET(long offset, CFolder &folder, int& idx)
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
		if (endtry.name[0] == 0xE5)
		{
			lfn.clear();
			hasLFN = false;
			name = "";
		}
		else
		{
			if (!hasLFN)
		{

			for (int i = 0; i < 8; ++i)
			{
				name += endtry.name[i];
			}
			name = normalization(name);

			//name += ".";
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

	}
		
		std::string state = hexToBin(endtry.OB);
		
		long startCluster = littleEdian({ endtry.clusterLo[0], endtry.clusterLo[1], endtry.clusterHi[0], endtry.clusterHi[1] });

		std::vector<long> clusterLinkedList = clusterLinkListFrom(startCluster);
		
		std::string size = std::to_string(littleEdian(endtry.size, 4));

		/*CFolder* newFolder = new CFolder(name, state, size, clusterLinkedList, idx);

		if (!newFolder->isFolder() && !hasLFN && name.size() > 3)
			name.insert(name.end() - 3, '.');
		newFolder->name = name;
		
		
		if (newFolder->canPrint(true, true))
		{
			delete newFolder;
			newFolder = new CFolder(name, state, size, clusterLinkedList, ++idx);
		}
		if (newFolder->name.size())
		res.push_back(newFolder);*/

		CFolder* newFolder = new CFolder(name, state, size, clusterLinkedList, ++idx);

		if (!newFolder->isFolder() && !hasLFN && name.size() > 3 )
			name.insert(name.end() - 3, '.');
		newFolder->name = name;
		
		/*
		if (newFolder->canPrint(true, true))
		{
			delete newFolder;
			newFolder = new CFolder(name, state, size, clusterLinkedList, ++idx);
		}*/
		if (newFolder->name.size())
		res.push_back(newFolder);


		lfn.clear();
		hasLFN = false;
		name = "";
	}
		
	

	fclose(f);
	folder.getChild(res);

	for (int i = 0; i < folder.subItem.size(); ++i)
	{
		if (folder.subItem[i]->isFolder() && folder.subItem[i]->canPrint(true, true))
		{
			
			for (int j = 0; j < folder.subItem[i]->cluster.size(); ++j)
			{
				long subOffset = clusterToSector(folder.subItem[i]->cluster[j]) * littleEdian(bootSector.bytePerSector, 2);
				readRDET(subOffset, *folder.subItem[i], idx);
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
void FAT_32::printRDET(CFolder &folder,bool printHidden, bool printSystem, std::string time, bool last)
{
	//folder.print(false);
	
	if (folder.name.size() != 0)
	{
		printf("%s\n",folder.name.c_str());

		int lst = folder.subItem.size() - 1;
		//while (lst >= 0 && !folder.subItem[lst]->canPrint())
		while (lst >= 0 && ((!printHidden && folder.isHidden()) 
	                    || (!printSystem && folder.isSystem())))
	    
			lst--;
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
	

}
void FAT_32::printRDET()
{
	printRDET(root);

}
std::vector<BYTE> FAT_32::printFolderInfo(CFolder *folder)
{
	printf("--------------Info-------------\n");
	folder->print();
	long startSector = clusterToSector(folder->cluster[0]);
	long lastSector = startSector + int(bootSector.sectorPerCluster) * folder->cluster.size() - 1;

	//printf("Sector: %d | %d\n",startSector, folder->cluster.size() * int(bootSector.sectorPerCluster));
	printf("-------------------------------\n");
        	printf("|         Start |   Number of    \n");
        	printf("|        Sector |   Sectors      \n");
        	printf("|   ");
        	printf("% 11u | ", startSector);
            printf("% 6u\n", folder->cluster.size() * int(bootSector.sectorPerCluster));
	printf("------------CONTENT------------\n");

	std::vector<BYTE> res;
	if (folder->isFolder())
	{
		tree();
	}
	else
	{
		std::string ext = "";
		int idx = folder->name.size();
		for (idx; folder->name[idx] != '.'; --idx)
		{
		}

		for (int i = idx + 1; i <= idx + 3; ++i)
			ext += folder->name[i];

		

		if (ext != "txt" && ext != "TXT")
		{
			printf("Please use the appropriate reader to read this file.");
			return res;
		}

		
		long startOffset = startSector * littleEdian(bootSector.bytePerSector, 2);
		long lastOffset = (lastSector + 1) * littleEdian(bootSector.bytePerSector, 2);
		FILE *f;
		f = fopen(diskName.c_str(), "rb");
		fseek(f, startOffset, SEEK_SET);

		BYTE character;
		for (startOffset; startOffset <= lastOffset; ++startOffset)
		{
			fread(&character, sizeof(BYTE), 1, f);
			if (character == 0)
				break;
			//if (character != 0)
				res.push_back(character);
		}
		
		fclose(f);
	}

	return res;
}
/*CFolder *FAT_32::findFolderByName(CFolder &folder, std::string folderName, bool searchAll)
{

	CFolder *res = folder.findByName(folderName, searchAll);
	return res;
}*/
void FAT_32::print_base_in4()
{
	Volume::print_base_in4();
	printf("%C:\\ \n", this->diskName[4]);
	this->printBootSector();
}


// Volume
bool FAT_32::hasIndex(std::string& command)
{
	unsigned int index = 0;
	std::string temp = "";
	while(command[index] != ' ')
	{
		temp += command[index++];
	}
	if (temp == "--index" || temp == "-i")
	{
		temp = "";
		for (int i = index + 1; i < command.size(); ++i)
		{
			temp += command[i];
		}
		if (isNumber(temp))
		{
			index = std::stoi(temp);
			CFolder* tempPath = curPath;
			if (tempPath->index != index)
			{
				for (CFolder* subFolder : tempPath->subItem)
				{
					if (subFolder->index == index)
					{
						command = subFolder->name;
						break;
					}
				}
			}
		}
		else
		{
			//printf("Error: Invalid index");
        	fprintf(stderr, "Error: Invalid index");

			return false;

		}
		
	}
	else
		return false;
	return true;
}
bool FAT_32::cd(std::string path)
{
	std::vector <std::string > inPath = splitString(path);

	if (inPath.size() == 1)
	{
		hasIndex(inPath[0]);
	}
	
if (inPath[0] == ".")
		return true;
	if (inPath[0] == "..")
	{
		if (this->path.size() == 0 ||  this->path.size() == 1)
			return true;
		unsigned int back = 0;
		for (auto dots : inPath)
			if (dots == "..")
				back++;


		if (this->path.size() > back)
		{
			for (int i = 0; i < back; ++i)
			{
				this->path.pop_back();
			}

		}
		else
		{
			
			std::string first = this->path[0];
			this->path.clear();
			this->path.push_back(first);
		}
		inPath = this->path;
	}

	CFolder* temp = curPath;
	std::vector<std::string> temPath = this->path;
	std::string disk = this->path[0];
	

	if (inPath[0] != disk)
	{
		for (auto pathName : inPath)
		{
			temp = temp->findByName(pathName, false);
			if (temp == nullptr || !temp->isFolder())
			{
				printf("\rError: No such directory found");

				return false;
			}
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
			temp = temp->findByName(inPath[i], false);
			if (temp == nullptr || !temp->isFolder())
			{
				printf("\rError: No such directory found");

				return false;
			}
			temPath.push_back(inPath[i]);

		}

	}

	this->path = temPath;
	this->curPath = temp;


	return true;



}
std::wstring FAT_32::pwd()
{
	std::string str = csd();
	return std::wstring(str.begin(), str.end());
}
std::string FAT_32::csd()
{
	std::string currentPath = "";
	for (std::string pathName : path)
		currentPath += pathName + "\\";
	if (path.size() != 1)
		currentPath.pop_back();

	return currentPath;
}
void FAT_32::ls(bool printHidden, bool printSystem)
{
	int index[] = { 3,2,7,6,5};
	char type[] = { 'd', 'a', 'r','h','s'};
	
	Volume::ls();
	for (auto subFolder : this->curPath->subItem)
	{
		if (!printHidden &&  subFolder->isHidden())
			continue;
		if (!printSystem &&  subFolder->isSystem())
			continue;
		if (subFolder->name.size())
		{
			char mode[] = "------";
			for (int i = 0; i < 5; ++i)
				if (subFolder->state[index[i]] == '1')
					mode[i] = type[i];
			if (subFolder->state[4] == '1')
				mode[4] = 's';
			printf("%s\t", mode);
			printf("%04u\t", subFolder->index);
			printf("%s\n", subFolder->name.c_str());
		}
			
		
	}
}
void FAT_32::tree(bool printHidden, bool printSystem)
{
	printRDET(*curPath,printHidden, printSystem);
}



void FAT_32::read(const std::string& name)
{
	std::string tempName = name;
	if (tempName.size() == 0)
	{
		tempName = curPath->name;
	}
	if (tempName.front() == '\"' && tempName.back() == '\"')
	{
		tempName.erase(0, 1);
		tempName.pop_back();
	}
	std::vector<BYTE> res;
	

	hasIndex(tempName);
	

	if (curPath->name == tempName)
	{
		res = printFolderInfo(curPath);
	}
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
			res = printFolderInfo(subFolder);
			if (subFolder->name != "." && subFolder->name != ".." && subFolder->isFolder())
				cd("..");
		}
	}

	for (char content : res)
		printf("%c", content);
	
}