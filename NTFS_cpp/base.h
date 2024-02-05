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

    virtual bool change_dir(string path) = 0;
    virtual wstring get_current_path() = 0;
    virtual void list() = 0;
    // virtual void tree() = 0;
};

