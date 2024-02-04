from NTFS.Directory import *

def checkNTFS (volumeName: str):
    try:
        volumePath = r"\\." + f"\{volumeName}:"
        with open (volumePath, 'rb') as f:
            f.read(1) #? For avoid ERROR INVALID ARGUMENT, we can change 1 by any value
            f.seek(0x03)
            OEM_ID = f.read(8)
            
            if (OEM_ID == b"NTFS    "):
                return True
            return False
    except Exception as e:
        print ("Error:", e)
        exit()

class NTFS:
    def __init__ (self, volume_name):
        self.volumeName = volume_name
        self.path = f"{self.volumeName}:\\"
        
        #? Components
        self.volumeBootRecord: VolumeBootRecord = None
        self.MFT: list[MFTEntry] = []
        self.rootDirectory = None
        
        self.readComponents()
        
    def readComponents (self):
        try:
            volumePath = r'\\.' + f'\{self.volumeName}:'
            with open (volumePath, 'rb') as f:
                #! Read VBR
                #? Because at this time, the parameter bytePerSector is not defined, so we use the value 512 as the common number of bytes in a sector
                VBR_rawData = f.read(512) 
                self.volumeBootRecord = VolumeBootRecord(VBR_rawData)
                
                #! Read MFT
                #? From the first bytes (the second parameter is 0), we seek the first cluster of Master File Table (MFT)
                f.seek(self.volumeBootRecord.BPB.MFTStartingCluster * self.volumeBootRecord.BPB.sectorsPerCluster * self.volumeBootRecord.BPB.bytesPerSector, 0)
                #? Get $MFT File, which is the first MFT Entry
                first_MFTEntry_rawData = f.read(self.volumeBootRecord.BPB.MFTEntrySize)
                first_MFTEntry = MFTEntry(first_MFTEntry_rawData, self.volumeName, self.path, self.volumeBootRecord)
                MFT_numCLusters = None
                """ 
                Get the information of MFT from $MFT, this amount of information is not found at any documentations
                We found from analyzing the structure of $MFT we read C disk on Active@Disk Editor
                """
                #? We will traverse each attribute in $MFT and find $DATA attribute
                """
                In reality, $MFT include only 4 attributes, which are $STANDARD_INFORMATION, $FILE_NAME, $DATA and $BITMAP
                We can get $DATA attribute by calling the third attribute in the attribute list of this MFT entry
                But for sure, we will traverse and compare the type ID
                """
                for i in range (0, len(first_MFTEntry.attributes)):
                    if (first_MFTEntry.attributes[i].header.typeID == MFT_DATA):
                        """
                        We care two parameters: starting VCN and last VCN
                        In $MFT, starting VCN will be 0, so last VCN + 1 is the number of clusters in MFT
                        """
                        try:
                            if (first_MFTEntry.attributes[i].lastVCN is not None):
                                MFT_numCLusters = first_MFTEntry.attributes[i].lastVCN + 1
                            else:
                                raise ("Your disk is having some problems")
                        except Exception as e:
                            print(e)
                            exit()
                        
                #? So, the size in bytes of MFT is MFT_numClusters * SectorsPerCluster * BytesPerSector
                f.seek(self.volumeBootRecord.BPB.MFTStartingCluster * self.volumeBootRecord.BPB.sectorsPerCluster * self.volumeBootRecord.BPB.bytesPerSector, 0)
                MFT_rawData = f.read(MFT_numCLusters * self.volumeBootRecord.BPB.sectorsPerCluster * self.volumeBootRecord.BPB.bytesPerSector)

                #? Read all entries in MFT by getting each (MFTEntrysize) bytes
                for i in range (0, len(MFT_rawData), self.volumeBootRecord.BPB.MFTEntrySize):
                    if (MFT_rawData[i: i + 4] == b"FILE"):
                        self.MFT.append(MFTEntry(MFT_rawData[i: (i + self.volumeBootRecord.BPB.MFTEntrySize)], self.volumeName, self.path, self.volumeBootRecord))
                        
                #! Build root directory
                self.rootDirectory = RootDirectory(self.MFT)
                
        except Exception as e:
            print ("Error:", e)
            exit()
            
    def findDirectory (self) -> MFTEntry:
        MFT_entry: MFTEntry = None

        for entry in self.rootDirectory.directoryDict.values():
            if (entry.path == self.path):
                MFT_entry = entry
                break
            
        return MFT_entry
    
    def findEntryInDirectory (self, Det: MFTEntry, nameEntry: str) -> MFTEntry:
        try:
            if (Det.isUsedDirectory()):
                dir_Entry: MFTEntry = None
                for entry in Det.children:
                    if (entry.fileName.lower() == nameEntry.lower()):
                            dir_Entry = entry
                            break
                        
                return dir_Entry
            else:
                raise Exception ("You are passing a file, not a directory")
        except Exception as e:
            print(e)
        
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
                MFT_Entry: MFTEntry = self.findEntryInDirectory(Det, nameEntry = listArgs[0])
                if (MFT_Entry is None):
                    raise Exception (f"Not found {listArgs[0]}")
                elif (MFT_Entry.isUsedFile()):
                    raise Exception (f"{listArgs[0]} is not a directory")
                
                if (self.path == f"{self.volumeName}:\\"):
                    self.path += MFT_Entry.fileName
                else:
                    self.path += "\\" + MFT_Entry.fileName
            except Exception as e:
                print("Error:", e)
    
    def drawDirectoryTree (self):
        #! Find directory corresponding the current path
        MFT_Entry = self.findDirectory()
        #!Draw directory tree
        MFT_Entry.drawDirectoryTree()
        
    def readTextContext (self, listArgs: list[str]):
        #! Find directory corresponding the current path
        Det = self.findDirectory()
        
        for Arg in listArgs:
            fileContent = ""
            try:
                #! Find entry based on the given argument
                dir_Entry: MFTEntry = self.findEntryInDirectory(Det, Arg)
                if (dir_Entry is None):
                    raise Exception (f"Not found {Arg}")
                elif (dir_Entry.isUsedDirectory()):
                    raise Exception (f"{Arg} is not a file")
                
                fileContent = dir_Entry.fileContent.decode().strip('\x00')
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