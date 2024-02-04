#include "ntfs.cpp"

int main() {
    string disk = "D";
    NTFS ntfs(disk);
    ntfs.print_vbr();

    return 0;
}
