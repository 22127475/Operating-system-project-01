from FAT32.BootSector import BootSector
# Special cluster's values
FREE = 0x00000000
BAD = 0x0FFFFFF7
EOF = 0x0FFFFFFF

class FATTable:
    def __init__ (self, data: bytes, bootSector: BootSector):
        self.rawData = data
        self._bootSector = bootSector
        self.listFATElements = []
        for i in range(0, len(self.rawData), 4):
            self.listFATElements.append(int.from_bytes(self.rawData[i:i + 4], byteorder='little'))
            
    def getListCLusters (self, starting_cluster):
        listClusters = []
        
        cluster = starting_cluster
        while True:
            listClusters.append(cluster)
            #! Get the next cluster which was pointed by this cluster
            cluster = self.listFATElements[cluster]
            
            try:        
                if (cluster == BAD):
                    raise Exception ("Bad Cluster Found")
            except Exception as e:
                print(e)
                exit()
                
            if (cluster == FREE or cluster == EOF):
                break
            
        return listClusters
    
    def getListSectors (self, starting_cluster):
        listClusters = self.getListCLusters(starting_cluster)
        listSectors = []
        
        for cluster in listClusters:
            firstSector = self._bootSector.getFirstSectorOfRDET() + (cluster - 2) * self._bootSector.sectorsPerCluster
            listSectors.append(firstSector)
            
            for i in range (1, self._bootSector.sectorsPerCluster):
                listSectors.append(firstSector + i)
            
        return listSectors
        
        
        