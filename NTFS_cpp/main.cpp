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
    if (format == "FAT32   ") {
        fclose(volume);
        return 1;
    }

    fseeko64(volume, 0x3, 0);
    fread(&format[0], 1, 8, volume);
    if (format == "NTFS    ") {
        fclose(volume);
        return 2;
    }
    return 0; // if not both fat32 and ntfs
}

void print_help() {
    printf("Supported commands:\n");
    printf("  cd <path> - change directory\n");
    printf("  cwd - print current working directory\n");
    printf("  ls - list directory contents\n");
    printf("  tree - print directory tree\n");
    printf("  read <file> - print file contents\n");
}
void print_data(Volume *volume, string name) {
    try {
        vector<BYTE> data = volume->get_data(name);
        for (auto &i : data)
            printf("%c", i);
        printf("\n");
    }
    catch (const char *msg) {
        fprintf(stderr, "Error: %s\n", msg);
    }
}
void run(Volume *volume) {
    system("cls");
    volume->print_base_in4();
    while (true) {
        printf("\n> ");

        char buffer[100];
        fgets(buffer, 100, stdin);
        // string line(l);
        vector<string> command = splitString(string(buffer), " \n");

        if (command[0] == "cd") {
            if (command.size() == 1) {
                fprintf(stderr, "Error: No path specified\n"); //todo re-prompt
                continue;
            }
            if (!volume->cd(command[1]))
                fprintf(stderr, "Error: No such directory\n");
        }
        else if (command[0] == "cwd") {
            wstring path = volume->cwd();
            wprintf(L"%ls\n", path.c_str());
        }
        else if (command[0] == "ls")
            volume->ls();
        else if (command[0] == "tree")
            volume->tree();
        else if (command[0] == "read")
            print_data(volume, command[1]);
        else if (command[0] == "help" || command[0] == "?")
            print_help();
        else if (command[0] == "quit") {
            printf("Goodbye\n");
            return; //todo add prompt
        }
        else
            fprintf(stderr, "Error: Unknown command\n");

    }
}

int main() {
    string name = "D";
    Volume *volume;

    int check = checkVolume(name);
    if (check == 1) {
        fprintf(stderr, "It is FAT32\n");
        // volume = new FAT32(name);
        exit(1);
    }
    else if (check == 2) {
        // fprintf(stderr, "It is NTFS\n");
        volume = new NTFS(name);
        run(volume);
    }
    else {
        fprintf(stderr, "Error: Not FAT32 or NTFS\n");
        exit(1);
    }

    return 0;
}