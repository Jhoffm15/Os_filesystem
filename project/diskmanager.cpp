#include "disk.h"
#include "diskmanager.h"
#include <iostream>
using namespace std;

DiskManager::DiskManager(Disk *d, int partcount, DiskPartition *dp)
{
  myDisk = d;
  partCount= partcount;
  r = myDisk->initDisk();
  char buffer[64];

  /* If needed, initialize the disk to keep partition information */
    diskP = dp;
  /* else  read back the partition information from the DISK1 todo*/

}

/*
 *   returns: 
 *   0, if the block is successfully read;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds; (same as disk)
 *  -3 if partition doesn't exist
 */
int DiskManager::readDiskBlock(char partitionname, int blknum, char *blkdata)
{
  /* write the code for reading a disk block from a partition */
  //is the disk opened?
  int parloc = 0;
  if (r == -1) return -1; //if initdisk results in a -1, then we cant really do anything here.
  //scan for the partition
    int j = -1;
    for (int i = 0; i < partCount; i++)
    {
        if (diskP[i].partitionName == partitionname){
          j = i;
          i = partCount;
        }
        else{
          parloc += diskP[i].partitionSize;
        }
    }
    //we now have the partition if we go in here
    if (j != -1)
    {
        //now check if blknum is out of bounds
        if (blknum >= myDisk->getBlockCount()) return -2;
        else
        {
            //if we are here, we can read
            myDisk->readDiskBlock(blknum +parloc,blkdata);
        }

    }
    else if (j == -1) return -3; //otherwise partition doesn't exist
  return 0;
}


/*
 *   returns: 
 *   0, if the block is successfully written;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds;  (same as disk)
 *  -3 if partition doesn't exist
 */
int DiskManager::writeDiskBlock(char partitionname, int blknum, char *blkdata)
{
  /* write the code for writing a disk block to a partition */
    if (r == -1) return -1; //if initdisk results in a -1, then we cant really do anything here.
    //scan for the partition
    int j = -1;
    int parloc = 0;
    for (int i = 0; i < partCount; i++)
    {
        if (diskP[i].partitionName == partitionname){
          j = i;
          i = partCount;
        }
        else{
          parloc += diskP[i].partitionSize;
        }
    }
    //we now have the partition if we go in here
    if (j != -1)
    {
        //now check if blknum is out of bounds
        if (blknum >= myDisk->getBlockCount()) return -2;
        else
        {
            //if we are here, we can FINALLY write to the disk
            myDisk->writeDiskBlock(blknum + parloc,blkdata);
        }
    }
    else if (j == -1) return -3; //otherwise partition doesn't exist
  return 0;
}

/*
 * return size of partition
 * -1 if partition doesn't exist.
 */
int DiskManager::getPartitionSize(char partitionname)
{
  /* write the code for returning partition size */
  //check if theres a match for that partition name
  int j = -1;
  for (int i = 0; i < partCount; i++)
  {
      if (diskP[i].partitionName == partitionname) j = i;
  }
  if (j != -1)
  {
      return diskP[j].partitionSize;
  }
  else return j;
   //place holder so there is no warnings when compiling.
}
