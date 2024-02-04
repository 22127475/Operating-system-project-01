#include "ntfs.h"

NTFS::NTFS(string name) {
    name = "\\\\.\\" + name + ":";
    FILE *volume = fopen(name.c_str(), "rb");
    if (!volume) {
        fprintf(stderr, "Error: Permission denied\n");
        exit(1);
    }
    vbr.resize(512);
    size_t bytesRead = fread(vbr.data(), 1, 512, volume);
    if (bytesRead != 512) {
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
    fseeko64(volume, mft_offset, 0);

    bytesRead = fread(mft_header.data(), 1, mft_record_size, volume);

    //? Read the mft entry
    MFT_Header mft_header_info(mft_header);

    vector<BYTE> mft_entry(mft_record_size);

    for (uint64_t i = 2; i < mft_header_info.num_sector; i += 2) {
        bytesRead = fread(mft_entry.data(), 1, mft_record_size, volume);

        string mark = "    ";
        for (int j = 0; j < 4; ++j)
            mark[j] = mft_entry[j];
        if (mark != "FILE") continue;

        try {
            MFT_Entry mft_entry_info(mft_entry);
            mft_entries[mft_entry_info.mft_record_number] = mft_entry_info;
        }
        catch (const char *msg) {
            continue;
        }
    }

    //! Build the directory tree from the list of MFT entries
    child_linker();

    fclose(volume);
}

bool NTFS::is_NTFS() {
    if (oem_id == "NTFS    ")
        return true;
    return false;
}

uint64_t pow2(uint64_t x) {
    return 1 << x;
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

    // Signed
    uint64_t recordsize = vbr[0x40];
    if (recordsize > 127) recordsize = 256 - recordsize;
    mft_record_size = pow2(recordsize);


    serial_number = cal(vbr, 0x48, 0x50);

    mft_offset = mft_cluster_number * sectors_per_cluster * bytes_per_sector;
}

void NTFS::child_linker() {
    //? Add child mft record number to its parent
    for (auto it = mft_entries.begin(); it != mft_entries.end(); ++it) {
        uint64_t parent = it->second.parent_mft_record_number;
        if (mft_entries.count(parent))
            mft_entries[parent].sub_files_number.push_back(it->first);
    }

    //? Find the root 
    for (auto it = mft_entries.begin(); it != mft_entries.end(); ++it)
        if (it->second.mft_record_number == it->second.parent_mft_record_number) {
            root = it->first;
            break;
        }
    current_node = root;
}


void NTFS::print_vbr() {
    for (int i = 0; i < VBR_SIZE; ++i) {
        printf("%02x ", (uint8_t)vbr[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
}
void NTFS::print_ntfs_in4() {
    printf("OEM ID: %s\n", oem_id.c_str());
    printf("Bytes per sector: %u\n", bytes_per_sector);
    printf("Sectors per cluster: %u\n", sectors_per_cluster);
    printf("Reserved sectors: %u\n", reserved_sectors);
    printf("Total sectors: %u\n", total_sectors);
    printf("First cluster 0f MFT: %u\n", mft_cluster_number);
    printf("First cluseter of MFT mirror: %u\n", mft_mirror_cluster_number);
    printf("MFT record size: %d\n", mft_record_size);
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
    // Skip the deleted record
    if (flag == 0 || flag == 2)
        throw "Error: Deleted record";

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

    sub_files_number.resize(0); // Initialize the child list
}

vector<string> MFT_Entry::convert2attribute(uint32_t flags) {
    vector<string> attributes;
    if (flags & 0x1) attributes.push_back("READ ONLY");
    if (flags & 0x2) attributes.push_back("HIDDEN");
    if (flags & 0x4) attributes.push_back("SYSTEM");
    if (flags & 0x20) attributes.push_back("ARCHIVE");
    if (flags & 0x40) attributes.push_back("DEVICE");
    if (flags & 0x80) attributes.push_back("NORMAL");
    if (flags & 0x100) attributes.push_back("TEMPORARY");
    if (flags & 0x200) attributes.push_back("SPARSE_FILE");
    if (flags & 0x400) attributes.push_back("REPARSE_POINT");
    if (flags & 0x800) attributes.push_back("COMPRESSED");
    if (flags & 0x1000) attributes.push_back("OFFLINE");
    if (flags & 0x2000) attributes.push_back("NOT_CONTENT_INDEXED");
    if (flags & 0x4000) attributes.push_back("ENCRYPTED");

    return attributes;
}
void MFT_Entry::extract_standard_i4(vector<BYTE> &data, uint64_t start) {
    //? Check the first 4 BYTEs to be 0x10
    uint64_t type_id = cal(data, start, start + 4);
    if (type_id != 0x10)
        throw "Error: Invalid STANDARD INFORMATION attribute";

    uint64_t offset = data[start + 20];
    uint64_t beginIS = start + offset;

    created_time = cal(data, beginIS, beginIS);
    last_modified_time = cal(data, beginIS + 0x8, beginIS + 0x10);

    uint32_t flag = cal(data, beginIS + 0x20, beginIS + 0x24);
    attribute = convert2attribute(flag);
}

void MFT_Entry::extract_file_name(vector<BYTE> &data, uint64_t start) {
    //? Check the first 4 BYTEs to be 0x30
    uint64_t type_id = cal(data, start, start + 4);
    if (type_id != 0x30)
        throw "Error: Invalid FILE NAME attribute";


    uint64_t data_size = cal(data, start + 0x10, start + 0x14);
    uint64_t offset = cal(data, start + 0x14, start + 0x16);

    uint64_t beginFN = start + offset;

    parent_mft_record_number = cal(data, beginFN, beginFN + 0x8);

    uint8_t name_length = data[beginFN + 0x40];

    vector<BYTE> name(name_length * 2);
    for (int i = 0; i < name_length * 2; ++i)
        name[i] = data[beginFN + 0x42 + i];
    file_name = fromUnicode(name);
}


void MFT_Entry::checkdata(vector<BYTE> &data, uint64_t start) {
    //? Check for the Object ID (0x40)
    uint64_t type_id = cal(data, start, start + 0x4);
    if (type_id == 0x40) {
        uint64_t step = cal(data, start + 0x4, start + 0x8);
        start += step;
        type_id = cal(data, start, start + 0x4); // calculate again
    }
    if (type_id == 0x80) //? Data attribute
        extract_data(data, start);
    else if (type_id == 0x90) { //? No data attribute => A directory
        resident = true;
        num_cluster = start_cluster = 0;
        real_size = 0;
        attribute.push_back("DIRECTORY");
    }
}
void MFT_Entry::extract_data(vector<BYTE> &data, uint64_t start) {
    resident = (data[start + 0x8] == 0x00);
    if (resident) {
        real_size = cal(data, start + 0x10, start + 0x14);
        uint64_t offset = cal(data, start + 0x14, start + 0x16);
        uint64_t beginFN = start + offset;

        content.resize(real_size);
        for (int i = 0; i < real_size; ++i)
            content[i] = data[beginFN + i];

        num_cluster = start_cluster = 0; // Resident
    }
    else { // Non-resident, no name
        BYTE datarun_header = data[start + 0x40];
        BYTE length = datarun_header & 0xF; // 4 low bits
        BYTE offset = datarun_header >> 4; // 4 high bits

        real_size = cal(data, start + 0x30, start + 0x38);
        num_cluster = cal(data, start + 0x41, start + 0x41 + length);
        start_cluster = cal(data, start + 0x41 + length, start + 0x41 + length + offset);
    }
}

// Calculate Little endian
uint64_t cal(vector<BYTE> &bytes, int start, int end) {
    uint64_t sum = 0;
    int shift = 0;
    for (int i = start; i < end; i++) {
        sum |= (static_cast<uint64_t>(bytes[i]) << shift);
        shift += 8;
    }
    return sum;
}
wstring fromUnicode(vector<BYTE> &BYTEs) {
    // Cast BYTEs to wchar_t* (UTF-16)
    const wchar_t *wcharData = reinterpret_cast<const wchar_t *>(BYTEs.data());

    // Construct wstring from wchar_t* data
    return wstring(wcharData, BYTEs.size() / sizeof(wchar_t));
}


// int main() {
//     string disk = "D";
//     NTFS ntfs(disk);
//     // ntfs.print_vbr();
//     ntfs.print_ntfs_in4();

//     return 0;
// }
