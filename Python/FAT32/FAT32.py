from FAT32.BootSector import BootSector
from FAT32.constant import *
from FAT32.Directory import Directory
from FAT32.FAT_Table import FATTable
from FAT32.Entry import Entry

def checkFAT32 (volumeName: str):
    try:
        volumePath = r"\\." + f"\{volumeName}:"
        with open (volumePath, 'rb') as f:
            f.read(1) #? For avoid ERROR INVALID ARGUMENT, we can change 1 by any value
            f.seek(0x52)
            FAT_Name = f.read(8)
            
            if (FAT_Name == b"FAT32   "):
                return True
            return False
    except Exception as e:
        print ("Error:", e)
        exit()

class FAT32:
    def __init__ (self, volume_name):
        self.volumeName = volume_name
        self.path = f"{self.volumeName}:\\"
        
        #? Components
        self.bootSector = None
        self.listFAT_Tables: list[FATTable] = []
        self.RDET: Directory = None
        #? Dictionary for looking up RDET or SDET(s), key is the starting_cluster
        self.DET: dict[int, Directory] = dict()
        
        self.readComponents()
    
    def readComponents (self):
        try:
            volumePath = r'\\.' + f'\{self.volumeName}:'
            with open (volumePath, 'rb') as f:
                #! Read Boot Sector
                #? Because at this time, the parameter bytePerSector is not defined, so we use the value 512 as the common number of bytes in a sector
                bootSector_rawData = f.read(512) 
                self.bootSector = BootSector(bootSector_rawData)
                
                #! Read FAT Tables
                #? From the first bytes (the second parameter is 0), we seek the first sector of FAT Table 1
                f.seek(self.bootSector.getFirstSectorOfFAT1() * self.bootSector.bytesPerSector, 0)
                for i in range (0, self.bootSector.numFATCopies):
                    FATTable_rawData = f.read(self.bootSector.FAT_Size * self.bootSector.bytesPerSector)
                    self.listFAT_Tables.append(FATTable(FATTable_rawData, self.bootSector))
                    
                #! Read RDET
                #? From the starting cluster read from Boot Sector, we will read all sectors of RDET, through FAT Table 1
                RDET_listSectors = self.listFAT_Tables[0].getListSectors(self.bootSector.RDET_startingCluster)
                RDET_rawData = b""
                for sector in RDET_listSectors:
                    f.seek(sector * self.bootSector.bytesPerSector, 0)
                    RDET_rawData += f.read(self.bootSector.bytesPerSector)
                
                self.RDET = Directory(self.volumeName, self.path, RDET_rawData, self.listFAT_Tables[0], self.DET)
                self.DET[self.bootSector.RDET_startingCluster] = self.RDET
                
        except Exception as e:
            print ("Error:", e)
            exit()
    
    def findDirectory (self) -> Directory:
        Det: Directory = None

        for det in self.DET.values():
            if (det.path == self.path):
                Det = det
                break
            
        return Det
    
    def findEntryInDirectory (self, Det: Directory, nameEntry: str) -> Entry:
        dir_Entry: Entry = None
        for entry in Det.directoryTree:
            if (entry.attribute in (ARCHIVE, DIRECTORY) and entry.name.lower() == nameEntry.lower()):
                    dir_Entry = entry
                    break
                
        return dir_Entry
        
    def changeDirectory (self, listArgs: list[str]):
        #! Change the path
        if (listArgs[0] == ".."):
            if (self.path != f"{self.volumeName}:\\"):
                listDirs = self.path.split('\\')
                
                newPath = ""
                if (len(listDirs) == 2):
                    newPath = f"{self.volumeName}:\\"
                else:
                    for i in range (0, len(listDirs) - 2):
                        newPath += listDirs[i] + "\\"
                    newPath += listDirs[len(listDirs) - 2]
                self.path = newPath
        
        #! Check the existence of the directory given by the first element of the argument list
        elif (listArgs[0] != "."):
            try:
                #! Find directory corresponding the current path
                Det = self.findDirectory()
                
                #! Find entry based on the given argument
                dir_Entry: Entry = self.findEntryInDirectory(Det, listArgs[0])
                if (dir_Entry is None):
                    raise Exception (f"Not found {listArgs[0]}")
                elif (dir_Entry.attribute != DIRECTORY):
                    raise Exception (f"{listArgs[0]} is not a directory")
                
                if (self.path == f"{self.volumeName}:\\"):
                    self.path += dir_Entry.name
                else:
                    self.path += "\\" + dir_Entry.name
            except Exception as e:
                print("Error:", e)
    
    def drawDirectoryTree (self):
        #! Find directory corresponding the current path
        Det = self.findDirectory()
        #!Draw directory tree
        Det.drawDirectoryTree()
        
    def readTextContext (self, listArgs: list[str]):
        #! Find directory corresponding the current path
        Det = self.findDirectory()
        
        for Arg in listArgs:
            fileContent = ""
            try:
                #! Find entry based on the given argument
                dir_Entry: Entry = self.findEntryInDirectory(Det, Arg)
                if (dir_Entry is None):
                    raise Exception (f"Not found {Arg}")
                elif (dir_Entry.attribute != ARCHIVE):
                    raise Exception (f"{Arg} is not a file")
                
                fileContent = dir_Entry.getFileContent()[:dir_Entry.fileSize].decode()
            except UnicodeDecodeError:
                print (f"{Arg} is not a text file, please use an appropriate application to read the content")
                return
            except Exception as e:
                print(f"Error in {Arg}:", e)
                return
            
            print(fileContent)
    
    def listAllSupportCommands (self):
        print("CD             Displays the name of or changes the current directory.")
        print("HELP           Provides Help information for Windows commands.")
        print("TREE           Graphically displays the directory structure of a drive or path.")
        print("TYPE           Displays the contents of a text file.")
    
    def doCommand (self, input: str):
        input = input.strip()
        templistArgs = input.split()
        
        try:
            if (len(templistArgs) == 0):
                raise Exception ("Input nothing")
            
            command = templistArgs[0]
            listArgs = templistArgs[1:]
            
            if (command[0:2].lower() == "cd"):
                try:
                    temp = command[2:]
                    tempStr = ""
                    for i in range (0, len(temp)):
                        tempStr += "."
                    if (temp != tempStr):
                        raise Exception ("cd [Directory]")
                    else:
                        if (len(temp) == 2):
                            listArgs = [".."]
                        elif (len(temp) >= 1):
                            listArgs = ["."]
                    
                    if (len(listArgs) != 1):
                        raise Exception ("cd [Directory]")
                    
                    self.changeDirectory(listArgs)
                except Exception as e:
                    print("Error:", e)
            elif (command.lower() == "tree"):
                if (len(listArgs) != 0):
                    raise Exception ("There is nothing after the command \"tree\"")
                self.drawDirectoryTree()
            elif (command.lower() == "type"):
                self.readTextContext(listArgs)
            elif (command.lower() == "help"):
                if (len(listArgs) != 0):
                    raise Exception ("There is nothing after the command \"help\"")
                self.listAllSupportCommands()
            else:
                raise Exception ("Our program does not support this command")
        except Exception as e:
            print("Error:", e)