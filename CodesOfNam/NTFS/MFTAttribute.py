"""
   Reference: 
   https://www.dubeyko.com/development/FileSystems/NTFS/ntfsdoc.pdf  (1)
   https://stackoverflow.com/questions/42777907/understanding-the-attribute-list-in-ntfs (2)
   https://learn.microsoft.com/en-us/windows/win32/devnotes/attribute-list-entry (3)
"""
from datetime import datetime, timedelta
from NTFS.constant import *

def convert_ntfs_time_to_utc(ntfs_time):
    # NTFS epoch starts from January 1, 1601
    ntfs_epoch_start = datetime(1601, 1, 1)

    # Convert hundreds of nanoseconds to seconds
    seconds = ntfs_time / 1e7

    # Create a timedelta object and add it to the NTFS epoch start time
    ntfs_datetime_utc = ntfs_epoch_start + timedelta(seconds=seconds)

    return ntfs_datetime_utc

class DataRun:
    """
    Non-resident attributes are stored in intervals of clusters called runs. 
    Each run is represented by its starting cluster and its length. 
    The starting cluster of a run is coded as an offset to the starting cluster of the previous run
    """
    def __init__ (self, data: bytes, clusterCount_numBytes, offset_numBytes):
        self.rawData = data
        
        #? Components
        """
        These components were combined through the part "DataRun layout" of (1) and 
        the analysis of a non-resident file in Active@Disk Editor
        Besides, we also have some other references related to Data Runs
        https://www.youtube.com/watch?v=6WFUM5eViIk
        https://www.file-recovery.com/recovery-define-clusters-chain-ntfs.htm
        """
        self.clusterCount_numBytes = clusterCount_numBytes
        self.clusterCount = None
        self.offset_numBytes = offset_numBytes
        self.offsetToFirstCluster = None
        
        self.readDataRun ()
        
    def readDataRun (self):
        self.clusterCount = int.from_bytes(self.rawData[1: 1 + self.clusterCount_numBytes], byteorder = 'little')
        self.offsetToFirstCluster = int.from_bytes(self.rawData[1 + self.clusterCount_numBytes: 1 + self.clusterCount_numBytes + self.offset_numBytes], byteorder = 'little')

class MFTAttributeHeader:
    def __init__ (self, data: bytes):
        self.rawData = data
        self.typeID = None
        self.length = None
        self.residentFlag = None
        self.nameLength = None
        self.offsetToName = None
        self.flags = []
        self.attributeID = None
        
        self.readMFTAttributeHeader()
        
    def readMFTAttributeHeader (self):
        self.typeID = int.from_bytes(self.rawData[0:4], byteorder = 'little')
        self.length = int.from_bytes(self.rawData[4:8], byteorder = 'little')
        self.residentFlag = int.from_bytes(self.rawData[8:9], byteorder = 'little')
        self.nameLength = int.from_bytes(self.rawData[9:10], byteorder = 'little')
        self.offsetToName = int.from_bytes(self.rawData[10:12], byteorder = 'little')
        flags = int.from_bytes(self.rawData[12:14], byteorder = 'little')
        self.attributeID = int.from_bytes(self.rawData[14:16], byteorder = 'little')
        
        if (flags & ATTRIBUTE_IS_COMPRESSED):
            self.flags.append(ATTRIBUTE_IS_COMPRESSED)
        if (flags & ATTRIBUTE_IS_ENCRYPTED):
            self.flags.append(ATTRIBUTE_IS_ENCRYPTED)
        if (flags & ATTRIBUTE_IS_SPARSE):
            self.flags.append(ATTRIBUTE_IS_SPARSE)
    
