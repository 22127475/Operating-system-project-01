#include "All.h"
void App::normalization(std::string& str) {


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
}
unsigned long App::littleEdian(BYTE* arr, unsigned int n)
{
	unsigned long res = 0;
	for (int i = n - 1; i >= 0; i--)
		res = (res << 8) | arr[i];
	return res;
}
unsigned long App::littleEdian(std::vector<BYTE> arr)
{
	unsigned int res = 0;
	for (int i = arr.size() - 1; i >= 0; i--)
		res = (res << 8) | arr[i];

	return res;
}
std::string App::hexToBin(BYTE hex)
{
	std::string res;
	std::string hexTable[16] = { "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111" };

	res += hexTable[hex / 16];
	res += hexTable[hex % 16];

	return res;

}
void App::scanDisk(std::string& diskName)
{
	
	if (diskName.size() > 9)
	{
		std::cerr << "[ERROR]: INVALID DISK NAME SIZE (" << diskName.size() << " instead of 1) \n";
		exit(0);
	}

	FILE* f;
	f = fopen(diskName.c_str(), "rb");

	if (f == nullptr)
	{
		fclose(f);
		std::cerr << "[ERROR]: CANNOT OPEN " << diskName << "\n";
		exit(0);
	}

	fclose(f);

	if (diskName[4] >= 'a' && diskName[4] <= 'z')
		diskName[4] = std::toupper(diskName[4]);

	this->diskName = diskName;
	this->readBootSector();

	std::string oemID = "";
	for (int i = 0; i < 8; ++i)
	{
		oemID += this->bootSector.oemID[i];
	}
	if (oemID == "NTFS")
	{
		std::cerr << "[ERROR]: INVALID FILE SYSTEM: " << oemID << "\n";
		exit(0);
	}
	
	std::cout << "SUCCESS OPEN " << diskName << "\n";

	this->readFatTable();
	this->makeRDET();
	

	
}
void App::readBootSector()
{
	FILE* f;
	f = fopen(diskName.c_str(), "rb");

	if (f == nullptr)
	{
		fclose(f);
		std::cerr << "[ERROR]: CANNOT OPEN " << diskName << "\n";
		exit(0);
	}
	fseek(f, 0, SEEK_SET);
	fread(&this->bootSector, sizeof(bootSector), 1, f);
	fseek(f, 0, SEEK_SET);
	fclose(f);
}
void App::printBootSector()
{
	printf("Jump code: 0x%02X%02X%02 \n", bootSector.jumpCode[2], bootSector.jumpCode[1], bootSector.jumpCode[0]);
	printf("OEM ID: %s \n", bootSector.oemID);
	printf("Byte per sector: %d \n", littleEdian(bootSector.bytePerSector, 2));
	printf("Sector per cluster (Sc): %d \n", int(bootSector.sectorPerCluster));
	printf("Reserved sector (Sb): %d \n", littleEdian(bootSector.sectorOfBootsector, 2));
	printf("Number of FAT table (Nf): %d \n", int(bootSector.copyOfFAT));
	printf("Volume size (Sv): %d \n", littleEdian(bootSector.volumeSize, 4));
	printf("FAT size (Sf): %d \n", littleEdian(bootSector.FATSize, 4));
	std::cout << "First sector of FAT table: " << littleEdian(bootSector.sectorOfBootsector, 2) << '\n';
	printf("First sector of RDET: %d \n", littleEdian(bootSector.sectorOfBootsector, 2) + littleEdian(bootSector.FATSize, 4) * (unsigned int)bootSector.copyOfFAT);
	printf("First cluster of RDET: %d \n", littleEdian(bootSector.clusterStartOfRDET, 4));
	printf("First sector of Data: %d \n", littleEdian(bootSector.sectorOfBootsector, 2) + littleEdian(bootSector.FATSize, 4) * (unsigned int)bootSector.copyOfFAT);
	printf("Data size: %d \n", littleEdian(bootSector.volumeSize, 4) - littleEdian(bootSector.sectorOfBootsector, 2) - littleEdian(bootSector.FATSize, 4) * (unsigned int)bootSector.copyOfFAT);
	printf("FAT type: ");
	for (int i = 0; i < 8; ++i)
		printf("%C", bootSector.FATType[i]);
	printf("\n");
	printf("End sign: 0x%02X%02X \n", bootSector.endSign[1], bootSector.endSign[0]);
}
long App::clusterToSector(int cluster)
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
std::vector<long> App::clusterLinkListFrom(long startCluster)
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
void App::readFatTable()
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
void App::printFatTable()
{
	long clusterNo = 0;
	for (int i = 0; i < fatMap.size(); ++i)
	{
		clusterNo = littleEdian({ fatMap[i][0], fatMap[i][1], fatMap[i][2], fatMap[i][3] });
		if (clusterNo == 268435455)
			std::cout << i << ": EOF \n";
		else
			std::cout << i << ": " << clusterNo << "\n";
	}
}
std::string App::readVFAT(FILE* f)
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
std::vector<int> App::numberOfFile(long offset)
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
std::vector<CFolder*> App::readRDET(long offset)
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
			name += ".";
			for (int i = 0; i < 3; ++i)
			{
				name += endtry.ext[i];
			}
		}
		else
		{
			for (int i = 0; i < lfn.size(); ++i)
				name += lfn[lfn.size() - 1 - i];

			for (int i = 0; i < name.size(); ++i)
				if (name[i] == '�')
					name[i] = ' ';

		}


		normalization(name);

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




	for (int i = 2; i < res.size(); ++i)
	{
		if (res[i]->isFolder())
		{

			for (long cluster : res[i]->cluster)
			{
				long subOffset = clusterToSector(cluster) * 512;
				std::vector<CFolder*> subFolder = readRDET(subOffset);
				res[i]->getChild(subFolder);
			}
		}

	}


	return res;
}
void App::readRDET(long offset, CFolder& folder)
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
			normalization(name);

			name += ".";
			for (int i = 0; i < 3; ++i)
			{
				name += endtry.ext[i];
			}
			normalization(name);

		}
		else
		{
			for (int i = 0; i < lfn.size(); ++i)
				name += lfn[lfn.size() - 1 - i];
			normalization(name);


			for (int i = 0; i < name.size(); ++i)
				if (name[i] == '�')
					name[i] = ' ';
			normalization(name);

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
void App::makeRDET()
{

	long startCluster = littleEdian(bootSector.sectorOfBootsector, 2) + littleEdian(bootSector.FATSize, 4) * (unsigned int)bootSector.copyOfFAT;


	this->root.name = "Root";
	this->root.state = "00010000";
	this->root.size = "0";
	this->root.cluster = { 0,1,2 };

	readRDET(startCluster * 512, this->root);

}
void App::printRDET(CFolder& folder, int time)
{
	if (folder.canPrint())
	{
		folder.print(time);
		for (int i = 0; i < folder.subItem.size(); ++i)
		{
			printRDET(*folder.subItem[i], time + 1);
		}
	}
}
void App::printRDET()
{
	root.print(0);
}
void App::findFolderByName()
{
	std::string folderName = "";
	std::cout << "Input name: ";
	getline(std::cin, folderName);

	CFolder* res = this->root.findByName(folderName);
	if (res == nullptr)
	{
		std::cerr << "[ERROR] CANNOT FIND \"" << folderName << "\"\n";
		exit(0);
	}
	res->print(0);

}
void App::printFolderInfo(CFolder& folder)
{
	if (folder.isFolder())
	{
		printRDET(folder, 0);
	}
	else
	{
		std::string ext = "";
		int idx = folder.name.size() - 1;
		for (idx; folder.name[idx] != '.'; --idx)
		{
		}

		for (int i = idx + 1; i <= idx + 3; ++i)
			ext += folder.name[i];

		if (ext != "txt")
		{
			std::cout << "Open: \n\t" << folder.name << "\nwith another app \n";
			return;
		}

		folder.print(0);

		long startOffset = clusterToSector(folder.cluster[0]) * 512;
		long lastOffset = clusterToSector(folder.cluster[folder.cluster.size() - 1] + 1 + 1) * 512;

		std::cout << "startOffset: " << startOffset << "\n";
		std::cout << "lastOffset: " << lastOffset << "\n";


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

		std::cout << "Content: \n";
		std::cout << content << '\n';

	}
}
void App::mainloop()
{
	std::cout << "//////////////////////////////////////////\n";
	std::cout << "//                                      //\n";
	std::cout << "// Project 1 - Quan ly he thong tap tin //\n";
	std::cout << "//                                      //\n";
	std::cout << "//////////////////////////////////////////\n";
	std::cout << "------------------------------------------\n";
	std::cout << "Made by: " << "\n";
	std::cout << "|--------------------------------------------|\n";
	std::cout << "|  MSSV                                 Name |\n";
	std::cout << "|--------------------------------------------|\n";
	std::cout << "| 22127026\t" << "                  On Gia Bao |" << "\n";
	std::cout << "| 22127275\t" << "               Tran Anh Minh |" << "\n";
	std::cout << "| 22127280\t" << "        Doan Dang Phuong Nam |" << "\n";
	std::cout << "| 22127465\t" << "           Bui Nguyen Lan Vy |" << "\n";
	std::cout << "| 22127475\t" << "                Diep Gia Huy |" << "\n";
	std::cout << "|--------------------------------------------|\n";

	
	std::string disk = "\\\\.\\";
	std::string volume = "";
	std::cout << "Input disk name (C,D,E,F...): ";
	getline(std::cin, volume);
	volume += ":";
	disk += volume;
	this->scanDisk(disk);

	bool loop = true;
	while (loop)
	{
		std::cout << this->diskName[4] << this->diskName[5] << "\\>";
		std::cin >> loop;
	}

}