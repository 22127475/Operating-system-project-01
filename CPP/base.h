#pragma once
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <map>

using namespace std;
using BYTE = unsigned char;
constexpr int VBR_SIZE = 512;

class Volume {
public:
    virtual void print_base_in4() = 0;

    virtual bool cd(string path) = 0;
    virtual wstring cwd() = 0;
    virtual void ls() = 0;
    virtual void tree() = 0;

    virtual vector<BYTE> get_data(const string &name) = 0;
};

// Support functions
uint64_t cal(vector<BYTE> &BYTEs, int start, int end);
wstring fromUnicode(vector<BYTE> &BYTEs);

vector<string> splitString(const string &input, string delimeter = "\\/");
bool compareWstrVsStr(const wstring &wstr, const string &str);