class MFTAttribute:
    def __init__ (self, data: bytes):
        self.rawData = data
        self.header: MFTAttributeHeader = None
        self.dataLength = None
        self.offsetToData = None
        self.indexedFlag = None
        self.padding = None
        
        #? If header.nameLength > 0
        self.name = None
        
        #? If header.residentFlag != 0 (--> non-resident attribute)
        self.startingVCN = None
        self.lastVCN = None
        self.compressionUnitSize = None
        self.allocatedSize = None
        self.realSize = None
        self.initializedStreamDataSize = None
        self.listDataRuns: list[DataRun] = []
        
        self.readMFTAttribute()
        
    def readMFTAttribute (self):
        self.header = MFTAttributeHeader(self.rawData[0:16])
        
        #? Case : Resident attribute
        if (self.header.residentFlag == 0):
            self.dataLength = int.from_bytes(self.rawData[16:20], byteorder = 'little')
            self.offsetToData = int.from_bytes(self.rawData[20:22], byteorder = 'little')
            self.indexedFlag = int.from_bytes(self.rawData[22:23], byteorder = 'little')
            self.padding = int.from_bytes(self.rawData[23:24], byteorder = 'little')
            if (self.header.nameLength > 0):
                self.name = self.rawData[24: (24 + 2 * self.header.nameLength)].decode('utf-16le').strip('\x00')

        #? Case : Non-resident attribute
        else:
            #? VCN: Virtual CLuster Number
            self.startingVCN = int.from_bytes(self.rawData[16:24], byteorder = 'little')
            self.lastVCN = int.from_bytes(self.rawData[24:32], byteorder = 'little')
            self.offsetToData = int.from_bytes(self.rawData[32:34], byteorder = 'little')
            self.compressionUnitSize = int.from_bytes(self.rawData[34:36], byteorder = 'little')
            self.padding = int.from_bytes(self.rawData[36:40], byteorder = 'little')
            self.allocatedSize = int.from_bytes(self.rawData[40:48], byteorder = 'little')
            self.realSize = int.from_bytes(self.rawData[48:56], byteorder = 'little')
            self.initializedStreamDataSize = int.from_bytes(self.rawData[56:64], byteorder = 'little')
            if (self.header.nameLength > 0):
                self.name = self.rawData[64: (64 + 2 * self.header.nameLength)].decode('utf-16le').strip('\x00') 
            
            #? Read Data Run List
            length = 0
            while (length < self.header.length - self.offsetToData):
                dataRun_offset = self.offsetToData + length
                dataRun_size = self.rawData[dataRun_offset]
                
                offset_numBytes = (dataRun_size & 0b11110000) >> 4
                clusterCount_numBytes = dataRun_size & 0b00001111
                
                if (offset_numBytes == 0 or clusterCount_numBytes == 0):
                    break
                
                self.listDataRuns.append(DataRun(self.rawData[dataRun_offset: (dataRun_offset + 1 + clusterCount_numBytes + offset_numBytes)], clusterCount_numBytes, offset_numBytes))
                length = length + 1 + clusterCount_numBytes + offset_numBytes
                
