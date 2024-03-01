#include "NTFS.h"

// MFT Header
MFT_Header::MFT_Header(vector<BYTE> &data) {
    info_offset = cal(data, 0x14, 0x16);
    info_size = cal(data, 0x3C, 0x40);

    file_name_offset = info_offset + info_size;
    file_name_size = cal(data, 0x9C, 0xA0);

    data_offset = file_name_offset + file_name_size;
    data_size = cal(data, 0x104, 0x108);

    //? Get the total number of sectors
    num_sector = (cal(data, 0x118, 0x120) + 1) * 8;
}


// MFT Entry
MFT_Entry::MFT_Entry(vector<BYTE> &data, uint64_t record_size) {
    // Save the record size
    this->record_size = record_size;
    // Mark the MFT number
    mft_record_number = cal(data, 0x2C, 0x30);

    // Get the flag of this MFT (Directory, File, Deleted)
    flag = cal(data, 0x16, 0x18);
    if (flag == 0x0 || flag == 0x2)
        throw "Error: Deleted record";

    // Locate the STANDARD INFORMATION
    standard_i4_start = cal(data, 0x14, 0x16);
    standard_i4_size = cal(data, standard_i4_start + 0x4, standard_i4_start + 0x8);
    standard_i4_size = standard_i4_size % 1024;
    // Get some standard information 
    extract_standard_i4(data, standard_i4_start);

    // Locate the FILE NAME
    uint64_t start = standard_i4_start + standard_i4_size;
    file_name_start = start;
    file_name_size = cal(data, file_name_start + 0x4, file_name_start + 0x8);
    file_name_size = file_name_size % 1024;
    // Get FILE NAME information
    extract_file_name(data, file_name_start);

    // There is more than one FILE NAME attribute
    // Get the FILE NAME information again
    start = file_name_start + file_name_size;
    uint64_t type_id = cal(data, start, start + 0x4);
    while (type_id == 0x30)
    {   
        uint64_t step = cal(data, start + 0x4, start + 0x8);
        step = step % record_size;
        extract_file_name(data, start);
        start += step;
        type_id = cal(data, start, start + 0x4);
    } 

    data_size = cal(data, start + 0x4, start + 0x8);
    // Get DATA information
    checkdata(data, start); // + extract data

    if (flag == 0x3) {
        bool has = false;
        for (auto x : attribute) {
            if (x == "DIRECTORY") {
                has = true;
                break;
            }
        }

        if (!has)
            attribute.push_back("DIRECTORY");
    }

    sub_files_number.resize(0); // Initialize the child list
}

// Convert the permission bit string into readable attributes
vector<string> MFT_Entry::convert2attribute(uint64_t flags) {
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
    if (flags & 0x10000000) attributes.push_back("DIRECTORY");
    return attributes;
}
void MFT_Entry::extract_standard_i4(vector<BYTE> &data, uint64_t start) {
    //? Check the first 4 BYTEs to be 0x10
    uint64_t type_id = cal(data, start, start + 4);
    if (type_id != 0x10)
        throw "Error: Invalid STANDARD INFORMATION attribute";

    // Starting position
    uint64_t offset = data[start + 20];
    uint64_t beginIS = start + offset;

    created_time = cal(data, beginIS, beginIS);
    last_modified_time = cal(data, beginIS + 0x8, beginIS + 0x10);

    // Get the DOS file permission
    uint64_t flag = cal(data, beginIS + 0x20, beginIS + 0x24);
    attribute = convert2attribute(flag);
}


