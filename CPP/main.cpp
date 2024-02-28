#include "ntfs.cpp"
#include "FAT32.cpp"
#include "try.cpp"
using namespace std;



int main() {
    system("cls");
    print_team();

    // string name = "D";
    // string name = chooseDisk();
    vector<string> disk = chooseDisk();
    Volume *volume;
    system("cls");
    
    if (disk[1] == "FAT32   ") {
        // fprintf(stderr, "It is a FAT32 volume\n");
        volume = new FAT_32(disk[0]);
        run(volume);
    }
    else if (disk[1] == "NTFS    ") {
        // fprintf(stderr, "It is a NTFS volume\n\n");
        volume = new NTFS(disk[0]);
        run(volume);
    }
    else {
        fprintf(stderr, "Error: Not FAT32 or NTFS\n");
        return 0;
    }

    delete volume;
    return 0;
}