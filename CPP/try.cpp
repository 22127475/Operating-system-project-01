#include "base.h"
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
void print_help() {
    printf("Supported commands:\n");
    printf("  'info' - print volume information\n");
    printf("  'cd [path]' - change directory\n");
    printf("  'cd -i [ID]' or 'cd --index [ID]' - change directory by index\n");
    printf("  'pwd' - print current working directory\n");
    printf("  'ls' or 'dir' - list directory contents\n");
    printf("  'tree' - print directory tree\n");
    printf("  'read [file]' - print file contents\n");
    printf("  'cls' or 'clear' - clear the screen\n");
    printf("  'quit' or 'exit' - exit the program\n");
    printf("  '-h' or '--help' or 'help' or '?' - print the support commands\n");
}
void try_read(Volume *volume, std::vector<std::string> name) {
    try {
        if (name.size() == 1)
            volume->read("");
        else
            volume->read(name[1]);
    }
    catch (const char *msg) {
        fprintf(stderr, "Error: %s\n", msg);
    }
}
void try_cd(Volume *volume, vector<string> command) {
    if (command[0] == "cd.." || command[0] == "cd." || command[0] == "cd\\") {
        volume->cd(command[0].substr(2));
        return;
    }

    if (command.size() == 1) {
        fprintf(stderr, "Error: No path specified\n");
        return;
    }
    if (command[1][0] == '\"' ^ command[1][command[1].size() - 1] == '\"') { // xor for if the begin and end are different
        fprintf(stderr, "Error: The \"\" must be completed\n");
        return;
    }

    try {
        volume->cd(command[1]);
    }
    catch (const char *msg) {
        fprintf(stderr, "%s", msg);
    }
}


void run(Volume *volume) {
    volume->print_base_in4();
    printf("\nEnter '?' or 'help' to view the supported commands\n");
    while (true) {
        // printf("\n%s", Utf16toUtf8(volume->cwd()).c_str());
        wprintf(L"%ls", (volume->pwd()).c_str());
        printf(" >> ");

        char buffer[256];
        fgets(buffer, 256, stdin);
        // string line(l);
        vector<string> command = splitString(string(buffer), " \n", false, false);

        if (command[0] == "cd" || command[0] == "cd.." || command[0] == "cd." || command[0] == "cd\\")
            try_cd(volume, command);

        else if (command[0] == "pwd") {
            volume->Volume::pwd();
            wprintf(L"%ls\n", (volume->pwd()).c_str());
        }
        // printf("%s\n", Utf16toUtf8(volume->cwd()).c_str());

        else if (command[0] == "ls" || command[0] == "dir") {
            command.push_back("");
            vector<string> tmp = splitString(command[1], " ");
            bool hidden = false, system = false;
            for (auto &i : tmp) {
                if (i == "-a" || i == "--all")
                    hidden = system = true;
                else if (i == "-s" || i == "--system")
                    system = true;
                else if (i == "-h" || i == "--hidden")
                    hidden = true;
            }
            volume->ls(hidden, system);
        }
        else if (command[0] == "tree") {
            command.push_back("");
            vector<string> tmp = splitString(command[1], " ");
            bool hidden = false, system = false;
            for (auto &i : tmp) {
                if (i == "-a" || i == "--all")
                    hidden = system = true;
                else if (i == "-s" || i == "--system")
                    system = true;
                else if (i == "-h" || i == "--hidden")
                    hidden = true;
            }
            volume->tree(hidden, system);
        }
        else if (command[0] == "read")
            try_read(volume, command);

        else if (command[0] == "cls" || command[0] == "clear")
            system("cls");
        else if (command[0] == "-h" || command[0] == "--help" || command[0] == "help" || command[0] == "?")
            print_help();
        else if (command[0] == "exit" || command[0] == "quit") {
            printf("Goodbye\n");
            return; //todo add prompt
        }
        else if (command[0] == "info")
            volume->print_base_in4();
        else
            fprintf(stderr, "Error: Unknown command\n");
        
        printf("\n");
    }
}
vector<vector<string>> getDrive() {
    vector<vector<string>> drives;
    string format, buffer;
    for (char c = 'A'; c <= 'Z'; c++) {
        string disk = "\\\\.\\" + string(1, c) + ":";
        FILE *f = fopen(disk.c_str(), "rb");
        if (f) {
            format.resize(8);

            buffer.resize(3);
            fread(&buffer[0], 1, 3, f);
            fread(&format[0], 1, 8, f);
            if (format == "NTFS    ") {
                drives.push_back({string(1, c), format});
                fclose(f);
                continue;
            }

            buffer.resize(0x52 - 0xB);
            fread(&buffer[0], 1, 0x52 - 0xB, f);
            fread(&format[0], 1, 8, f);
            if (format == "FAT32   ") {
                drives.push_back({string(1, c), format});
                fclose(f);
                continue;
            }

            drives.push_back({string(1, c), "Other format"});
            fclose(f);
        }
    }
    return drives;
}
vector<string> chooseDisk() {
    vector<vector<string>> drives = getDrive();

    // DWORD drivesBitMask = GetLogicalDrives();
    // vector<char> drives;
    // for (int i = 0; i < 26; ++i)  // Assume there are at most 26 drive letters
    //     if (drivesBitMask & (1 << i))
    //         drives.push_back('A' + i);

    printf("Available drives:\n");
    int i = 0;
    for (auto drive : drives)
        printf("%d.  %s:\\ -- %s\n", ++i, drive[0].c_str(), drive[1].c_str());

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

    vector<string> rs = drives[num - 1];
    printf("You have chosen drive %s\n", rs[0].c_str());
    // Sleep(500);
    return rs;
}