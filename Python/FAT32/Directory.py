from FAT32.Entry import Entry
from FAT32.FAT_Table import FATTable
from FAT32.constant import *

class Directory:
    """
    In FAT32, RDET locates in the data region
    """
    def __init__ (self, volume_name: str, path: str, data: bytes, FAT_Table: FATTable, DET_Dict: dict):
        self.volumeName = volume_name
        self.path = path
        self.rawData = data
        self.listEntries: list[Entry] = []
        self.FAT_table = FAT_Table
        
        self.DET_dict = DET_Dict

        self.is_root = True
        for i in range(0, len(self.rawData), 32):
            self.listEntries.append(Entry(self.volumeName, self.rawData[i:(i + 32)], FAT_Table))
            
        if (len(self.listEntries) >= 2 and self.listEntries[0].name.strip() in (".", "..") and self.listEntries[1].name.strip() in (".", "..")):
            self.listEntries = self.listEntries[2:]
            self.is_root = False
            
        self.formatEntries()
        self.directoryTree = self.readDirectoryTree()
            
    def formatEntries (self):
        index = 0
        
        while (index < len(self.listEntries)):
            while (index < len(self.listEntries) and (self.listEntries[index].status == DELETED or self.listEntries[index].status == EMPTY)):
                index = index + 1
                
            if (index == len(self.listEntries)):
                break
                
            tempListSubEntries = []
            while (index < len(self.listEntries) and self.listEntries[index].is_subentry == True):
                tempListSubEntries.append(self.listEntries[index])
                index = index + 1
            
            if (index == len(self.listEntries)):
                break
                
            self.listEntries[index].listSubEntries = tempListSubEntries.copy()
            self.listEntries[index].setFullName()
            
            index = index + 1
            
    def readDirectoryTree (self) -> list[Entry]:
        directory_tree = []
        
        for entry in self.listEntries:
            if (entry.status != EMPTY and entry.status != DELETED and entry.is_subentry == False):
                if (entry.attribute == DIRECTORY):
                    SDET_rawData = entry.readSDETData()
                    SDET_path = self.path
                    if (self.path == f"{self.volumeName}:\\"):
                        SDET_path += entry.name
                    else:
                        SDET_path += "\\" + entry.name
                    
                    entry.SDET = Directory(self.volumeName, SDET_path, SDET_rawData, self.FAT_table, self.DET_dict)
                    self.DET_dict[entry.firstCluster] = entry.SDET
                
                directory_tree.append(entry)
            
        return directory_tree
    
    def drawDirectoryTree (self):
        if (self.is_root == True):
            print(f"{self.volumeName}:")
        else:
            print(f"{self.volumeName}:.")
            
        def draw (directory: Directory, prefix = ""):
            for i in range (0, len(directory.directoryTree)):
                if (directory.directoryTree[i].attribute == ARCHIVE or directory.directoryTree[i].attribute == DIRECTORY):
                    print(prefix + ("└── " if i == len(directory.directoryTree) - 1 else "├── ") + directory.directoryTree[i].name)
                
                if (directory.directoryTree[i].attribute == ARCHIVE):
                    continue
                
                if (directory.directoryTree[i].attribute == DIRECTORY):
                    prefix_char = "    " if i == len(directory.directoryTree) - 1 else "│   "
                    draw(directory.directoryTree[i].SDET, prefix + prefix_char)
                    
        draw(self)
        
        