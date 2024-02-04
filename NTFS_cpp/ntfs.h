#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
// #include <cmath>
#include <map>

using namespace std;
using BYTE = unsigned char;

constexpr int VBR_SIZE = 512;


struct MFT_Header {
    uint64_t info_offset;
    uint64_t info_size;

    uint64_t file_name_offset;
    uint64_t file_name_size;

    uint64_t data_offset;
    uint64_t data_size;

    uint64_t num_sector;

public:
    MFT_Header(vector<BYTE> &data);
};

struct MFT_Entry {
    uint64_t mft_record_number;
    uint8_t flag;

    // Standard Information
    uint64_t created_time;
    uint64_t last_modified_time;
    vector<string> attribute;

    // File Name
    uint64_t parent_mft_record_number;
    wstring file_name;

    // Data
    bool resident;
    uint64_t real_size;
    vector<BYTE> content; // Resident, no name
    // Non-resident, no name
    uint64_t start_cluster;
    uint64_t num_cluster;

    vector<uint64_t> sub_files_number;

public:
    uint64_t standard_i4_start;
    uint64_t standard_i4_size;

    uint64_t file_name_start;
    uint64_t file_name_size;

    uint64_t data_start;
    uint64_t data_size;
public:
    MFT_Entry() {}
    MFT_Entry(vector<BYTE> &data);
    vector<string> convert2attribute(uint32_t flags);
    void extract_standard_i4(vector<BYTE> &data, uint64_t start);

    void extract_file_name(vector<BYTE> &data, uint64_t start);

    void checkdata(vector<BYTE> &data, uint64_t start);
    void extract_data(vector<BYTE> &data, uint64_t start);
};


uint64_t cal(vector<BYTE> &BYTEs, int start, int end);
//? use printf(L"%l", s) to print wstring
wstring fromUnicode(vector<BYTE> &BYTEs);


struct NTFS {
    vector<BYTE> vbr;
    // uint8_t jump[3];
    string oem_id;
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    // uint8_t media_descriptor;
    // uint16_t sectors_per_track;
    // uint16_t number_of_heads;
    // uint64_t hidden_sectors;
    uint64_t total_sectors;
    uint64_t mft_cluster_number;
    uint64_t mft_mirror_cluster_number;
    int32_t mft_record_size;
    uint64_t serial_number;

    // MFT size
    uint64_t mft_offset;
    // vector<MFT_Entry> mft_entries;
    map<uint64_t, MFT_Entry> mft_entries;   

public:
    uint64_t root;
    uint64_t current_node;

public:
    NTFS(string name);

    bool is_NTFS();
    void extract_vbr();

    void child_linker();

    uint64_t find_mft_entry(string name);
    uint64_t get_parent();

    void print_vbr();
    void print_ntfs_in4();
};
