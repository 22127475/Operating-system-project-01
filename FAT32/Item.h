#include <iostream>
#include <vector>
#include <cctype>
#pragma once
//class CItem
//{
//protected:
//	std::string name;
//	std::string state;
//	std::string size;
//	std::vector<int> cluster;
//
//public:
//	virtual void add(CItem*) = 0;
//	virtual void print() = 0;
//	virtual bool isFolder() = 0;
//	virtual void getChild(std::vector<CItem*>) = 0;
//	~CItem() {};
//};
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
	CFolder(std::string name, std::string state, std::string size, std::vector<long> cluster);
	void print(int time);
	bool isFolder();
	void getChild(std::vector<CFolder*>);
	bool canPrint();
	std::string toUpercase(std::string src);
	//CFolder* findByName(std::string);
	CFolder* findByName(std::string);
	//bool isTXT();
	~CFolder();
	
};

//class CFile : public CItem
//{
//public:
//	CFile();
//	CFile(std::string name, std::string state, std::string size);
//	void add(CItem*) override;
//	void print() override;
//	bool isFolder();
//	void getChild(std::vector<CItem*>);
//	~CFile() {};
//};