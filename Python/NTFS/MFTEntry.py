from NTFS.MFTAttribute import *
from NTFS.VolumeBootRecord import *

class MFTEntryHeader:
    def __init__ (self, data: bytes):
        self.rawData = data
        
        self.signature = None
        self.offsetToUpdateSequence = None
        self.UpdateSeq_FixedupArraySize = None
        self.logfileSequenceNumber = None
        self.sequenceNumber = None #? Number of reused attempts
        self.referenceCount = None #? Number of directories that refers to this file
        self.offsetToFirstAttribute = None
        self.flags = None
        self.realSize = None #? Number of bytes which was used in MFT Entry
        self.allocatedSize = None
        self.fileReferenceToBaseMFTRecord = None
        self.nextAttributeID = None
        self.entryID = None #? This information was found when we experience Active@Disk Editor
        
        self.readMFTEntryHeader()
    
    def readMFTEntryHeader (self):
        self.signature = str(self.rawData[0:4].decode(encoding = 'utf-8'))
        self.offsetToUpdateSequence = int.from_bytes(self.rawData[4:6], byteorder='little')
        self.UpdateSeq_FixedupArraySize = int.from_bytes(self.rawData[6:8], byteorder='little')
        self.logfileSequenceNumber = int.from_bytes(self.rawData[8:16], byteorder='little')
        self.sequenceNumber = int.from_bytes(self.rawData[16:18], byteorder='little')
        self.referenceCount = int.from_bytes(self.rawData[18:20], byteorder='little')
        self.offsetToFirstAttribute = int.from_bytes(self.rawData[20:22], byteorder='little')
        self.flags = int.from_bytes(self.rawData[22:24], byteorder='little')
        self.realSize = int.from_bytes(self.rawData[24:28], byteorder='little')
        self.allocatedSize = int.from_bytes(self.rawData[28:32], byteorder='little')
        self.fileReferenceToBaseMFTRecord = int.from_bytes(self.rawData[32:40], byteorder='little')
        self.nextAttributeID = int.from_bytes(self.rawData[40:42], byteorder='little')
        #? Entry ID will increase sequentially if we traverse sequentially all MFT entries in MFT
        self.entryID = int.from_bytes(self.rawData[44:48], byteorder = 'little')

