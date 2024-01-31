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
    
    //! Read the mft entry

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

// Calculate Little endian
uint32_t cal(vector<char> &bytes, int start, int end) {
    uint32_t sum = 0;
    int offset = (end - start) * 8;
    for (int i = start; i < end; i++) {
        sum += (uint8_t)bytes[i] << offset;
        offset -= 8;
    }
    return sum;
}

int main() {
    string disk = "D";
    NTFS ntfs(disk);
    // ntfs.print_vbr();

    return 0;
}
