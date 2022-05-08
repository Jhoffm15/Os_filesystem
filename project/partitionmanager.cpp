#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include <iostream>
using namespace std;
/* James Hoffman */
PartitionManager::PartitionManager(DiskManager *dm, char partitionname, int partitionsize)
{
  myDM = dm;
  myPartitionName = partitionname;
  myPartitionSize = myDM->getPartitionSize(myPartitionName);
  myBV = new BitVector(myPartitionSize);
  //might need to put in an if statement (if partition size is too large (exceeds) 512 bits)
  char bitbuffer[64];
  readDiskBlock(0,bitbuffer);
  if(bitbuffer[0] == '#'){//we need to make a bv
    myBV->getBitVector((unsigned int *)bitbuffer);//add 0s
    myBV->setBit(0);//since first block will contain bv set the bit. 
    writeDiskBlock(0,bitbuffer);//write
  }else{//we need to remember the bv
    myBV->setBitVector((unsigned int *)bitbuffer);
  }
}

PartitionManager::~PartitionManager()
{ 
}

/* James Hoffman */
//returns the location of a free block
int PartitionManager::getFreeDiskBlock()
{
  //read and set mybv/buffer
  char bitbuffer[64];
  for (int j = 0; j < 64; j++) bitbuffer[j] = '#';
  myBV->getBitVector((unsigned int *)bitbuffer);
  //-----
  //find next avaliable block
  for(int i = 1; i < myPartitionSize; i++){//start at 1 since 0 will always be in use
    if(myBV->testBit(i)==0){ 
      myBV->setBit(i);//since first block will contain bv set the bit. 
      myBV->getBitVector((unsigned int *)bitbuffer);
      writeDiskBlock(0,bitbuffer);//write
      return i;
    }
  }
  return -1; //place holder so there are no compiler warnings
}

/*James Hoffman*/
//returns a taken block to memory 
int PartitionManager::returnDiskBlock(int blknum)
{
  if(blknum == 0){
    return -1;//should not be able to return the BV
  }
  //read and set bv
  char bvBuffer[64];
  for (int j = 0; j < 64; j++) bvBuffer[j] = '#';
  myBV->getBitVector((unsigned int *)bvBuffer);
  //-----
  if(myBV->testBit(blknum) != 0){//if block is taken
    myBV->resetBit(blknum);//free up bit
    char blkdata[64];
    for (int j = 0; j < 64; j++) blkdata[j] = '#';//fill blkdata
    writeDiskBlock(blknum, blkdata);//rewrite block with all '#'s
    writeDiskBlock(0,bvBuffer);//write bv to memory
    return 0;
  }
  return -1; //block is free therefore it can not be released
}


int PartitionManager::readDiskBlock(int blknum, char *blkdata)
{
  return myDM->readDiskBlock(myPartitionName, blknum, blkdata);
}

int PartitionManager::writeDiskBlock(int blknum, char *blkdata)
{
  return myDM->writeDiskBlock(myPartitionName, blknum, blkdata);
}

int PartitionManager::getBlockSize() 
{
  return myDM->getBlockSize();
}
