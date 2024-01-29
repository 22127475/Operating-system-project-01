from FAT_Table import FATTable
import datetime
from itertools import chain
from constant import *

class Entry:
    def __init__ (self, volume_name: str, data: bytes, FAT_Table: FATTable):
        #? Raw data
        self.volumeName = volume_name #? Need for read data of SDET (if this entry is directory)
        self.rawData = data
        
        #? Infomation
        self.name = None
        self.extension = None
        self.attribute = None
        self.creationTime = None
        self.creationDate = None
        self.lastAccessDate = None
        self.firstCluster = None #? Including 2 bytes at high order and 2 bytes at low order
        self.listClusters = []
        self.listSectors = []
        self.lastModifiedTime = None
        self.lastModifiedDate = None
        self.fileSize = None
        
        #? Other
        self.status = None #? Empty, deleted or are including sth 
        self.is_subentry = False
        self.listSubEntries: list[Entry] = None
        self.SDET = None
        self.contentFile = None
        self.FAT_table = FAT_Table
        
        self.readEntry()
        
    def readEntry (self):
        #! Read information
        self.attribute = self.rawData[11:12]
        if (self.attribute == b'\x0f'):
            self.is_subentry = True
        else:
            self.attribute = int.from_bytes(self.attribute, byteorder = 'little')
        
        if (self.is_subentry == False):
            self.listSubEntries = []
            self.status = int.from_bytes(self.rawData[0:1], byteorder = 'little')
            if (self.status == EMPTY):
                self.name = ""
                self.extension = ""
                return
            
            if (self.status == DELETED):
                self.name = self.rawData[1:8].decode(encoding = 'utf-8')
            else:
                self.name = self.rawData[0:8].decode(encoding = 'utf-8')
                
            self.extension = self.rawData[8:11].decode(encoding = 'utf-8')
            
            if (self.attribute == VOLUME_LABEL):
                return
            
            self.creationTime = int.from_bytes(self.rawData[13:16], byteorder = 'little')
            #? Hour: 5 bits, Minute: 6 bits, Second: 6 bits, Miliseconds: 7 bits, 5 + 6 + 6 + 7 = 24(bits) = 3 bytes
            """
            From the above numbers of bits for each component of time, so:
            Assume that we have a chain of 24 bits called A:
            So, to get the first 5 bits for hour, we will get the result A & 111110..0
            The same idea will be implemented to get minute, second and milisecond
            """
            createdHour = (self.creationTime & 0b111110000000000000000000) >> 19
            createdMinute = (self.creationTime & 0b000001111110000000000000) >> 13
            createdSecond = (self.creationTime & 0b000000000001111110000000) >> 7
            createdMilisecond = self.creationTime & 0b000000000000000001111111
            self.creationTime = datetime.time(createdHour, createdMinute, createdSecond, createdMilisecond)
            
            self.creationDate = int.from_bytes(self.rawData[16:18], byteorder = 'little')
            #? Year: 7 bits (--> value then plus 1980 is the exact year), Month: 4 bits, Day: 5 bits
            createdYear = 1980 + ((self.creationDate & 0b1111111000000000) >> 9)
            createdMonth = (self.creationDate & 0b0000000111100000) >> 5
            createdDay = self.creationDate & 0b0000000000011111
            self.creationDate = datetime.date(createdYear, createdMonth, createdDay)
            
            self.lastAccessDate = int.from_bytes(self.rawData[18:20], byteorder = 'little')
            #? Year: 7 bits (--> value then plus 1980 is the exact year), Month: 4 bits, Day: 5 bits
            lastAccessYear = 1980 + ((self.lastAccessDate & 0b1111111000000000) >> 9)
            lastAccessMonth = (self.lastAccessDate & 0b0000000111100000) >> 5
            lastAccessDay = self.lastAccessDate & 0b0000000000011111
            self.lastAccessDate = datetime.date(lastAccessYear, lastAccessMonth, lastAccessDay)
            
            self.lastModifiedTime = int.from_bytes(self.rawData[22:24], byteorder = 'little')
            #? Hour: 5 bits, Minute: 6 bits, Second: 5 bits (--> value then multiply 2 is the exact second)
            lastModifiedHour = (self.lastModifiedTime & 0b1111100000000000) >> 11
            lastModifiedMinute = (self.lastModifiedTime & 0b0000011111100000) >> 5
            lastModifiedSecond = (self.lastModifiedTime & 0b0000000000011111) * 2
            self.lastModifiedTime = datetime.time(lastModifiedHour, lastModifiedMinute, lastModifiedSecond)
            
            self.lastModifiedDate = int.from_bytes(self.rawData[24:26], byteorder = 'little')
            #? Year: 7 bits (--> value then plus 1980 is the exact year), Month: 4 bits, Day: 5 bits
            lastModifiedYear = 1980 + ((self.lastModifiedDate & 0b1111111000000000) >> 9)
            lastModifiedMonth = (self.lastModifiedDate & 0b0000000111100000) >> 5
            lastModifiedDay = self.lastModifiedDate & 0b0000000000011111
            self.lastModifiedDate = datetime.date(lastModifiedYear, lastModifiedMonth, lastModifiedDay)
            
            firstCluster_HighOrder2Bytes = self.rawData[20:22][::-1]
            firstCluster_LowOrder2Bytes = self.rawData[26:28][::-1]
            self.firstCluster = int.from_bytes(firstCluster_HighOrder2Bytes + firstCluster_LowOrder2Bytes, byteorder = 'big')
            #? I have tested my program and realize that I should add the second condition
            if (self.status != DELETED and self.name.strip() not in (".", "..")):
                self.listClusters = self.FAT_table.getListCLusters(self.firstCluster)
                self.listSectors = self.FAT_table.getListSectors(self.firstCluster)
    
            self.fileSize = int.from_bytes(self.rawData[28:32], byteorder = 'little')
        else:
            self.entryOrder = self.rawData[0]
            self.status = int.from_bytes(self.rawData[0:1], byteorder = 'little')
            self.name = b""
            
            for i in chain(range(1, 11, 2), range(14, 26, 2), range(28, 32, 2)):
                if (self.rawData[i:(i + 2)] == b"\xff\xff"):
                    break
                self.name += self.rawData[i:(i + 2)]
            
            self.name = self.name.decode(encoding = 'utf-16le').strip('\x00') 
            #? Need strip('\x00') for comparing self.name with another string which was decoded in utf-8
            
    def setFullName (self):
        if (len(self.listSubEntries) != 0):
            self.name = ""
            self.extension = ""
            self.listSubEntries = self.listSubEntries[::-1]
            for i in range (0, len(self.listSubEntries)):
                self.name += self.listSubEntries[i].name
                
        if (self.extension != ""):
            self.name = self.name + '.' + self.extension
            
    def readSDETData (self):
        if (self.attribute == DIRECTORY):
            try:
                volumePath = r'\\.' + f'\{self.volumeName}:'
                with open (volumePath, 'rb') as f:
                    SDET_rawData = b""
                    for sector in self.listSectors:
                        f.seek(sector * 512, 0)
                        SDET_rawData += f.read(512)
                        
                    return SDET_rawData
            except Exception as e:
                print('Error:', e)
                exit()
                
    def getFileContent (self):
        if (self.attribute == ARCHIVE):
            try:
                volumePath = r'\\.' + f'\{self.volumeName}:'
                with open (volumePath, 'rb') as f:
                    Content_rawData = b""
                    for sector in self.listSectors:
                        f.seek(sector * 512, 0)
                        Content_rawData += f.read(512)
                        
                    return Content_rawData
            except Exception as e:
                print('Error:', e)
                exit()