class MFTAttribute_STANDARD_INFORMATION (MFTAttribute):
    def __init__(self, data: bytes):
        MFTAttribute.__init__(self, data)
        
        self.creationTime = None
        self.alteredTime = None
        self.MFT_changedTime = None
        self.readTime = None
        self.DOSfilePermission = []
        self.maxNumVersions = None
        self.versionNumber = None
        self.classID = None
        
        #? From Window 2000 and later
        self.ownerID = None
        self.securityID = None
        self.quotaCharged = None
        self.updateSequenceNumber = None
        
        self.read_STANDARD_INFORMATION()
        
    def read_STANDARD_INFORMATION (self):
        try:
            if (self.header.residentFlag != 0):
                raise ("Our program does not support non-resident STANDARD_INFORMATION attribute")
            else:
                self.creationTime = int.from_bytes(self.rawData[self.offsetToData: (self.offsetToData + 8)], byteorder = 'little')
                self.alteredTime = int.from_bytes(self.rawData[(self.offsetToData + 8): (self.offsetToData + 16)], byteorder = 'little')
                self.MFT_changedTime = int.from_bytes(self.rawData[(self.offsetToData + 16): (self.offsetToData + 24)], byteorder = 'little')
                self.readTime = int.from_bytes(self.rawData[(self.offsetToData + 24): (self.offsetToData + 32)], byteorder = 'little')
                DOSfilePermission = int.from_bytes(self.rawData[(self.offsetToData + 32): (self.offsetToData + 36)], byteorder = 'little')
                self.maxNumVersions = int.from_bytes(self.rawData[(self.offsetToData + 36): (self.offsetToData + 40)], byteorder = 'little')
                self.versionNumber = int.from_bytes(self.rawData[(self.offsetToData + 40): (self.offsetToData + 44)], byteorder = 'little')
                self.classID = int.from_bytes(self.rawData[(self.offsetToData + 44): (self.offsetToData + 48)], byteorder = 'little')
                if (self.dataLength > 48):
                    self.ownerID = int.from_bytes(self.rawData[(self.offsetToData + 48): (self.offsetToData + 52)], byteorder = 'little')
                    self.securityID = int.from_bytes(self.rawData[(self.offsetToData + 52): (self.offsetToData + 56)], byteorder = 'little')
                    self.quotaCharged = int.from_bytes(self.rawData[(self.offsetToData + 56): (self.offsetToData + 64)], byteorder = 'little')
                    self.updateSequenceNumber = int.from_bytes(self.rawData[(self.offsetToData + 64): (self.offsetToData + 72)], byteorder = 'little')
                    
                self.creationTime = convert_ntfs_time_to_utc(self.creationTime).strftime('%A, %B %d, %Y %I:%M:%S%p UTC')
                self.alteredTime = convert_ntfs_time_to_utc(self.alteredTime).strftime('%A, %B %d, %Y %I:%M:%S%p UTC')
                self.MFT_changedTime = convert_ntfs_time_to_utc(self.MFT_changedTime).strftime('%A, %B %d, %Y %I:%M:%S%p UTC')
                self.readTime = convert_ntfs_time_to_utc(self.readTime).strftime('%A, %B %d, %Y %I:%M:%S%p UTC')
                
                if (DOSfilePermission & READ_ONLY):
                    self.DOSfilePermission.append(READ_ONLY)
                if (DOSfilePermission & HIDDEN):
                    self.DOSfilePermission.append(HIDDEN)
                if (DOSfilePermission & SYSTEM):
                    self.DOSfilePermission.append(SYSTEM)
                if (DOSfilePermission & ARCHIVE):
                    self.DOSfilePermission.append(ARCHIVE)
                if (DOSfilePermission & DEVICE):
                    self.DOSfilePermission.append(DEVICE)
                if (DOSfilePermission & NORMAL):
                    self.DOSfilePermission.append(NORMAL)
                if (DOSfilePermission & TEMPORARY):
                    self.DOSfilePermission.append(TEMPORARY)
                if (DOSfilePermission & SPARSE_FILE):
                    self.DOSfilePermission.append(SPARSE_FILE)
                if (DOSfilePermission & REPARSE_POINT):
                    self.DOSfilePermission.append(REPARSE_POINT)
                if (DOSfilePermission & COMPRESSED):
                    self.DOSfilePermission.append(COMPRESSED)
                if (DOSfilePermission & OFFLINE):
                    self.DOSfilePermission.append(OFFLINE)
                if (DOSfilePermission & NOT_CONTENT_INDEXED):
                    self.DOSfilePermission.append(NOT_CONTENT_INDEXED)
                if (DOSfilePermission & ENCRYPTED):
                    self.DOSfilePermission.append(ENCRYPTED)
                    
        except Exception as e:
            print(e)
            exit()
    
class MFTAttribute_ATTRIBUTE_LIST (MFTAttribute):
    """
    When there are lots of attributes and space in the MFT record is short, all those attributes that can be
    made non-resident are moved out of the MFT. If there is still not enough room, then an
    $ATTRIBUTE_LIST attribute is needed. The remaining attributes are placed in a new MFT record and
    the $ATTRIBUTE_LIST describes where to find them. It is very unusual to see this attribute.
    """
    def __init__(self, data: bytes):
        MFTAttribute.__init__(self, data)

        # #? This is the information about the attributes that are moved out of the MFT record
        # self.typeID = None
        # self.length = None
    
