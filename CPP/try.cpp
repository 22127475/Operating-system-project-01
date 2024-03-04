#include "base.h"
using namespace std;

// team information
void print_team() {
    printf("Members:\n");
    printf("22127026                 On Gia Bao\n");
    printf("22127275              Tran Anh Minh\n");
    printf("22127280       Doan Dang Phuong Nam\n");
    printf("22127465          Bui Nguyen Lan Vy\n");
    printf("22127475               Diep Gia Huy\n");

    printf("\n");
}
// Supported commands
void print_help() {
    printf("HCMU$hell, version 1.0.0 \n");
    printf("info          Print volume information\n");
    printf("\n");
    printf("cd            Change current directoty\n");
    /*printf("-----Detail-----\n");
    printf("NAME\n");
    printf("\tChange directory\n");
    printf("SYNTAX\n");
    printf("\tcd [path | [-i [ID]]] \n");
    printf("ALIASES\n");
    printf("\tcd\n\tchdir\n");
    printf("-----Detail-----\n");*/
    printf("\tSYNTAX: cd [path | [-i [ID]]] \n");
    printf("pwd           Print current working directory\n");
    printf("dir           Display list directory contents\n");
    printf("\tSYNTAX: dir [-h -s | -a] \n");
    printf("\tALIASES: ls \n");
    printf("tree          Print directory tree\n");
    printf("\tSYNTAX: tree [-h -s | -a] \n");
    printf("\n");
    printf("read          Print file contents\n");
    /*printf("-----Detail-----\n");
    printf("NAME\n");
    printf("\tGet-Content\n");
    printf("SYNTAX\n");
    printf("\tread [file]\n");
    printf("ALIASES\n");
    printf("\tgc\n\tread\n");
    printf("-----Detail-----\n");*/

    printf("\tSYNTAX: read [filename | [-i [ID]]] \n");

    printf("clear         Clear the screen\n");
    printf("exit          Exit the program\n");
    /*printf("-----Detail-----\n");
    printf("NAME\n");
    printf("\tExit\n");
    printf("SYNTAX\n");
    printf("\texit\n");
    printf("ALIASES\n");
    printf("\tquit\n\texit\n\tbye\n");*/
    printf("\tALIASES: quit, bye \n");

    /*printf("-----Detail-----\n");*/
    printf("help          Provides help infomation\n");
    /*printf("-----Detail-----\n");
    printf("NAME\n");
    printf("\tHelp\n");
    printf("SYNTAX\n");
    printf("\thelp\n");
    printf("ALIASES\n");
    printf("\thelp\n\t?\n\t-h\n\t--help\n");
    printf("-----Detail-----\n");*/
    printf("\tALIASES: ?, -h, --help \n");



    






    // printf("\n-----------------\n");
    // printf("Supported commands:\n");
    // printf("  'info' - print volume information\n");
    // printf("  'cd [path]' - change directory\n");
    // printf("  'cd -i [ID]' or 'cd --index [ID]' - change directory by index\n");
    // printf("  'pwd' - print current working directory\n");
    // printf("  'ls' or 'dir' - list directory contents\n");
    // printf("  'tree' - print directory tree\n");
    // printf("  'read [file]' - print file contents\n");
    // printf("  'cls' or 'clear' - clear the screen\n");
    // printf("  'quit' or 'exit' - exit the program\n");
    // printf("  '-h' or '--help' or 'help' or '?' - print the support commands\n");
}
// read the file
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
// change directory
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

// while-loop for the command line
void run(Volume *volume) {
    volume->print_base_in4(); // Print the volume information
    printf("\nEnter '?' or 'help' to view the supported commands\n");
    while (true) {
        // printf("\n%s", Utf16toUtf8(volume->cwd()).c_str());
        wprintf(L"%ls", (volume->pwd()).c_str()); // Print the current working directory
        printf(" >> ");

        char buffer[256];
        fgets(buffer, 256, stdin); // takes in command
        // string line(l);
        vector<string> command = splitString(string(buffer), " \n", false, false);

        // command for change directory
        if (command[0] == "cd" || command[0] == "cd.." || command[0] == "cd." || command[0] == "cd\\")
            try_cd(volume, command);

        // get the current working directory
        else if (command[0] == "pwd") {
            volume->Volume::pwd();
            wprintf(L"%ls\n", (volume->pwd()).c_str());
        }
        // printf("%s\n", Utf16toUtf8(volume->cwd()).c_str());

        // list the directory contents
        else if (command[0] == "ls" || command[0] == "dir") {
            command.push_back("");
            vector<string> tmp = splitString(command[1], " ");
            bool hidden = false, system = false;
            for (auto &i : tmp) { // additional options for printing limited files
                if (i == "-a" || i == "--all") 
                    hidden = system = true;
                else if (i == "-s" || i == "--system")
                    system = true;
                else if (i == "-h" || i == "--hidden")
                    hidden = true;
            }
            volume->ls(hidden, system);
        }
        // print the directory tree
        else if (command[0] == "tree") {
            command.push_back("");
            vector<string> tmp = splitString(command[1], " ");
            bool hidden = false, system = false;
            for (auto &i : tmp) { // additional options for printing limited files
                if (i == "-a" || i == "--all")
                    hidden = system = true;
                else if (i == "-s" || i == "--system")
                    system = true;
                else if (i == "-h" || i == "--hidden")
                    hidden = true;
            }
            volume->tree(hidden, system);
        }
        else if (command[0] == "read" ) // read the file
            try_read(volume, command);

        else if (command[0] == "cls" || command[0] == "clear") // clear the terminal screen
            system("cls");
        // print the supported commands
        else if (command[0] == "-h" || command[0] == "--help" || command[0] == "help" || command[0] == "?") 
            print_help();
        else if (command[0] == "info") // print the volume information
            volume->print_base_in4();
        
        else if (command[0] == "exit" || command[0] == "quit" || command[0] == "bye") { // exit the program
            printf("Goodbye\n");
            return;
        }
        else // Unrecognized command
            fprintf(stderr, "Error: Unknown command\n");
        
        printf("\n");
    }
}
// Loop through all the drives and return the drive letter and format
vector<vector<string>> getDrive() {
    vector<vector<string>> drives;
    string format, buffer;
    for (char c = 'A'; c <= 'Z'; c++) { // Loop from A to Z
        string disk = "\\\\.\\" + string(1, c) + ":";
        FILE *f = fopen(disk.c_str(), "rb"); // try to open once
        if (f) {
            format.resize(8);

            buffer.resize(3);
            fread(&buffer[0], 1, 3, f); // Skip the first 3 bytes
            fread(&format[0], 1, 8, f); // Read the format
            if (format == "NTFS    ") { // Check if the format is NTFS
                drives.push_back({string(1, c), format});
                fclose(f);
                continue;
            }

            buffer.resize(0x52 - 0xB);
            fread(&buffer[0], 1, 0x52 - 0xB, f); // Skip the next 0x52 - 0xB bytes
            fread(&format[0], 1, 8, f); // Read the format again
            if (format == "FAT32   ") { // Check if the format is FAT32
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

    printf("Available drives:\n"); // print the available drives
    int i = 0;
    for (auto drive : drives)
        printf("%d.  %s:\\ -- %s\n", ++i, drive[0].c_str(), drive[1].c_str());

    int num;
    char nextChar;
    do { // let the user choose the drive by inputting number
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
    printf("You have chosen drive %s\n", rs[0].c_str()); // Announce the chosen drive
    // Sleep(500);
    return rs;
}