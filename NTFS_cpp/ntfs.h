#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <cmath>

using namespace std;
using BYTE = unsigned char;

constexpr int VBR_SIZE = 512;

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
    // uint32_t hidden_sectors;
    uint32_t total_sectors;
    uint32_t mft_cluster_number;
    uint32_t mft_mirror_cluster_number;
    int8_t mft_record_size;
    uint32_t serial_number;

    // MFT size
    uint32_t mft_offset;
    vector<MFT_Entry> mft_entries;

public:
    NTFS(string name);

    bool is_NTFS();
    void extract_vbr();

    void print_vbr();
};

struct MFT_Header {
    uint32_t info_offset;
    uint32_t info_size;

    uint32_t file_name_offset;
    uint32_t file_name_size;

    uint32_t data_offset;
    uint32_t data_size;

    uint32_t num_sector;

public:
    MFT_Header(vector<BYTE> &data);
};

struct MFT_Entry {
    uint32_t mft_record_number;
    uint8_t flag;

    // Standard Information
    uint64_t created_time;
    uint64_t last_modified_time;
    string attribute;

    // File Name
    uint32_t parent_mft_record_number;
    wstring file_name;

    // Data
    bool resident;
    vector<BYTE> data; // Resident, no name
    // Non-resident, no name
    uint32_t start_cluster;
    uint32_t num_cluster;

    vector<MFT_Entry> sub_files;

public:
    uint32_t standard_i4_start;
    uint32_t standard_i4_size;

    uint32_t file_name_start;
    uint32_t file_name_size;

    uint32_t data_start;
    uint32_t data_size;
public:
    MFT_Entry(vector<BYTE> &data);
    string convert2attribute(uint32_t flags);
    void extract_standard_i4(vector<BYTE> &data, uint32_t start);

    void extract_file_name(vector<BYTE> &data, uint32_t start);

    void checkdata(vector<BYTE> &data, uint32_t start);
    void extract_data(vector<BYTE> &data, uint32_t start);
};


uint64_t cal(vector<BYTE> &BYTEs, int start, int end);
//? use printf(L"%l", s) to print wstring
wstring fromUnicode(vector<BYTE> &BYTEs);