class MFTAttribute_FILE_NAME (MFTAttribute):
    def __init__(self, data: bytes):
        MFTAttribute.__init__(self, data)
        
        """
        These two informations have some difference from the information in the Internet or in some documentations
        This need to be changed because we clarified on Active@Disk Editor
        """
        self.parentDirectory_fileRecordNumber = None
        self.parentDirectory_SequenceNumber = None
        
        self.creationTime = None
        self.alteredTime = None
        self.MFT_changedTime = None
        self.readTime = None
        self.allocatedSize = None
        self.realSize = None
        self.flags = []
        self.EAs_Reparse_value = None
        self.fileNameLength = None
        self.namespace = None
        self.fileName = None
        
        self.read_FILE_NAME()
        
    def read_FILE_NAME(self):
        try:
            if (self.header.residentFlag != 0):
                raise ("Our program does not support non-resident FILE_NAME attribute")
            else:
                self.parentDirectory_fileRecordNumber = int.from_bytes(self.rawData[self.offsetToData: (self.offsetToData + 6)], byteorder = 'little')
                self.parentDirectory_SequenceNumber = int.from_bytes(self.rawData[(self.offsetToData + 6): (self.offsetToData + 8)], byteorder = 'little')
                self.creationTime = int.from_bytes(self.rawData[(self.offsetToData + 8): (self.offsetToData + 16)], byteorder = 'little')
                self.alteredTime = int.from_bytes(self.rawData[(self.offsetToData + 16): (self.offsetToData + 24)], byteorder = 'little')
                self.MFT_changedTime = int.from_bytes(self.rawData[(self.offsetToData + 24): (self.offsetToData + 32)], byteorder = 'little')
                self.readTime = int.from_bytes(self.rawData[(self.offsetToData + 32): (self.offsetToData + 40)], byteorder = 'little')
                self.allocatedSize = int.from_bytes(self.rawData[(self.offsetToData + 40): (self.offsetToData + 48)], byteorder = 'little')
                self.realSize = int.from_bytes(self.rawData[(self.offsetToData + 48): (self.offsetToData + 56)], byteorder = 'little')
                flags = int.from_bytes(self.rawData[(self.offsetToData + 56): (self.offsetToData + 60)], byteorder = 'little')
                self.EAs_Reparse_value = int.from_bytes(self.rawData[(self.offsetToData + 60): (self.offsetToData + 64)], byteorder = 'little')
                self.fileNameLength = int.from_bytes(self.rawData[(self.offsetToData + 64): (self.offsetToData + 65)], byteorder = 'little')
                self.namespace = int.from_bytes(self.rawData[(self.offsetToData + 65): (self.offsetToData + 66)], byteorder = 'little')
                self.fileName = self.rawData[(self.offsetToData + 66): (self.offsetToData + 66 + 2 * self.fileNameLength)].decode('utf-16le').strip('\x00')
                
                self.creationTime = convert_ntfs_time_to_utc(self.creationTime).strftime('%A, %B %d, %Y %I:%M:%S%p UTC')
                self.alteredTime = convert_ntfs_time_to_utc(self.alteredTime).strftime('%A, %B %d, %Y %I:%M:%S%p UTC') 
                self.MFT_changedTime = convert_ntfs_time_to_utc(self.MFT_changedTime).strftime('%A, %B %d, %Y %I:%M:%S%p UTC')
                self.readTime = convert_ntfs_time_to_utc(self.readTime).strftime('%A, %B %d, %Y %I:%M:%S%p UTC')
                
                if (flags & READ_ONLY):
                    self.flags.append(READ_ONLY)
                if (flags & HIDDEN):
                    self.flags.append(HIDDEN)
                if (flags & SYSTEM):
                    self.flags.append(SYSTEM)
                if (flags & ARCHIVE):
                    self.flags.append(ARCHIVE)
                if (flags & DEVICE):
                    self.flags.append(DEVICE)
                if (flags & NORMAL):
                    self.flags.append(NORMAL)
                if (flags & TEMPORARY):
                    self.flags.append(TEMPORARY)
                if (flags & SPARSE_FILE):
                    self.flags.append(SPARSE_FILE)
                if (flags & REPARSE_POINT):
                    self.flags.append(REPARSE_POINT)
                if (flags & COMPRESSED):
                    self.flags.append(COMPRESSED)
                if (flags & OFFLINE):
                    self.flags.append(OFFLINE)
                if (flags & NOT_CONTENT_INDEXED):
                    self.flags.append(NOT_CONTENT_INDEXED)
                if (flags & ENCRYPTED):
                    self.flags.append(ENCRYPTED)
                if (flags & DIRECTORY):
                    self.flags.append(DIRECTORY)
                if (flags & INDEX_VIEW):
                    self.flags.append(INDEX_VIEW)
        except Exception as e:
            print(e)
            exit()
    
