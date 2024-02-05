#include <iostream>
#include <vector>
#include <cctype>
#include "All.h"
CFolder::CFolder()
{
	name = "";
	state = "";
	size = "";
	cluster = {};
}
CFolder::CFolder(std::string name,  std::string state, std::string size, std::vector<long> cluster)
{
	this->name = name;
	this->state = state;
	this->size = size;
	this->cluster = cluster;
}

std::string CFolder::toUpercase(std::string src)
{
	std::string result = src;

	for (char& c : result) {
		c = std::toupper(c);
	}

	return result;
}

bool CFolder::canPrint()
{
	bool res = false;
	for (int i = 0; i < this->name.size(); ++i)
	{
		if (name[i] >= 'A' && name[i] <= 'Z' || name[i] >= 'a' && name[i] <= 'z')
			res = true;

	}
	
	if (this->state[6] != 1)
		res = res && true;

	
	
	

	return res;
}

void CFolder::print(int time)
{
	
	if (this->canPrint())
	{
		int idx = 1 << time;
		
		for (int i = 0; i < idx; ++i)
			std::cout << "-";

		std::cout << "Name: " << this->name << "\n";
		for (int i = 0; i < idx; ++i)
			std::cout << "-";

		std::cout << "State: " << this->state << "\n";
		//if (!this->isFolder())
		for (int i = 0; i < idx; ++i)
			std::cout << "-";

		std::cout << "Size: " << this->size << "\n";
		for (int i = 0; i < idx; ++i)
			std::cout << "-";

		std::cout << "Cluster: ";
		for (int i = 0; i < cluster.size() - 1; ++i)
		{
			std::cout << cluster[i] << ",";
		} 
		std::cout << cluster[cluster.size() - 1];
		std::cout << "\n";
		for (int i = 0; i < this->subItem.size(); ++i)
			this->subItem[i]->print(time + 1);
	}
	
}

CFolder* CFolder::findByName(std::string fileName)
{
	if (toUpercase( this->name) == toUpercase( fileName))
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
	std::string folder[] = {"00010000", "00010010"};
	for (int i = 0; i < 2; ++i)
	{
		if (this->state == folder[i])
			return true;
	}

	return false;
	
}

void CFolder::getChild(std::vector<CFolder* > child)
{
	this->subItem = child;
}


CFolder::~CFolder()
{
	
}
