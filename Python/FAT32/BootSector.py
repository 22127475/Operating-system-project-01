class BootSector:
    def __init__ (self, data: bytes):
        self.rawData = data
        self.jump_code = None
        self.OEM_ID = None
        self.bytesPerSector = None
        self.sectorsPerCluster = None
        self.reservedSectors = None
        self.numFATCopies = None
        self.numRDET_Entries = None
        self.sectorsPerVolume = None
        self.mediaDescriptor = None
        self.sectorsPerFAT = None
        self.sectorsPerTrack = None
        self.numHeads = None
        self.numHiddenSectors = None
        self.volumeSize = None
        self.FAT_Size = None
        self.flags = None
        self.FAT32_Version = None
        self.RDET_startingCluster = None
        self.fileSystemInformationSector = None
        self.BackupBootSector = None
        self.physicalDiskNumber = None
        self.bootSignature = None
        self.seriesNumber = None
        self.volumeLabel = None
        self.FAT_Type = None
        
        self.readBootSector()
        
    def readBootSector (self):
        #!Read all the information from the boot sector
        self.jump_code = hex(int.from_bytes(self.rawData[0:3], byteorder = 'little'))
        self.OEM_ID = str(self.rawData[3:11].strip().decode())   
        self.bytesPerSector = int.from_bytes(self.rawData[11:13], byteorder = 'little')
        self.sectorsPerCluster = int.from_bytes(self.rawData[13:14], byteorder = 'little')
        self.reservedSectors = int.from_bytes(self.rawData[14:16], byteorder = 'little')
        self.numFATCopies = int.from_bytes(self.rawData[16:17], byteorder = 'little')
        self.numRDET_Entries = int.from_bytes(self.rawData[17:19], byteorder = 'little')
        self.sectorsPerVolume = int.from_bytes(self.rawData[19:21], byteorder = 'little')
        self.mediaDescriptor = hex(int.from_bytes(self.rawData[21:22], byteorder = 'little'))
        self.sectorsPerFAT = int.from_bytes(self.rawData[22:24], byteorder = 'little')
        self.sectorsPerTrack = int.from_bytes(self.rawData[24:26], byteorder = 'little')
        self.numHeads = int.from_bytes(self.rawData[26:28], byteorder = 'little')
        self.numHiddenSectors = int.from_bytes(self.rawData[28:32], byteorder = 'little')
        self.volumeSize = int.from_bytes(self.rawData[32:36], byteorder = 'little')
        self.FAT_Size = int.from_bytes(self.rawData[36:40], byteorder = 'little')
        self.flags = int.from_bytes(self.rawData[40:42], byteorder = 'little')
        self.FAT32_Version = int.from_bytes(self.rawData[42:44], byteorder = 'little')
        self.RDET_startingCluster = int.from_bytes(self.rawData[44:48], byteorder = 'little')
        self.fileSystemInformationSector = int.from_bytes(self.rawData[48:50], byteorder = 'little')
        self.BackupBootSector = int.from_bytes(self.rawData[50:52], byteorder = 'little')
        self.physicalDiskNumber = hex(int.from_bytes(self.rawData[64:65], byteorder = 'little'))
        self.bootSignature = hex(int.from_bytes(self.rawData[66:67], byteorder = 'little'))
        self.seriesNumber = hex(int.from_bytes(self.rawData[67:71], byteorder = 'little'))
        self.volumeLabel = int.from_bytes(self.rawData[71:82], byteorder = 'little')
        self.FAT_Type = str(self.rawData[82:90].strip().decode())
        
    def showInfoOfBootSector (self):
        print ("Jump Code:", self.jump_code)
        print ("OEM ID:", self.OEM_ID)
        print ("Bytes per sector:", self.bytesPerSector)
        print ("Sectors per cluster:", self.sectorsPerCluster)
        print ("Reversed sectors:", self.reservedSectors)
        print ("Number of FAT copies:", self.numFATCopies)
        print ("Number of RDET entries:", self.numRDET_Entries)
        print ("Sectors per volume:", self.sectorsPerVolume)
        print ("Media descriptor:", self.mediaDescriptor)
        print ("Sectors per FAT:", self.sectorsPerFAT)
        print ("Sectors per track:", self.sectorsPerTrack)
        print ("Number of heads:", self.numHeads)
        print ("Number of hidden sectors:", self.numHiddenSectors)
        print ("Volume size:", self.volumeSize)
        print ("FAT size:", self.FAT_Size)
        print ("Flags:", self.flags)
        print ("FAT32 version:", self.FAT32_Version)
        print ("RDET starting cluster:", self.RDET_startingCluster)
        print ("File system information sector:", self.fileSystemInformationSector)
        print ("Backup boot sector:", self.BackupBootSector)
        print ("Physical disk number:", self.physicalDiskNumber)
        print ("Boot signature:", self.bootSignature)
        print ("Series number:", self.seriesNumber)
        print ("Volume label:", self.volumeLabel)
        print ("FAT type:", self.FAT_Type)
        
    def getFirstSectorOfFAT1 (self):
        return self.reservedSectors
    
    def getFirstSectorOfRDET (self):
        return self.reservedSectors + self.numFATCopies * self.FAT_Size