void MFT_Entry::extract_file_name(vector<BYTE> &data, uint64_t start) {
    //? Check the first 4 BYTEs to be 0x30
    uint64_t type_id = cal(data, start, start + 4);

    if (type_id == 0x20) { // Attribute list
        uint64_t step = cal(data, start + 0x4, start + 0x8);
        step = step % 1024;
        start += step;
        type_id = cal(data, start, start + 0x4); // calculate again
    }
    if (type_id != 0x30)
        throw "Error: Invalid FILE NAME attribute";

    uint64_t data_size = cal(data, start + 0x10, start + 0x14);
    uint64_t offset = cal(data, start + 0x14, start + 0x16);

    uint64_t beginFN = start + offset;

    // Save its parent MFT record number
    parent_mft_record_number = cal(data, beginFN, beginFN + 0x6);
    // Get the attributes
    uint64_t fl = cal(data, 0x38, 0x3C);
    vector<string> tmp_attr = convert2attribute(fl);
    for (auto x : tmp_attr) {
        bool has = false;
        for (auto y : attribute)
            if (x == y) {
                has = true;
                break;
            }
        if (!has)
            attribute.push_back(x);
    }

    // Get the file namespace
    uint32_t name_space = data[beginFN + 0x41];
    if (name_space == 0x0)
        file_namespace = "POSIX";
    else if (name_space == 0x1)
        file_namespace = "Win32";
    else if (name_space == 0x2)
        file_namespace = "DOS";
    else if (name_space == 0x3)
        file_namespace = "Win32 & DOS";
    else
        file_namespace = "Unknown";

    // The filename length
    uint8_t name_length = data[beginFN + 0x40];

    // Read the filename (Unicode)
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
        step = step % 1024;
        start += step;
        type_id = cal(data, start, start + 0x4); // calculate again
    }
    if (type_id == 0x80) //? Data attribute
        extract_data(data, start);
    else if (type_id == 0x90) { //? No data attribute => A directory
        resident = true;
        // num_cluster = start_cluster = 0;
        real_size = 0;
        attribute.push_back("DIRECTORY");
    }
}
void MFT_Entry::extract_data(vector<BYTE> &data, uint64_t start) {
    // Check if the data is resident
    resident = (data[start + 0x8] == 0x00);
    if (resident) {
        real_size = cal(data, start + 0x10, start + 0x14);

        // Locate the content
        uint64_t offset = cal(data, start + 0x14, start + 0x16);
        uint64_t beginFN = start + offset;

        // Save the content
        content.resize(real_size);
        for (int i = 0; i < real_size; ++i)
            content[i] = data[beginFN + i];

    }
    else {
        real_size = cal(data, start + 0x30, start + 0x38);

        // Mark the datarun
        start += 0x40;
        while (data[start] != 0x00) {
            BYTE datarun_header = data[start];
            BYTE length = datarun_header & 0xF; // 4 low bits
            BYTE offset = datarun_header >> 4; // 4 high bits

            // get the number of clusters of each datarun
            num_cluster.push_back(cal(data, start + 0x1, start + 0x1 + length));
            // get the starting cluster of each datarun
            start_cluster.push_back(cal(data, start + 0x1 + length, start + 0x1 + length + offset));
            start += 1 + length + offset;
        }
    }
}

bool MFT_Entry::is_directory() {
    for (auto &x : attribute)
        if (x == "DIRECTORY")
            return true;
    return false;
}
bool MFT_Entry::is_archive() {
    for (auto &x : attribute)
        if (x == "ARCHIVE")
            return true;
    return false;
}
bool MFT_Entry::is_hidden() {
    for (auto &x : attribute)
        if (x == "HIDDEN")
            return true;
    return false;
}
bool MFT_Entry::is_system() {
    for (auto &x : attribute)
        if (x == "SYSTEM")
            return true;
    return false;
}


// NTFS
NTFS::NTFS(string name) {
    // Open the volume
    disk_name = name + ":";
    name = "\\\\.\\" + disk_name;
    volume = fopen(name.c_str(), "rb");

    if (!volume) {
        fprintf(stderr, "Error: Permission denied\n");
        exit(1);
    }

    // Read the VBR
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

    //? Read MFT
    vector<BYTE> mft_header(mft_record_size);
    fseeko64(volume, mft_offset, 0);
    bytesRead = fread(mft_header.data(), 1, mft_record_size, volume);
    MFT_Header mft_header_info(mft_header);



    //? Loop through the MFT to get all the MFT entries
    vector<BYTE> mft_entry(mft_record_size);
    int step = mft_record_size / bytes_per_sector;

    for (uint64_t i = 2; i < mft_header_info.num_sector; i += step) {
        bytesRead = fread(mft_entry.data(), 1, mft_record_size, volume);

        string mark = "    ";
        for (int j = 0; j < 4; ++j)
            mark[j] = mft_entry[j];
        if (mark != "FILE") continue; //? Skip the anomal record

        try {
            MFT_Entry mft_entry_info(mft_entry, mft_record_size);
            // Mark this MFT saving sector
            uint64_t num_sector = (i / 2 * mft_record_size + mft_offset) / bytes_per_sector + reserved_sectors;
            mft_entry_info.sector_list.push_back(num_sector);

            // Save into the list of MFT entries
            mft_entries[mft_entry_info.mft_record_number] = mft_entry_info;
        }
        catch (const char *msg) {
            continue;
        }
    }
    child_linker(); //? Link child to its parent
}
NTFS::~NTFS() {
    fclose(volume); //? Close the volume when exit
}

