#pragma once
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <map>
#include <windows.h>

#include <locale>
#include <codecvt>

using namespace std;
// using BYTE = unsigned char;
constexpr int VBR_SIZE = 512;

class Volume {
public:
    virtual void print_base_in4() {
        printf("The base information of the volume:\n");
    }

    virtual bool cd(string path) = 0;
    virtual wstring pwd() {
        printf("Path\n----\n");
        return L"";
    }
    virtual void ls() {
        printf(" Mode \t ID \tFile name\n");
        printf("------\t----\t---------\n");
    }
    virtual void tree() = 0;

    // virtual vector<BYTE> get_data(const string &name) = 0;

    virtual void read(const string &name = "") = 0;

    // virtual void info(const string &path = "") = 0;
};



string trim(const string &str, bool end_slash = true) {
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = end_slash ? str.find_last_not_of(" \t\n\r\\/") : str.find_last_not_of(" \t\n\r");


    if (start == string::npos || end == string::npos)
        return ""; // No non-whitespace characters found

    return str.substr(start, end - start + 1);
}
vector<string> splitString(const string &input, string delimeter = "/\\", bool all = true, bool end_slash = true) {
    vector<string> tokens;

    string tmp = trim(input, end_slash);
    if (tmp[0] == '\"' && tmp[tmp.size() - 1] == '\"')
        tmp = tmp.substr(1, tmp.size() - 2);

    size_t startPos = 0;
    size_t foundPos = tmp.find_first_of(delimeter);

    if (all)
        while (foundPos != string::npos) {
            tokens.push_back(tmp.substr(startPos, foundPos - startPos));

            startPos = tmp.find_first_not_of(delimeter, foundPos + 1);
            foundPos = tmp.find_first_of(delimeter, startPos);
        }
    else {
        if (foundPos == string::npos)
            return { tmp };
        tokens.push_back(tmp.substr(startPos, foundPos - startPos));
        startPos = tmp.find_first_not_of(delimeter, foundPos + 1);
        foundPos = tmp.find_first_of(delimeter, startPos);
    }

    tokens.push_back(tmp.substr(startPos));

    return tokens;
}
bool compareWstrVsStr(const wstring &wstr, const string &str) {
    wstring str2(str.begin(), str.end());
    return wstr == str2;
}

string Utf16toUtf8(const wstring &wstr) {
    wstring_convert<codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
    return convert.to_bytes(wstr);
}