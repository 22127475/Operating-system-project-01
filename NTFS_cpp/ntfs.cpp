#include "ntfs.h"

NTFS::NTFS(string name) {
    name = "\\\\.\\" + name + ":";
    FILE *volume = fopen(name.c_str(), "rb");
    if (!volume) {
        fprintf(stderr, "Error: Permission denied\n");
        exit(1);
    }
    vbr.resize(VBR_SIZE);
    size_t bytesRead = fread(vbr.data(), 1, VBR_SIZE, volume);
    if (bytesRead != VBR_SIZE) {
        fprintf(stderr, "Error: unable to read VBR\n");
        exit(1);
    }
    extract_vbr();

    if (!is_NTFS()) {
        fprintf(stderr, "Error: Not NTFS\n");
        exit(1);
    }
    
    //? Read MFT header
    vector<BYTE> mft_header(mft_record_size);
    fseek(volume, mft_offset, SEEK_SET);
    bytesRead = fread(mft_header.data(), 1, mft_record_size, volume);

    //? Read the mft entry
    MFT_Header mft_header_info(mft_header);
    mft_entries.resize(0);
    for (int i = 2; i < mft_header_info.num_sector; i += 2) {
        vector<BYTE> mft_entry(mft_record_size);
        bytesRead = fread(mft_entry.data(), 1, mft_record_size, volume);

        string mark = "    ";
        for (int i = 0; i < 4; ++i)
            mark[i] = mft_entry[i];
        if (mark != "FILE") continue; // BAAD

        MFT_Entry mft_entry_info(mft_entry);
        mft_entries.push_back(mft_entry_info);
    }

    //! Build the directory tree from the list of MFT entries

    fclose(volume);
}

bool NTFS::is_NTFS() {
    if (oem_id == "NTFS    ")
        return true;
    return false;
}

void NTFS::extract_vbr() {
    oem_id.resize(8);
    for (int i = 3; i < 0xB; i++)
        oem_id[i - 3] = vbr[i];

    bytes_per_sector = cal(vbr, 0xB, 0xD);
    sectors_per_cluster = vbr[0xD];
    reserved_sectors = cal(vbr, 0xE, 0x10);
    total_sectors = cal(vbr, 0x28, 0x30);
    mft_cluster_number = cal(vbr, 0x30, 0x38);
    mft_mirror_cluster_number = cal(vbr, 0x38, 0x40);
    mft_record_size = pow(2, abs(vbr[0x40]));

    serial_number = cal(vbr, 0x48, 0x50);

    mft_offset = mft_cluster_number * sectors_per_cluster * bytes_per_sector;
}

