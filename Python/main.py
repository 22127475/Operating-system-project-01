from FAT32.FAT32 import *
from NTFS.NTFS import *
import os

def main ():
    listVolumeNames = []
    for volume_name in range (65, 91):
        if (os.path.exists(chr(volume_name) + ":")):
            listVolumeNames.append(chr(volume_name))
            
    for index, volumeName in enumerate(listVolumeNames):
        print(str(index + 1) + ". Volume " + str(volumeName))
        
    option = int(input("Which volume do you want to choose: "))
    while (option not in range (1, len(listVolumeNames) + 1)):
        option = int(input(f"Input again (input a number from 1 to {len(listVolumeNames)}): "))
        
    #TODO: Continue tomorrow (Do read content of text file, change_directory, draw Directory Tree and test all program)
    volume = None
    if (checkFAT32(listVolumeNames[option - 1])):
        volume = FAT32(listVolumeNames[option - 1])
    elif (checkNTFS(listVolumeNames[option - 1])):
        volume = NTFS(listVolumeNames[option - 1])
    else:
        print ("Our program supports FAT32 and NTFS for the volume type")
        exit()
    
    while (True):
        print(volume.path + ">", end = "")
        command = input()
        volume.doCommand(command)

if __name__ == "__main__":
    main ()