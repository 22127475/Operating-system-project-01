#include "NTFS.cpp"
#include "FAT32.cpp"
#include "try.cpp"
using namespace std;

int main() {
    system("cls"); // Clear the screen before use
    print_team(); // Output the group information

    // string name = "D";
    // string name = chooseDisk();
    vector<string> disk = chooseDisk(); // Choose the disk and get the disk format
    Volume *volume;
    system("cls");

    if (disk[1] == "FAT32   ") { // Fat32 format
        volume = new FAT_32(disk[0]);
        run(volume);
    }
    else if (disk[1] == "NTFS    ") { // NTFS format
        volume = new NTFS(disk[0]);
        run(volume);
    }
    else { // No supported format
        fprintf(stderr, "Error: Not FAT32 or NTFS\n");
        return 0;
    }

    delete volume;
    return 0;
}