void NTFS::print_vbr() {
    for (int i = 0; i < VBR_SIZE; ++i) {
        printf("%02x ", (uint8_t)vbr[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
}

MFT_Header::MFT_Header(vector<BYTE> &data) {
    info_offset = cal(data, 0x14, 0x16);
    info_size = cal(data, 0x3C, 0x40);

    file_name_offset = info_offset + info_size;
    file_name_size = cal(data, 0x9C, 0xA0);

    data_offset = file_name_offset + file_name_size;
    data_size = cal(data, 0x104, 0x108);

    num_sector = (cal(data, 0x118, 0x120) + 1) * 8;
}

MFT_Entry::MFT_Entry(vector<BYTE> &data) {
    mft_record_number = cal(data, 0x2C, 0x30);

    flag = data[0x16];

    standard_i4_start = cal(data, 0x14, 0x16);
    standard_i4_size = cal(data, standard_i4_start + 0x4, standard_i4_start + 0x8);
    //todo Some standard information here
    extract_standard_i4(data, standard_i4_start);

    file_name_start = standard_i4_start + standard_i4_size; //? After the STANDARD INFORMATION
    file_name_size = cal(data, file_name_start + 0x4, file_name_start + 0x8);
    //todo FILE NAME information
    extract_file_name(data, file_name_start);

    data_start = file_name_start + file_name_size; //? After the FILE NAME
    data_size = cal(data, data_start + 0x4, data_start + 0x8);
    //todo DATA information
    checkdata(data, data_start); // + extract data

    sub_files.resize(0); // Initialize the child list
}

string MFT_Entry::convert2attribute(uint32_t flags) {
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
void MFT_Entry::extract_standard_i4(vector<BYTE> &data, uint32_t start) {
    //? Check the first 4 BYTEs to be 0x10
    uint32_t type_id = cal(data, start, start + 4);
    if (type_id != 0x10) {
        fprintf(stderr, "Error attribute information\n");
        exit(1);
    }
    uint32_t offset = cal(data, start + 20, start + 21);
    uint32_t beginIS = start + offset;

    created_time = cal(data, beginIS, beginIS);
    last_modified_time = cal(data, beginIS + 0x8, beginIS + 0x10);

    attribute = convert2attribute(cal(data, beginIS + 0x20, beginIS + 0x24));
}

void MFT_Entry::extract_file_name(vector<BYTE> &data, uint32_t start) {
    //? Check the first 4 BYTEs to be 0x30
    uint32_t type_id = cal(data, start, start + 4);
    if (type_id != 0x30) {
        fprintf(stderr, "Error file name\n");
        exit(1);
    }

    uint32_t data_size = cal(data, start + 0x10, start + 0x14);
    uint32_t offset = cal(data, start + 0x14, start + 0x16);

    uint32_t beginFN = start + offset;

    parent_mft_record_number = cal(data, beginFN, beginFN + 0x8);

    uint8_t name_length = data[beginFN + 0x40];
    vector<BYTE> name(name_length * 2);
    for (int i = 0; i < name_length * 2; ++i)
        name[i] = data[beginFN + 0x42 + i];
    file_name = fromUnicode(name);
}


void MFT_Entry::checkdata(vector<BYTE> &data, uint32_t start) {
    //? Check for the Object ID (0x40)
    uint32_t type_id = cal(data, start, start + 0x4);
    if (type_id == 0x40) {
        uint32_t step = cal(data, start + 0x4, start + 0x8);
        start += step;
    }
    type_id = cal(data, start, start + 0x4);
    if (type_id == 0x80) //? Data attribute
        extract_data(data, start);
    else if (type_id == 0x90) { //? No data attribute => A directory
        resident = true;
        num_cluster = start_cluster = 0;
        attribute = "DIRECTORY";
    }
}
void MFT_Entry::extract_data(vector<BYTE> &data, uint32_t start) {
    resident = (data[start + 0x8] == 0x00);
    if (resident) {
        uint32_t data_size = cal(data, start + 0x10, start + 0x14);
        uint32_t offset = cal(data, start + 0x14, start + 0x16);
        uint32_t beginFN = start + offset;

        data.resize(data_size);
        for (int i = 0; i < data_size; ++i)
            data[i] = data[beginFN + i];
        
        num_cluster = start_cluster = 0; // Resident
    }
    else { // Non-resident, no name
        BYTE datarun_header = data[start + 0x40];
        BYTE length = datarun_header & 0xF; // 4 low bits
        BYTE offset = datarun_header >> 4; // 4 high bits

        num_cluster = cal(data, start + 0x41, start + 0x41 + length);
        start_cluster = cal(data, start + 0x41 + length, start + 0x41 + length + offset);
    }
}

// Calculate Little endian
uint64_t cal(vector<BYTE> &bytes, int start, int end) {
    uint32_t sum = 0;
    int offset = (end - start) * 8;
    for (int i = start; i < end; i++) {
        sum += (uint8_t)bytes[i] << offset;
        offset -= 8;
    }
    return sum;
}
wstring fromUnicode(vector<BYTE> &BYTEs) {
    // Cast BYTEs to wchar_t* (UTF-16)
    const wchar_t *wcharData = reinterpret_cast<const wchar_t *>(BYTEs.data());

    // Construct wstring from wchar_t* data
    return wstring(wcharData, BYTEs.size() / sizeof(wchar_t));
}
