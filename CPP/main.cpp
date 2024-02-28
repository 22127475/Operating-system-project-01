#include "ntfs.cpp"
#include "FAT32.cpp"
#include "try.cpp"
using namespace std;



int main() {
    system("cls");
    print_team();

    string name = chooseDisk();
    // string name = "D";
    Volume *volume;
    system("cls");

    int check = checkVolume(name);
    if (check == 1) {
        // fprintf(stderr, "It is a FAT32 volume\n");
        volume = new FAT_32(name);
        run(volume);
    }
    else if (check == 2) {
        // fprintf(stderr, "It is a NTFS volume\n\n");
        volume = new NTFS(name);
        run(volume);
    }
    else {
        fprintf(stderr, "Error: Not FAT32 or NTFS\n");
        return 0;
    }

    delete volume;
    return 0;
}