class MFTEntry:
    def __init__ (self, data: bytes, volume_name: str, path: str, VBR: VolumeBootRecord):
        self.rawData = data
        self.volumeName = volume_name
        self.path = path
        self.volumeBootRecord = VBR
        
        self.header: MFTEntryHeader = None
        self.attributes: list[MFTAttribute] = []
        self.readMFTEntry()
        
        self.parentDirectory_fileRecordNumber = None
        self.getparentDirectory_fileRecordNumber()
        self.fileName = None
        self.getFileName()
        self.fileContent = None
        self.getFileContent()
        
        self.is_root = False
        self.isRootDirectory()
        self.children: list[MFTEntry] = []
    
    def readMFTEntry (self):
        self.header = MFTEntryHeader(self.rawData[0:48])
        self.readAttributes()
        
    def readAttributes (self):
        offset = self.header.offsetToFirstAttribute
        while offset < self.header.realSize:
            tempTypeID = int.from_bytes(self.rawData[offset: (offset + 4)], byteorder = 'little')
            if (tempTypeID == END_MARKER):
                break
            
            dataLength = int.from_bytes(self.rawData[(offset + 4): (offset + 8)], byteorder = 'little')
            self.attributes.append(identifyMFTAttribute(self.rawData[offset: (offset + dataLength)]))
            offset = offset + dataLength
            
    def isUsedFile (self):
        return (self.header.flags & IS_USED != 0) and ((self.header.flags & IS_DIRECTORY == 0))
    def isUsedDirectory (self):
        return (self.header.flags & IS_USED != 0) and ((self.header.flags & IS_DIRECTORY != 0))
    def isRootDirectory (self):
        if (self.header.entryID == 5):
            self.is_root = True
    
    def changePath (self, listParentDirectoryNames):
        tempPath = self.path
        
        for i in range (len(listParentDirectoryNames) - 1, -1, -1):
            if (tempPath == f"{self.volumeName}:\\"):
                tempPath += listParentDirectoryNames[i]
            else:
                tempPath += "\\" + listParentDirectoryNames[i]
                
        if (self.fileName != "."):
            if (tempPath == f"{self.volumeName}:\\"):
                tempPath += self.fileName
            else:
                tempPath += "\\" + self.fileName
        self.path = tempPath
    
    def getparentDirectory_fileRecordNumber (self):
        for i in range (0, len(self.attributes)):
            if (self.attributes[i].header.typeID == MFT_FILE_NAME):
                self.parentDirectory_fileRecordNumber = self.attributes[i].parentDirectory_fileRecordNumber
                break
    
    def getFileName (self):
        for i in range (0, len(self.attributes)):
            if (self.attributes[i].header.typeID == MFT_FILE_NAME):
                self.fileName = self.attributes[i].fileName
                break
    
    def getFileContent (self):
        for i in range (0, len(self.attributes)):
            if (self.attributes[i].header.typeID == MFT_DATA):
                if (self.attributes[i].header.residentFlag == 0):
                    self.fileContent = self.attributes[i].rawData[self.attributes[i].offsetToData: (self.attributes[i].offsetToData + self.attributes[i].dataLength)]
                else:
                    try:
                        volumePath = r'\\.' + f'\{self.volumeName}:'
                        with open (volumePath, 'rb') as f:
                            Content_rawData = b""
                            for j in range (len(self.attributes[i].listDataRuns)):
                                offsetToFirstCluster = self.attributes[i].listDataRuns[j].offsetToFirstCluster
                                clusterCount = self.attributes[i].listDataRuns[j].clusterCount
                                
                                f.read(1)
                                f.seek(offsetToFirstCluster * self.volumeBootRecord.BPB.sectorsPerCluster * self.volumeBootRecord.BPB.bytesPerSector, 0)
                                
                                attemptReadContent = 1
                                while (clusterCount * self.volumeBootRecord.BPB.sectorsPerCluster * self.volumeBootRecord.BPB.bytesPerSector >= attemptReadContent * 500000000):
                                    Content_rawData += f.read(500000000)
                                    attemptReadContent += 1
                                temp = int(clusterCount* self.volumeBootRecord.BPB.sectorsPerCluster * self.volumeBootRecord.BPB.bytesPerSector - (attemptReadContent - 1) * 500000000)
                                Content_rawData += f.read(temp)
                                
                            self.fileContent = Content_rawData
                    except Exception as e:
                        print('Error:', e)
                        exit()
                break
    
    def drawDirectoryTree (self):
        def get_STANDARD_INFORMATION_Attribute (MFT_entry: MFTEntry) -> MFTAttribute_STANDARD_INFORMATION:
            for i in range (0, len(MFT_entry.attributes)):
                if (MFT_entry.attributes[i].header.typeID == MFT_STANDARD_INFORMATION):
                    return MFT_entry.attributes[i]
            
            return None
        
        def draw (directory: MFTEntry, prefix = ""):
            for i in range (0, len(directory.children)):
                STANDARD_INFORMATION_Attribute = get_STANDARD_INFORMATION_Attribute(directory.children[i])
                if (HIDDEN not in STANDARD_INFORMATION_Attribute.DOSfilePermission and SYSTEM not in STANDARD_INFORMATION_Attribute.DOSfilePermission
                    and (directory.children[i].isUsedDirectory() or directory.children[i].isUsedFile())):
                    print(prefix + ("└── " if i == len(directory.children) - 1 else "├── ") + directory.children[i].fileName)
                
                    if (directory.children[i].isUsedFile()):
                        continue
                    
                    if (directory.children[i].isUsedDirectory()):
                        prefix_char = "    " if i == len(directory.children) - 1 else "│   "
                        draw(directory.children[i], prefix + prefix_char)
                    
        if (self.isUsedDirectory()):
            if (self.is_root == True):
                print(f"{self.volumeName}:")
            else:
                print(f"{self.volumeName}:.")
                
            draw(self)
    
    