// Check whether is it NTFS or not
bool NTFS::is_NTFS() {
    if (oem_id == "NTFS    ")
        return true;
    return false;
}
void NTFS::extract_vbr() {
    oem_id.resize(8);
    for (int i = 3; i < 0xB; i++)                   // Get the OEM ID (NTFS)
        oem_id[i - 3] = vbr[i];

    bytes_per_sector = cal(vbr, 0xB, 0xD);          // Byte per sector
    sectors_per_cluster = vbr[0xD];                 // Sectors per cluster
    reserved_sectors = cal(vbr, 0xE, 0x10);         // Reserved sectors
    total_sectors = cal(vbr, 0x28, 0x30);           // Total sectors
    mft_cluster_number = cal(vbr, 0x30, 0x38);      // First cluster of $MFT
    mft_mirror_cluster_number = cal(vbr, 0x38, 0x40); // First cluster of $MFTMirr

    // Signed number, get the record size
    uint64_t recordsize = vbr[0x40];
    if (recordsize > 127) recordsize = 256 - recordsize;
    mft_record_size = 1 << recordsize;


    serial_number = cal(vbr, 0x48, 0x50);          // Serial number

    // Calculate the the offset of the first MFT record
    mft_offset = mft_cluster_number * sectors_per_cluster * bytes_per_sector;
}
void NTFS::child_linker() {
    //? Add child mft record number to its parent
    for (auto it = mft_entries.begin(); it != mft_entries.end(); ++it) {
        uint64_t parent = it->second.parent_mft_record_number;
        // Put the child id inside its parent
        if (mft_entries.count(parent))
            mft_entries[parent].sub_files_number.push_back(it->first);
    }

    //? Find the root 
    for (auto it = mft_entries.begin(); it != mft_entries.end(); ++it)
        if (it->second.mft_record_number == it->second.parent_mft_record_number) {
            root = it->second.mft_record_number;
            break;
        }
    current_node.push_back(root); // Assign the current directory flow at the root
}

