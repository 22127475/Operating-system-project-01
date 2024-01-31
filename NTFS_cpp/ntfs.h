#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <cmath>
using namespace std;

constexpr int VBR_SIZE = 512;

struct NTFS {
    vector<char> vbr;
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


public:
    NTFS(string name);

    bool is_NTFS();
    void extract_vbr();

    void print_vbr();
};

struct MFT {
    uint32_t info_offset;
    uint32_t info_size;

    uint32_t file_name_offset;
    uint32_t file_name_size;

    uint32_t data_offset;
    uint32_t data_size;

    uint32_t num_sector;

public:
    MFT(vector<char> &data);
};
MFT::MFT(vector<char> &data) {
    info_offset = cal(data, 0x14, 0x16);
    info_size = cal(data, 0x3C, 0x40);

    file_name_offset = info_offset + info_size;
    file_name_size = cal(data, 0x9C, 0xA0);

    data_offset = file_name_offset + file_name_size;
    data_size = cal(data, 0x104, 0x108);

    num_sector = (cal(data, 0x118, 0x120) + 1) * 8;
}


struct MFT_Entry {
    uint32_t mft_record_number;
    uint8_t flag;

    // Standard Information
    uint64_t created_time;
    uint64_t last_modified_time;
    string attribute;

    // File Name
    uint32_t parent_mft_record_number;
    string file_name;

    vector<MFT_Entry> children;
    
public:
    uint32_t standard_i4_start;
    uint32_t standard_i4_size;

    uint32_t file_name_start;
    uint32_t file_name_size;

    uint32_t data_start;
    uint32_t data_size;
public:
    MFT_Entry(vector<char> &data) {
        mft_record_number = cal(data, 0x2C, 0x30);

        flag = data[0x16];

        standard_i4_start = cal(data, 0x14, 0x16);
        standard_i4_size = cal(data, standard_i4_start + 0x4, standard_i4_start + 0x8);
        //todo Some standard information here
        extract_standard_i4(data, standard_i4_start);

        file_name_start = standard_i4_start + standard_i4_size; //? After the STANDARD INFORMATION
        file_name_size = cal(data, file_name_start + 0x4, file_name_start + 0x8);
        //todo FILE NAME information

        data_start = file_name_start + file_name_size; //? After the FILE NAME
        data_size = cal(data, data_start + 0x4, data_start + 0x8);
    }
    string convert2attribute(uint32_t flags) {
        flags = flags & 0x1FFF;
        if (flags == 0x1) return "READ ONLY";
        if (flags == 0x2) return "HIDDEN";
        if (flags == 0x4) return "SYSTEM";
        if (flags == 0x20) return "ARCHIVE";
        if (flags == 0x40) return "DEVICE";
        if (flags == 0x80) return "NORMAL";
        if (flags == 0x100) return "TEMPORARY";
        if (flags == 0x200) return "SPARSE_FILE";
        if (flags == 0x400) return "REPARSE_POINT";
        if (flags == 0x800) return "COMPRESSED";
        if (flags == 0x1000) return "OFFLINE";
        if (flags == 0x2000) return "NOT_CONTENT_INDEXED";
        if (flags == 0x4000) return "ENCRYPTED";
        if (flags == 0x8000) return "DIRECTORY";
        return "";
    }
    void extract_standard_i4(vector<char> &data, uint32_t start) {
        //! Check the first 4 bytes to be 0x10
        uint32_t marker = cal(data, start, start + 4);
        if (marker != 0x10) {
            fprintf(stderr, "Error");
            exit(1);
        }
        uint32_t offset = cal(data, start + 20, start + 21);
        uint32_t beginIS = start + offset;

        created_time = cal(data, beginIS, beginIS);
        last_modified_time = cal(data, beginIS + 0x8, beginIS + 0x10);

        attribute = convert2attribute(cal(data, beginIS + 0x20, beginIS + 0x24));
    }

    string fromUnicode(vector<char> &bytes) {

    }
    void extract_file_name(vector<char> &data, uint32_t start) {
        //! Check the first 4 bytes to be 0x30
        uint32_t marker = cal(data, start, start + 4);
        if (marker != 0x30) {
            fprintf(stderr, "Error");
            exit(1);
        }
        
        uint32_t data_size = cal(data, start + 0x10, start + 0x14);
        uint32_t offset = cal(data, start + 0x14, start + 0x16);

        uint32_t beginFN = start + offset;

        parent_mft_record_number = cal(data, beginFN, beginFN + 0x8);

        uint8_t name_length = data[beginFN + 0x40];
        vector<char> name(name_length * 2);
        for (int i = 0; i < name_length * 2; ++i)
            name[i] = data[beginFN + 0x42 + i];
        file_name = fromUnicode(name);
    }
};

uint32_t cal(vector<char> &bytes, int start, int end);
