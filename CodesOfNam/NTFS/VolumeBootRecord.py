"""
    Reference: 
    https://www.ntfs.com/ntfs-partition-boot-sector.htm
    https://legiacong.blogspot.com/2014/04/he-thong-quan-ly-tap-tin-ntfs-4-vbr-bpb.html
"""

class BIOS_ParameterBlock:
    def __init__ (self, data: bytes):
        #? Component
        self.rawData = data
        self.bytesPerSector = None
        self.sectorsPerCluster = None
        self.reservedSectors = None
        self.mediaDescriptor = None
        self.sectorsPerTrack = None
        self.numHeads = None
        self.startingSector = None
        self.totalSectors = None
        self.MFTStartingCluster = None
        self.MFTMirrorStartingCluster = None
        self.MFTEntrySize = None
        self.clusterPerIndexBuffer = None
        self.volumeSerialNumber = None
        self.checkSum = None
        
        self.readBPB()
    
    def readBPB (self):
        self.bytesPerSector = int.from_bytes(self.rawData[0:2], byteorder = 'little')
        self.sectorsPerCluster = int.from_bytes(self.rawData[2:3], byteorder = 'little')
        self.reservedSectors = int.from_bytes(self.rawData[3:5], byteorder = 'little')
        self.mediaDescriptor = hex(int.from_bytes(self.rawData[10:11], byteorder = 'little'))
        self.sectorsPerTrack = int.from_bytes(self.rawData[13:15], byteorder = 'little')
        self.numHeads = int.from_bytes(self.rawData[15:17], byteorder = 'little')
        self.startingSector = int.from_bytes(self.rawData[17:21], byteorder = 'little')
        self.totalSectors = int.from_bytes(self.rawData[29:37], byteorder = 'little')
        self.MFTStartingCluster = int.from_bytes(self.rawData[37:45], byteorder = 'little')
        self.MFTMirrorStartingCluster = int.from_bytes(self.rawData[45:53], byteorder = 'little')
        
        self.MFTEntrySize = int.from_bytes(self.rawData[53:54], byteorder = 'little', signed = True)
        self.MFTEntrySize = 2 ** abs(self.MFTEntrySize)
        
        self.clusterPerIndexBuffer = int.from_bytes(self.rawData[57:58], byteorder = 'little')
        self.volumeSerialNumber = hex(int.from_bytes(self.rawData[61:69], byteorder = 'little'))
        self.checkSum = int.from_bytes(self.rawData[69:73], byteorder = 'little')
    
    def showBPB (self, prefix = "", marker = ""):
        print(prefix + marker + " " + "Bytes per sector: " + str(self.bytesPerSector) + " bytes")
        print(prefix + marker + " " + "Sectors per cluster: " + str(self.sectorsPerCluster) + " sectors")
        print(prefix + marker + " " + "Reserved sectors: " + str(self.reservedSectors) + " sectors")
        print(prefix + marker + " " + "Media descriptor: " + str(self.mediaDescriptor))
        print(prefix + marker + " " + "Sectors per track: " + str(self.sectorsPerTrack) + " sectors")
        print(prefix + marker + " " + "Number of heads: " + str(self.numHeads) + " heads")
        print(prefix + marker + " " + "Starting sector: " + str(self.startingSector))
        print(prefix + marker + " " + "Total sectors: " + str(self.totalSectors) + " sectors")
        print(prefix + marker + " " + "MFT starting cluster: " + str(self.MFTStartingCluster))
        print(prefix + marker + " " + "MFT mirror starting cluster: " + str(self.MFTMirrorStartingCluster))
        print(prefix + marker + " " + "MFT entry size: " + str(self.MFTEntrySize) + " bytes")
        print(prefix + marker + " " + "Cluster per index buffer: " + str(self.clusterPerIndexBuffer) + " clusters")
        print(prefix + marker + " " + "Volume serial number: " + str(self.volumeSerialNumber))
        print(prefix + marker + " " + "Checksum: " + str(self.checkSum))

class VolumeBootRecord:
    def __init__(self, data: bytes):
        #? Component
        self.rawData = data
        self.jump_instruction = None
        self.OEM_ID = None
        self.BPB: BIOS_ParameterBlock = None
        self.bootstrapCode = None
        self.endOfSectorMarker = None
        
        self.readVBR()
        
    def readVBR (self):
        self.jump_instruction = hex(int.from_bytes(self.rawData[0:3], byteorder = 'little'))
        self.OEM_ID = str(self.rawData[3:11].decode(encoding = 'utf-8'))
        self.BPB = BIOS_ParameterBlock(self.rawData[11:84])
    
    def showVBR (self):
        print("Jump Instruction: " + self.jump_instruction)
        print("OEM ID: " + self.OEM_ID)
        print("BIOS Parameter Block: ")
        self.BPB.showBPB(prefix = "\t", marker = "+")