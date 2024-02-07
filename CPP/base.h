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

    // virtual void read(const string &name) = 0;
};

// Support functions
uint64_t cal(vector<BYTE> &BYTEs, int start, int end);
wstring fromUnicode(vector<BYTE> &BYTEs);

vector<string> splitString(const string &input, string delimeter = "\\/");
bool compareWstrVsStr(const wstring &wstr, const string &str);


// split A/B, A\B => [A] [B]
// split cd A => [cd] [A] [\n]
vector<string> splitString(const string &input, string delimeter) {
    vector<string> tokens;
    // if (input.back() == '\n')
    //     input.pop_back();
    size_t startPos = 0;
    size_t foundPos = input.find_first_of(delimeter);

    while (foundPos != string::npos) {
        tokens.push_back(input.substr(startPos, foundPos - startPos));
        startPos = foundPos + 1;
        foundPos = input.find_first_of(delimeter, startPos);
    }

    tokens.push_back(input.substr(startPos));

    return tokens;
}
bool compareWstrVsStr(const wstring &wstr, const string &str) {
    wstring str2(str.begin(), str.end());
    return wstr == str2;
}

// void read(const string &name) {
//     if (is_directory()) {
//         cd (name);
//         tree();
//         cd ("..");
//         return;
//     }

//     try
//     vector<BYTE> data = get_data(name);
//     catch (const char *msg) {
//         fprintf(stderr, "Error: %s\n", msg);
//     }
//     for (auto &i : data)
//         printf("%c", i);
//     printf(\n);
// }