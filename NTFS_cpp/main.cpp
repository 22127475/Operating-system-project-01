#include "ntfs.cpp"

int checkVolume(string name) {
    name = "\\\\.\\" + name + ":";
    FILE *volume = fopen(name.c_str(), "rb");
    if (!volume) {
        fprintf(stderr, "Error: Permission denied\n");
        exit(1);
    }

    string format = "        ";
    fseeko64(volume, 0x52, 0);
    fread(&format[0], 1, 8, volume);
    if (format == "FAT32   ")
        return 1;

    fseeko64(volume, 0x3, 0);
    fread(&format[0], 1, 8, volume);
    if (format == "NTFS    ")
        return 2;
    return 0; // if not both fat32 and ntfs
}

void run(Volume *volume) {
    volume->print_base_in4();
    // while (true) {
    //     vector<string> 
    // }
    volume->list();
}

int main() {
    string name = "E";
    Volume *volume;

    int check = checkVolume(name);
    if (check == 1)
        fprintf(stderr, "It is FAT32\n");
        // volume = new FAT32(name);
    else if (check == 2) {
        fprintf(stderr, "It is NTFS\n");
        volume = new NTFS(name);
    }
    else {
        fprintf(stderr, "Error: Not FAT32 or NTFS\n");
        exit(1);
    }

    // run(volume);

    return 0;
}