#include "ntfs.cpp"

int main() {
    string disk = "D";
    NTFS ntfs(disk);
    // ntfs.print_vbr();
    ntfs.print_ntfs_in4();

    return 0;
}