void NTFS::print_vbr() {
    for (int i = 0; i < VBR_SIZE; ++i) {
        printf("%02x ", (uint8_t)vbr[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
}
// Print out the basic information
void NTFS::print_base_in4() {
    Volume::print_base_in4();
    printf("\n");
    printf("Disk:                       %s\\\n", disk_name.c_str());
    printf("OEM ID:                     %s\n", oem_id.c_str());
    printf("Bytes per sector:           %u B\n", bytes_per_sector);
    printf("Sectors per cluster:        %u\n", sectors_per_cluster);
    printf("Reserved sectors:           %u\n", reserved_sectors);
    printf("Total sectors:              %u\n", total_sectors);
    printf("First cluster of $MFT:      %u\n", mft_cluster_number);
    printf("First cluster of $MFTMirr:  %u\n", mft_mirror_cluster_number);
    printf("MFT record size:            %d B\n", mft_record_size);
}
// Find the child MFT record by searching its name
uint64_t NTFS::find_mft_entry(const string &record_name) {
    uint64_t des = 0;
    for (auto x : mft_entries[current_node.back()].sub_files_number)
        if (compareWstrVsStr(mft_entries[x].file_name, record_name)) {
            des = x;
            break;
        }
    return des;
}

// Read the content of the record by giving the path
void NTFS::read(const string &name) {
    string tmp_path = name;
    if (tmp_path[0] == '\"' && tmp_path[tmp_path.size() - 1] == '\"') // Remove the quote
        tmp_path = tmp_path.substr(1, tmp_path.size() - 2);

    uint64_t des = find_mft_entry(tmp_path); // Find the child MFT record
    if (tmp_path == "")
        des = current_node.back(); // If no path is given, read the current directory

    vector<string> tmp_split = splitString(tmp_path, " ");
    if (tmp_split[0] == "-i" || tmp_split[0] == "--index") { // Choose path by using index
        if (tmp_split.size() < 2) { // No index specified
            throw "Error: No index specified\n";
            return;
        }
        if (!isNumber(tmp_split[1])) { // Invalid index
            throw "Error: Invalid index\n";
            return;
        }
        uint64_t index = stoull(tmp_split[1]); // Convert the index to number
        for (auto &x : mft_entries[current_node.back()].sub_files_number)
            if (mft_entries[x].mft_record_number == index) {
                des = x;
                break;
            }
    }
    if (des == 0) // No child found
        throw "File not found";

    MFT_Entry mft = mft_entries[des];

    mft.info(); // Print the information of the file
    printf("\n------------CONTENT------------\n");

    if (mft.is_directory()) { // if directory, print its sub-files tree
        // change_dir(name);
        current_node.push_back(des);
        tree(false, false);
        // change_dir("..");
        current_node.pop_back();
        return;
    }

    // Check the file extension to be .txt
    wstring ext = mft.file_name;
    if (mft.file_name.size() > 4)
        ext = mft.file_name.substr(mft.file_name.size() - 4);

    if (ext != L".txt" && ext != L".TXT") {
        printf("Please use the appropriate reader to read this file.\n");
        return;
    }

    if (mft.resident) { // If resident, print its content
        for (auto &x : mft.content)
            printf("%c", x);
        printf("\n");
        return;
    }

    // Non-resident file
    for (int i = 0; i < mft.num_cluster.size(); i++) {
        // Locate the datarun
        uint64_t size = mft.num_cluster[i] * bytes_per_sector * sectors_per_cluster;
        uint64_t offset = mft.start_cluster[i] * bytes_per_sector * sectors_per_cluster;
        vector<BYTE> data(size);
        fseeko64(volume, offset, 0);
        // read each data run into the buffer
        size_t bytesRead = fread(data.data(), 1, size, volume);

        // print out the content read from the datarun
        for (auto &x : data)
            printf("%c", x);
    }
    printf("\n");
    return;
}

bool NTFS::change_dir(string path) {
    if (path == "\\") { // cd\, go back to the root
        current_node.resize(1);
        return true;
    }
    vector<string> paths = splitString(path, " ", 0);
    if (paths[0] == "-i" || paths[0] == "--index") { // change the current directory by index
        if (paths.size() < 2) { // No index specified
            throw "Error: No index specified\n";
            return false;
        }
        if (!isNumber(paths[1])) { // Invalid index
            throw "Error: Invalid index\n";
            return false;
        }
        uint64_t index = stoull(paths[1]); // Convert the index to number and move to that child directory
        for (auto &x : mft_entries[current_node.back()].sub_files_number)
            if (mft_entries[x].mft_record_number == index && mft_entries[x].is_directory()) {
                current_node.push_back(x);
                return true;
            }
        throw "Error: No such directory found\n"; // No child found
        return false;
    }

    paths = splitString(path); // Split the path by '\'
    vector<uint64_t> temp = current_node; // Backup the current node

    if (paths[0] == disk_name) { // Absolute path
        current_node.resize(1);
        paths.erase(paths.begin());
    }

    for (auto &x : paths) {
        if (x == "..") { // back to the parent directory
            if (current_node.size() > 1)
                current_node.pop_back();
        }

        else if (x == ".") // do nothing (stay at the current directory)
            continue;

        else {
            uint64_t des = find_mft_entry(x);
            if (des == 0 || !mft_entries[des].is_directory()) // Only allow to move the directory 
            {
                current_node = temp;
                throw "Error: No such directory found\n";
                return false;
            }
            current_node.push_back(des);
        }
    }
    return true;
}
wstring NTFS::get_current_path() { // Get the current directory flow
    wstring path;
    path += wstring(disk_name.begin(), disk_name.end());

    if (current_node.size() == 1)
        return path += L"\\";
    for (int i = 1; i < current_node.size(); i++) { // join the path by '\'
        path += L"\\";
        path += mft_entries[current_node[i]].file_name;
    }
    return path;
}


// directory-archive-readonly-hidden-system-reparse point
// Convert the attribute to a bit string
string attribute_bit(vector<string> &attribute) {
    string attr = "------";
    for (auto &x : attribute) {
        if (x == "DIRECTORY") attr[0] = 'd';
        else if (x == "ARCHIVE") attr[1] = 'a';
        else if (x == "READ ONLY") attr[2] = 'r';
        else if (x == "HIDDEN") attr[3] = 'h';
        else if (x == "SYSTEM") attr[4] = 's';
        else if (x == "REPARSE POINT") attr[5] = 'l';
    }
    return attr;
}
// List all the files in the current directory
void NTFS::list(bool hidden, bool system) {
    uint64_t node = current_node.back();

    Volume::ls();
    for (auto &x : mft_entries[node].sub_files_number) {
        // if (!print_hidden && mft_entries[x].is_hidden_system())
            // continue;
        if (!hidden && mft_entries[x].is_hidden()) // Skip the hidden file if not called
            continue;
        if (!system && mft_entries[x].is_system()) // Skip the system file if not called
            continue;
        string attr = attribute_bit(mft_entries[x].attribute); // print the attribute list
        printf("%s\t", attr.c_str());
        uint64_t id = mft_entries[x].mft_record_number; // print the MFT record number for index purpose
        printf("%llu\t", id);

        wstring name = mft_entries[x].file_name; // print the file name
        wprintf(L"%ls\n", name.c_str());
        // printf("%s\n", Utf16toUtf8(name).c_str());
    }
}
// print the tree of the current directory
void NTFS::print_tree(bool hidden, bool system, uint64_t entry, string prefix, bool last) {
    if (entry == 0) entry = current_node.back();
    MFT_Entry mft = mft_entries[entry];


    wprintf(L"%ls\n", mft.file_name.c_str()); // print the current mft name
    // printf("%s\n", Utf16toUtf8(mft.file_name).c_str());
    if (mft.is_archive())
        return;


    // handle printing the folder and its sub-files
    int lst = mft.sub_files_number.size() - 1; // Find the last file to be printed
    while (lst >= 0 && ((!hidden && mft_entries[mft.sub_files_number[lst]].is_hidden())
        || (!system && mft_entries[mft.sub_files_number[lst]].is_system()))) // Skip the hidden and system file if not called
        lst--;
    for (int i = 0; i < mft.sub_files_number.size(); i++) {
        if (mft_entries[mft.sub_files_number[i]].is_hidden() && !hidden) // Skip the hidden file if not called
            continue;
        if (mft_entries[mft.sub_files_number[i]].is_system() && !system) // Skip the system file if not called
            continue;

        printf("%s", (prefix + "+---").c_str()); // print the prefix before the file name
        if (i != lst) {
            // printf("%s", (prefix + char(195) + char(196)).c_str());
            // print_tree(hidden, system, mft.sub_files_number[i], prefix + char(179) + " ", false);
            print_tree(hidden, system, mft.sub_files_number[i], prefix + "|   ", false); // print the sub-files but not the last
        }
        else {
            // printf("%s", (prefix + char(192) + char(196)).c_str());
            // print_tree(hidden, system, mft.sub_files_number[i], prefix + "  ", true);
            print_tree(hidden, system, mft.sub_files_number[i], prefix + "    ", true); // print the last child file
        }
    }
}

//! Here
void MFT_Entry::info(const string &path) {
    // Volume::read();
    printf("--------------Info-------------\n");

    // Print out the name and it filename namespace
    wprintf(L"Name: %ls\n", file_name.c_str());
    printf("Filename namespace: %s\n", file_namespace.c_str());

    // Print the attribute bit string
    printf("Attribute: ");
    for (string &s : attribute)
        printf("%s    ", s.c_str());
    printf("\n");

    printf("Size: %u B\n", real_size); // print the size

    printf("Sector of this $MFT record: %u\n", sector_list[0]); // print the $MFT record storing sector

    if (!resident) { // Print the list of dataruns follow: start cluster and number of clusters
        printf("-------------------------------\n");
        printf("            |         Start |   Number of    \n");
        printf("            |       Cluster |   Clusters     \n");
        for (size_t i = 0; i < start_cluster.size(); i++) {
            printf("Data-run % 3u|\t", i + 1);

            printf("% 11u | ", start_cluster[i]);
            printf("% 6u\n", num_cluster[i]);
        }
    }
}

// Support functions
// Calculate the value of a BYTE array following Little Endian format
uint64_t cal(vector<BYTE> &bytes, int start, int end) {
    uint64_t sum = 0;
    int shift = 0;
    for (int i = start; i < end; i++) { // Add up and shift the BYTEs
        sum |= (static_cast<uint64_t>(bytes[i]) << shift);
        shift += 8; // Increase the shift value
    }
    return sum;
}
wstring fromUnicode(vector<BYTE> &BYTEs) { // Convert unicode character to wstring
    const wchar_t *wcharData = reinterpret_cast<const wchar_t *>(BYTEs.data());

    return wstring(wcharData, BYTEs.size() / sizeof(wchar_t));
}

