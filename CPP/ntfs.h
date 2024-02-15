#pragma once
#include "base.h"

class MFT_Header {
private:
    uint64_t info_offset;
    uint64_t info_size;

    uint64_t file_name_offset;
    uint64_t file_name_size;

    uint64_t data_offset;
    uint64_t data_size;

public:
    uint64_t num_sector;

public:
    MFT_Header(vector<BYTE> &data);
};

class MFT_Entry {
public:
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
    vector<uint64_t> start_cluster;
    vector<uint64_t> num_cluster;
    // uint64_t start_cluster;
    // uint64_t num_cluster;

    // Sub-files
    vector<uint64_t> sub_files_number;

private:
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

    bool is_directory();
    bool is_archive();
    bool is_hidden_system();
};

class NTFS : public Volume {
public:
    string disk_name;
    FILE *volume;

    vector<BYTE> vbr;
    string oem_id;
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint64_t total_sectors;
    uint64_t mft_cluster_number;
    uint64_t mft_mirror_cluster_number;
    int32_t mft_record_size;
    uint64_t serial_number;

    // MFT start position
    uint64_t mft_offset;

    // List of all MFT entries
    map<uint64_t, MFT_Entry> mft_entries;

public:
    uint64_t root;
    vector<uint64_t> current_node;

public:
    NTFS(string name);
    ~NTFS();

    bool is_NTFS();
    void extract_vbr();
    void child_linker();

    uint64_t find_mft_entry(const string &record_name);
    vector<BYTE> get_data(const string &name);

    bool change_dir(string path);
    wstring get_current_path();
    void list(bool print_hidden = false);
    void print_tree(uint64_t entry = 0, string prefix = "", bool last = false);

    void print_vbr();
    void print_base_in4();

public: //? polymorphism
    bool cd(string path) {
        return change_dir(path);
    }
    wstring cwd() {
        return get_current_path();
    }
    void ls() {
        list();
    }
    void tree() {
        print_tree();
    }
    void read(const string &name);
    void info(const string &path = "");
};

// Support functions
uint64_t cal(vector<BYTE> &BYTEs, int start, int end);
wstring fromUnicode(vector<BYTE> &BYTEs);