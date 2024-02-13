#include "ntfs.cpp"
#include "FAT32.cpp"
using namespace std;

void print_team() {
    printf("Members:\n");
    printf("22127026                 On Gia Bao\n");
    printf("22127275              Tran Anh Minh\n");
    printf("22127280       Doan Dang Phuong Nam\n");
    printf("22127465          Bui Nguyen Lan Vy\n");
    printf("22127475               Diep Gia Huy\n");

    printf("\n");
}


int checkVolume(string name) {
    name = "\\\\.\\" + name + ":";
    FILE *volume = fopen(name.c_str(), "rb");
    if (!volume) {
        fprintf(stderr, "Error: Permission denied\n");
        exit(1);
    }

    string format, buffer;
    format.resize(8);

    buffer.resize(3);
    fread(&buffer[0], 1, 3, volume);
    fread(&format[0], 1, 8, volume);
    if (format == "NTFS    ") {
        fclose(volume);
        return 2;
    }

    buffer.resize(0x52 - 0xB);
    fread(&buffer[0], 1, 0x52 - 0xB, volume);
    fread(&format[0], 1, 8, volume);
    if (format == "FAT32   ") {
        fclose(volume);
        return 1;
    }

    fclose(volume);
    return 0;
}

void try_read(Volume *volume, string name) {
    try {
        volume->read(name);
    }
    catch (const char *msg) {
        fprintf(stderr, "Error: %s\n", msg);
    }
}
void try_cd(Volume *volume, vector<string> command) {
    if (command.size() == 1) {
        fprintf(stderr, "Error: No path specified\n");
        return;
    }
    if (command[1][0] == '\"' ^ command[1][command[1].size() - 1] == '\"') { // xor for if the begin and end are different
        fprintf(stderr, "Error: The \"\" must be completed\n");
        return;
    }

    if (!volume->cd(command[1]))
        fprintf(stderr, "Error: No such directory found\n");
}
void print_help() {
    printf("Supported commands:\n");
    printf("  cd <path> - change directory\n");
    printf("  cwd - print current working directory\n");
    printf("  ls - list directory contents\n");
    printf("  dir - list directory contents\n");
    printf("  tree - print directory tree\n");
    printf("  read <file> - print file contents\n");
    printf("  exit - exit the program\n");

}
void run(Volume *volume) {

    volume->print_base_in4();
    printf("\nEnter '?' or 'help' to view the supported commands\n");
    while (true) {
        // printf("\n%s", Utf16toUtf8(volume->cwd()).c_str());
        wprintf(L"%ls", (volume->cwd()).c_str());
        printf(" >> ");

        char buffer[256];
        fgets(buffer, 256, stdin);
        // string line(l);
        vector<string> command = splitString(string(buffer), " \n", false);

        if (command[0] == "cd")
            try_cd(volume, command);

        else if (command[0] == "cwd")
            wprintf(L"%ls\n", (volume->cwd()).c_str());
            // printf("%s\n", Utf16toUtf8(volume->cwd()).c_str());

        else if (command[0] == "ls" || command[0] == "dir")
            volume->ls();
        else if (command[0] == "tree")
            volume->tree();
        else if (command[0] == "read")
            try_read(volume, command[1]);

        else if (command[0] == "cls")
            system("cls");
        else if (command[0] == "help" || command[0] == "?")
            print_help();
        else if (command[0] == "exit" || command[0] == "quit") {
            printf("Goodbye\n");
            return; //todo add prompt
        }
        else
            fprintf(stderr, "Error: Unknown command\n");
    }
}

string chooseDisk() {
    DWORD drivesBitMask = GetLogicalDrives();

    vector<char> drives;
    for (int i = 0; i < 26; ++i)  // Assume there are at most 26 drive letters
        if (drivesBitMask & (1 << i))
            drives.push_back('A' + i);

    printf("Available drives:\n");
    int i = 0;
    for (char drive : drives)
        printf("%d.  %c:\\\n", ++i, drive);

    int num;
    char nextChar;
    do {
        printf("\nEnter an integer between 1 and %d: ", drives.size());
        if (scanf("%d", &num) != 1 || num < 1 || num > drives.size()) {
            // Clear the input buffer
            while ((nextChar = getchar()) != '\n' && nextChar != EOF) {}
            printf("Invalid input! Please enter an integer between 1 and %d\n", drives.size());
        }
        else if (num < 1 || num > drives.size()) {
            printf("Invalid input! Please enter an integer between 1 and %d\n", drives.size());
        }
    } while (num < 1 || num > drives.size());
    while ((nextChar = getchar()) != '\n' && nextChar != EOF) {}

    string rs(1, drives[num - 1]);
    printf("You have chosen drive %s\n", rs.c_str());
    Sleep(500);
    return rs;
}
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
        exit(1);
    }

    return 0;
}

// int main() {
//     string name = "D";
//     // Volume *volume = new FAT_32(name);
//     Volume *volume = new NTFS(name);
//     run(volume);

//     return 0;
// }