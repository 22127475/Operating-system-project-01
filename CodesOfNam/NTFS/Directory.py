from NTFS.MFTEntry import *

class RootDirectory:
    def __init__ (self, listMFTEntries: list[MFTEntry]):
        self.listMFTEntries: list[MFTEntry] = listMFTEntries
        self.directoryDict: dict[int, MFTEntry] = dict()
        self.buildDirectoryDictionary()
        
        self.directoryTree: MFTEntry = self.directoryDict[5] #? 5 is the INode of .(root directory)
        
    def buildDirectoryDictionary (self):
        for i in range (0, len(self.listMFTEntries)):
            #? Entry ID will increase sequentially if we traverse sequentially all MFT entries in MFT
            self.directoryDict[self.listMFTEntries[i].header.entryID] = self.listMFTEntries[i]
        
        for key in self.directoryDict:
            parentDirectory_fileRecordNumber = self.directoryDict[key].parentDirectory_fileRecordNumber
            if parentDirectory_fileRecordNumber is not None and key != 5:
                self.directoryDict[parentDirectory_fileRecordNumber].children.append(self.directoryDict[key])
                
        for key in self.directoryDict:
            parentDirectory_fileRecordNumber = self.directoryDict[key].parentDirectory_fileRecordNumber
            if parentDirectory_fileRecordNumber is not None:
                listParentDirNames = self.getlistDirNames(parentDirectory_fileRecordNumber)
                self.directoryDict[key].changePath(listParentDirNames)
                
    def getlistDirNames (self, parentDirectory_fileRecordNumber) -> list[str]:
        tempParentDir_FRN = parentDirectory_fileRecordNumber
        listDirs = []
        
        while (tempParentDir_FRN != 5):
            try:
                if (self.directoryDict[tempParentDir_FRN].fileName is not None):
                    listDirs.append(self.directoryDict[tempParentDir_FRN].fileName)
                    tempParentDir_FRN = self.directoryDict[tempParentDir_FRN].parentDirectory_fileRecordNumber
                else:
                    raise Exception ("Impossible")
            except Exception as e:
                print(e)
                exit()
                
        return listDirs