class MFTAttribute_OBJECT_ID (MFTAttribute):
    def __init__(self, data: bytes):
        MFTAttribute.__init__(self, data)
    
class MFTAttribute_SECURITY_DESCRIPTOR (MFTAttribute):
    def __init__(self, data: bytes):
        MFTAttribute.__init__(self, data)
    
class MFTAttribute_VOLUME_NAME (MFTAttribute):
    def __init__(self, data: bytes):
        MFTAttribute.__init__(self, data)
    
class MFTAttribute_VOLUME_INFORMATION (MFTAttribute):
    def __init__(self, data: bytes):
        MFTAttribute.__init__(self, data)
    
class MFTAttribute_DATA (MFTAttribute):
    def __init__(self, data: bytes):
        MFTAttribute.__init__(self, data)

class MFTAttribute_INDEX_ROOT (MFTAttribute):
    def __init__(self, data: bytes):
        MFTAttribute.__init__(self, data)
        
class MFTAttribute_INDEX_ALLOCATION (MFTAttribute):
    def __init__(self, data: bytes):
        MFTAttribute.__init__(self, data)
        
class MFTAttribute_BITMAP (MFTAttribute):
    def __init__(self, data: bytes):
        MFTAttribute.__init__(self, data)
        
class MFTAttribute_REPARSE_POINT (MFTAttribute):
    def __init__(self, data: bytes):
        MFTAttribute.__init__(self, data)
        
class MFTAttribute_EA_INFORMATION (MFTAttribute):
    def __init__(self, data: bytes):
        MFTAttribute.__init__(self, data)
    
class MFTAttribute_EA (MFTAttribute):
    def __init__(self, data: bytes):
        MFTAttribute.__init__(self, data)
        
class MFTAttribute_LOGGED_UTILITY_STREAM (MFTAttribute):
    def __init__(self, data: bytes):
        MFTAttribute.__init__(self, data)
        
def identifyMFTAttribute(data: bytes) -> MFTAttribute:
    typeID = int.from_bytes(data[0:4], byteorder = 'little')
    if (typeID == MFT_STANDARD_INFORMATION):
        return MFTAttribute_STANDARD_INFORMATION(data)
    elif (typeID == MFT_ATTRIBUTE_LIST):
        return MFTAttribute_ATTRIBUTE_LIST(data)
    elif (typeID == MFT_FILE_NAME):
        return MFTAttribute_FILE_NAME(data)
    elif (typeID == MFT_OBJECT_ID):
        return MFTAttribute_OBJECT_ID(data)
    elif (typeID == MFT_SECURITY_DESCRIPTOR):
        return MFTAttribute_SECURITY_DESCRIPTOR(data)
    elif (typeID == MFT_VOLUME_NAME):
        return MFTAttribute_VOLUME_NAME(data)
    elif (typeID == MFT_VOLUME_INFORMATION):
        return MFTAttribute_VOLUME_INFORMATION(data)
    elif (typeID == MFT_DATA):
        return MFTAttribute_DATA(data)
    elif (typeID == MFT_INDEX_ROOT):
        return MFTAttribute_INDEX_ROOT(data)
    elif (typeID == MFT_INDEX_ALLOCATION):
        return MFTAttribute_INDEX_ALLOCATION(data)
    elif (typeID == MFT_BITMAP):
        return MFTAttribute_BITMAP(data)
    elif (typeID == MFT_REPARSE_POINT):
        return MFTAttribute_REPARSE_POINT(data)
    elif (typeID == MFT_EA_INFORMATION):
        return MFTAttribute_EA_INFORMATION(data)
    elif (typeID == MFT_EA):
        return MFTAttribute_EA(data)
    elif (typeID == MFT_LOGGED_UTILITY_STREAM):
        return MFTAttribute_LOGGED_UTILITY_STREAM(data)
    else:
        return None
        