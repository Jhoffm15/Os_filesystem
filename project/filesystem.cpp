#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "filesystem.h"
#include <time.h>
#include <cstdlib>
#include <iostream>
#include <bitset>
#include <string>
using namespace std;

// cuts n numbers off the start of a char*
void slice(char* name, int namelen, char* &newname)
{
    newname = new char[namelen - 2];
    for (int i = 0; i < namelen - 2; i++)
    {
        newname[i] = name[i + 2];
    }
}
void slicef(char* name, int namelen, char* &newname)
{
    newname = new char[namelen - 2];
    for (int i = 0; i < namelen - 2; i++)
    {
        newname[i] = name[i];
    }
}
int charToInt(char * buffer, int pos) {
    char conv[4];
    for(int i = 0; i < 4; i++){
        conv[i] = buffer[pos+i];
    }
    int num = atoi(conv);
    return num;
}
void intToChar(char * buffer, int num, int pos) {
    int count = 3;
    while(count >=0){
        buffer[pos + count] = '0'+(num%10);
        num /=10;// c++ truncates ints so rounding is not an issue
        count --;
    }
}
//returns -1 if path not found
//returns blocknum of parent file direvory inode if found
int FileSystem::getpath(char *path, int len){
    int root =0;
    int loc = 1;//used to keep the spot in path we are looking for. 
    char block[64];
    //first find root
    for (int j = 0; j < myPM->myPartitionSize; j++){//search partition for root
        myPM->readDiskBlock(j, block);
        if (path[loc] == block[0] && block[1]==2){//found dir that might be our root dir
            bool found = false;
            //search every directory for the blocknum (j) of the potential root dir
            for(int i = 0; i < myPM->myPartitionSize;i++){
                myPM-> readDiskBlock(i,block);
                if(block[1] == 2){//found a dir. search it for the blocknum. 
                    myPM->readDiskBlock(charToInt(block,6),block);//read in actual dir block. 
                    while(true){
                        for(int x = 0; x < 10; x++){
                            if(charToInt(block,(x*6)+1) == j){//if any of the entries are equal to the potential root dir
                                //then this is not the correct entry.
                                found = true;
                                break;
                            }
                        }   
                        if(found){//if its not found then break to find the next entry. 
                            break;
                        }
                        if(charToInt(block,60) == 0){//there is nothing more to search
                            break;
                        }
                        myPM->readDiskBlock(charToInt(block,60),block);
                    }
                }
                if(found){
                    break;
                }
            }
            if(found){//then it was not the entry we were looking for, keep looking.

            }else{//it was the entry we were looking for so set root and break; 
                root = j;
                break;
            }
        }
    } 
    if(root==0) return -1;//if we could not find root then the path is bad. 
    //then find each dir.
    char* newname;
    slice(path,len,newname);
    return pathhelper(newname,len-2,root);

}
int FileSystem::pathhelper(char* path ,int len, int parent){
    char block[64];
    int holder;
    myPM->readDiskBlock(parent,block);//read in parent
    myPM->readDiskBlock(charToInt(block,6),block);//read in dir
    if(len == 0) return parent;//then we found the parent
    while(true){
        for(int x = 0; x < 10; x++){
            if(path[1] == block[x*6] && block[(x*6)+5] == 2){//if we find a next entry
                //test it out
                parent = charToInt(block,(x*6)+1);
                char* newname;
                slice(path,len,newname);
                holder =  pathhelper(newname,len-2,parent);
                if(holder ==-1) return -1;//wrong path
                return holder;
            }
        }  
        //nothing was found in that block, load in the next
        if(charToInt(block,60) == 0){//there is nothing more so path is bad
            return -1;
        }
        //else we can still find something. 
        myPM->readDiskBlock(charToInt(block,60),block);
    }
    return 0;//error/
}
//returns true if name is valid or false if its not
bool validname(char* filename, int fnameLen){
    char last = filename[0];
    if(fnameLen < 1)
    {
        //invalid filename
        return -3;
    }
    if(last != '/' || fnameLen < 2){
        return false;
    }
    for (int x = 1; x < fnameLen; x++)
    {
        if(last == '/'){
            if((filename[x] >='A' && filename[x] <='Z') || (filename[x] >='a' && filename[x] <='z')){//if next is valid
                last = filename[x];
            }else{//next is not valid meaning incorrect path/name
                return false;
            }
        }else{//else last is a name no need for if else because the above if will check for anything weird. 
            if(filename[x] == '/'){//is valid
                last = filename[x];
            }else{//is not valid
                return false;
            }
        }
    }
    return true;
}
FileSystem::FileSystem(DiskManager *dm, char fileSystemName)
{
  myDM = dm;
  myfileSystemName = fileSystemName;
  myPM = new PartitionManager(myDM,myfileSystemName,myDM->getPartitionSize(myfileSystemName));//(DiskManager *dm, char partitionname, int partitionsize)
  myfileSystemSize = myDM->getPartitionSize(myfileSystemName);
}
//returns -1 if file exists
//returns -2 if there is not enough disk space
//returns -3 if the filename is invalid
//returns -4 if the file cannot be created for any other reason
//returns 0 if successfull
int FileSystem::createFile(char *filename, int fnameLen)
{  
    int blocknum = 0;
    int parent = 0;
    int maxFileName = 8;

    if(!validname(filename,fnameLen))return -3;
    char block[64];
    
    if(fnameLen >2){//gotta deal with roots
        char* newname;
        slicef(filename,fnameLen,newname);
        parent = getpath(newname, fnameLen-2);
        if(parent == -1) return -4;
        blocknum = myPM->getFreeDiskBlock();
        myPM->readDiskBlock(parent,block);
        parent = charToInt(block,6);
        myPM->readDiskBlock(parent,block);//read in dir block
        while(true){
            bool found = false;
            for(int i = 0; i < 10; i++){
                if(block[i*6] == filename[fnameLen-1]&& block[(i*6)+5] == 1){
                    return -1; //already exits
                }
                if(block[i*6] == '0'){//then we found our empty spot
                    block[i*6] = filename[fnameLen-1];
                    intToChar(block,blocknum,(i*6)+1);
                    block[(i*6) + 5] = 1;
                    myPM->writeDiskBlock(parent,block);
                    found = true;
                    break;
                }
            }
            if(found){
                break;
            }
            if(charToInt(block,60) == 0){
                //then we need to make a new block
                int newdir = myPM->getFreeDiskBlock();
                intToChar(block,newdir,60);
                myPM->writeDiskBlock(parent,block);
                for(int i = 0; i < 64; i++) block[i] = '0';
                myPM->writeDiskBlock(newdir,block);
                myPM->readDiskBlock(parent,block);
            }
            myPM->readDiskBlock(charToInt(block,60),block);
        }
    }else{//look for file
        for (int j = 0; j < myPM->myPartitionSize; j++){//check if file already exists
            myPM->readDiskBlock(j, block);
            if (filename[fnameLen-1] == block[0] && block[1]==1){//found possible match
                bool found = true;
                for(int i = 0; i< myPM->myPartitionSize;i++){
                    myPM-> readDiskBlock(i,block);
                    if(block[1] == 2){//found a dir. search it for the blocknum. 
                        myPM->readDiskBlock(charToInt(block,6),block);//read in actual dir block. 
                        while(true){
                            for(int x = 0; x < 10; x++){
                                if(charToInt(block,(x*6)+1) == j){//if the match is in a dir
                                    //then we are good for now
                                    found = false;
                                    break;
                                }
                            }  
                            if(charToInt(block,60) == 0){//there is nothing more to search
                                break;
                            }
                            myPM->readDiskBlock(charToInt(block,60),block);
                        }
                    }
                }
                if(found)return -1;
            }
        }
        blocknum = myPM->getFreeDiskBlock();
    }
    
    char *data = new char[64];

    char nameChar = filename[fnameLen - 1];
    // string bits = bitset<8>(nameChar).to_string();
    data[0] = nameChar;
    data[1] = 1;
    data[2] = '0';
    data[3] = '0';
    data[4] = '0';
    data[5] = '1';
    for(int i =6; i < 64; i++){
        data[i] = '0';
    }
    myPM->writeDiskBlock(blocknum, data);
    

    //File created successfully
    return 0;
    
    return -4;
}

int FileSystem::createDirectory(char *dirname, int dnameLen)
{
    int parent, blocknum;
    char block[64];
    //validation
    if (!validname(dirname, dnameLen)) return -3; //invalid name

    if(dnameLen >2){//gotta deal with roots
        char* newname;
        slicef(dirname,dnameLen,newname);
        parent = getpath(newname, dnameLen-2);
        if(parent == -1) return -4;
        blocknum = myPM->getFreeDiskBlock();
        myPM->readDiskBlock(parent,block);
        parent = charToInt(block,6);
        myPM->readDiskBlock(parent,block);//read in dir block
        while(true){
            bool found = false;
            for(int i = 0; i < 10; i++){
                if(block[i*6] == dirname[dnameLen-1] && block[(i*6)+5] == 2){
                    return -1; //already exits
                }
                if(block[i*6] == '0'){//then we found our empty spot
                    block[i*6] = dirname[dnameLen-1];
                    intToChar(block,blocknum,(i*6)+1);
                    block[(i*6) + 5] = 2;
                    myPM->writeDiskBlock(parent,block);
                    found = true;
                    break;
                }
            }
            if(found){
                break;
            }
            if(charToInt(block,60) == 0){
                //then we need to make a new block
                int newdir = myPM->getFreeDiskBlock();
                intToChar(block,newdir,60);
                myPM->writeDiskBlock(parent,block);
                for(int i = 0; i < 64; i++) block[i] = '0';
                myPM->writeDiskBlock(newdir,block);
                myPM->readDiskBlock(parent,block);
            }
            myPM->readDiskBlock(charToInt(block,60),block);
        }
    }else{//look for file
        for (int j = 0; j < myPM->myPartitionSize; j++){//check if file already exists
            myPM->readDiskBlock(j, block);
            if (dirname[dnameLen-1] == block[0] && block[1]==2){//found possible match
                bool found = true;
                for(int i = 0; i< myPM->myPartitionSize;i++){
                    myPM-> readDiskBlock(i,block);
                    if(block[1] == 2){//found a dir. search it for the blocknum. 
                        myPM->readDiskBlock(charToInt(block,6),block);//read in actual dir block. 
                        while(true){
                            for(int x = 0; x < 10; x++){
                                if(charToInt(block,(x*6)+1) == j){//if the match is in a dir
                                    //then we are good for now
                                    found = false;
                                    break;
                                }
                            }  
                            if(charToInt(block,60) == 0){//there is nothing more to search
                                break;
                            }
                            myPM->readDiskBlock(charToInt(block,60),block);
                        }
                    }
                }
                if(found)return -1;
            }
        }
        blocknum = myPM->getFreeDiskBlock();
    }
    myPM->readDiskBlock(blocknum,block);
    for(int i = 0; i < 64; i++) block[i] = '0';
    block[0] = dirname[dnameLen-1];
    block[1] = 2;
    block[2] = '0';
    block[3] = '0';
    block[4] = '0';
    block[5] = '1';
    int newdir = myPM->getFreeDiskBlock();
    intToChar(block,newdir,6);//give it a starting dir block
    myPM->writeDiskBlock(blocknum,block);
    myPM->readDiskBlock(newdir,block);
    for(int i = 0; i < 64; i++) block[i] = '0';
    myPM->writeDiskBlock(newdir,block);
    return -0; //some other failure happened
}
//this finds the first free entry in the directory inode
//yes this modifies parameters, because it's recursive
//returns:
//-1 if theres not enough disk space to make a new directory inode
//0 if the first directory inode is full, and we had to create a new one. 0 is the location in that new inode we can write to
//anything else: location in the directory inode we can write to
int FileSystem::getfreedirentry(char *block, int &blocknum)
{
    for (int i = 0; i < 60; i+=6)
    {
        if((block[i] == 0 || block[i] == 1) && charToInt(block,i+1) == 0 && block[i+5] == 0) //this is a free entry
        {
            return i;
        }
    }
    //if we are here, we don't have any free entries in this block
    //check if there's an address here
    int a = charToInt(block,60);
    if (a != 0) //we must go to the new address
    {
        blocknum = a;
        myPM -> readDiskBlock(blocknum,block);
        return getfreedirentry(block,blocknum); //we keep looking
    }
    else //out of free entries in this block, but we don't have another directory inode, so we must make one
    {
        int b = myPM->getFreeDiskBlock();
        if (b == -1) return -1; //not enough disk space to do this
        intToChar(block,b,60);
        myPM->writeDiskBlock(blocknum,block); //add the address to the end of the inode
        blocknum = b;
        myPM->readDiskBlock(blocknum,block);
        return 0;//0 is the first entry in that new block
    }
}

//returns -1 if the file is already locked------
//returns -2 if the file does not exist---------
//returns -3 if the file is open----------------
//returns -4 if the file can not be locked for another reason
//else returns lock id (int greater than zero)
//file can not be deleted or renamed until file is unlocked
int FileSystem::lockFile(char *filename, int fnameLen)
{
    for(int i = 0; i < lockedfiletable.size(); i++){//check if already locked
        if(lockedfiletable[i].fname == filename){
            return -1;//file locked
        }
    }
    for(int i = 0; i < openfiletable.size(); i++){//chek if open
        if(openfiletable[i].fname == filename){
            return -3;//file is open
        }
    }
    if(!validname(filename,fnameLen)){
        return -4;
    }
    char block[64];
    int parent;
    int blocknum;
    bool found;
    if(fnameLen >2){//gotta deal with roots
        char* newname;
        slicef(filename,fnameLen,newname);
        parent = getpath(newname, fnameLen-2);
        if(parent == -1) return -1;
        myPM->readDiskBlock(parent,block);
        parent = charToInt(block,6);
        myPM->readDiskBlock(parent,block);//read in dir block
        while(true){
            found = false;
            for(int i = 0; i < 10; i++){
                if(block[i*6] == filename[fnameLen-1] && block[(i*6)+5] == 1){//then we found our file
                    blocknum = charToInt(block,(i*6)+1);
                    found = true;
                    break;
                }
            }
            if(found){
                break;
            }
            if(charToInt(block,60) == 0){//then the file does not exist since we reached the end of parent dir
                return -2;
            }
            myPM->readDiskBlock(charToInt(block,60),block);
        }
    }else{//look for file
    bool found = false;
        for (int j = 0; j < myPM->myPartitionSize; j++){//check if file exists
            myPM->readDiskBlock(j, block);
            if (filename[fnameLen-1] == block[0] && block[1]==1){//found possible match
                found = true;
                blocknum = j;
                for(int i = 0; i< myPM->myPartitionSize;i++){
                    myPM-> readDiskBlock(i,block);
                    if(block[1] == 2){//found a dir. search it for the blocknum. 
                        myPM->readDiskBlock(charToInt(block,6),block);//read in actual dir block. 
                        while(true){
                            for(int x = 0; x < 10; x++){
                                if(charToInt(block,(x*6)+1) == j){//if the match is in a dir
                                    //then we are good for now
                                    found = false;
                                    break;
                                }
                            }  
                            if(charToInt(block,60) == 0){//there is nothing more to search
                                break;
                            }
                            if(!found){
                                break;
                            }
                            myPM->readDiskBlock(charToInt(block,60),block);
                        }
                    }
                }
            }
        }
        if(!found)return -2;
    }
    int r = rand() % 10000 + 1;
    lockedfiletable.push_back(lock(filename, r));
    return r;
    /* if(fnameLen == 2){
        for (int i = 0; i < myPM->myPartitionSize; i++){
            int s = myPM->readDiskBlock(i,block);
            if (filename[fnameLen - 1] == block[0] && block[1] == 1){
                i = myPM -> myPartitionSize;
                //file passes all checks and exists therefore lock it.
                int r = rand() % 10000 + 1;
                lockedfiletable.push_back(lock(filename, r));
                return r;
            }
        }
    }else{//we need to find the dir.
        //int i = findfile(filename,fnameLen);
        //if(i != -1){//we found the file
        //    int r = rand() % 10000 + 1;
        //    lockedfiletable.push_back(lock(filename, r));
        //    return r;
        //}
    } */
    return -2;
}

//returns 0 if successful
//returns -1 if ID is invalid
//returns -2 for any other reason
int FileSystem::unlockFile(char *filename, int fnameLen, int lockId)
{
    for(int i = 0; i < lockedfiletable.size(); i++){
        if(lockedfiletable[i].fname == filename){
            if(lockedfiletable[i].lockid == lockId){//file found and lockid provided so remove lock
                lockedfiletable.erase(lockedfiletable.begin() + i);
                return 0;
            }else{
                return -1;
            }
        }
    }
    return -2; //any other reason
}

//returns -1 if file does not exist
//returns -2 if the file is open or locked
//returns -3 for any other reason
//returns 0 if deleted successfully 
int FileSystem::deleteFile(char *filename, int fnameLen)
{
    char block[64];
    char eblock[64];
    if(!validname(filename,fnameLen))return -3;
    for(int i = 0; i < 64; i++) eblock[i] = '#';
    int fblockloc = 0, iblockloc = 0;
    //check if file is opend or locked
    for(int i = 0; i < lockedfiletable.size(); i++ ){//check if file is locked
        if(lockedfiletable[i].fname == filename){//its locked
            return -2;
        }
    }
    for (int j = 0; j < openfiletable.size(); j++){//check if file is open
        if (filename == openfiletable[j].fname){//its open
            return -2;
        }
    }
    int parent;
    if(fnameLen >2){//gotta deal with roots
        bool found = true;
        char* newname;
        slicef(filename,fnameLen,newname);
        parent = getpath(newname, fnameLen-2);
        if(parent == -1) return -1;
        myPM->readDiskBlock(parent,block);
        parent = charToInt(block,6);
        myPM->readDiskBlock(parent,block);//read in dir block
        while(true){
            found = false;
            for(int i = 0; i < 10; i++){
                if(block[i*6] == filename[fnameLen-1] && block[(i*6)+5] == 1){//then we found our file
                    block[i*6] = 0;
                    block[(i*6)+1] = 0;
                    block[(i*6)+2] = 0;
                    block[(i*6)+3] = 0;
                    block[(i*6)+4] = 0;
                    block[(i*6)+5] = 0;
                    fblockloc = charToInt(block,(i*6)+1);
                    found = true;
                    break;
                }
            }
            if(found){
                break;
            }
            if(charToInt(block,60) == 0){//then the file does not exist since we reached the end of parent dir
                return -1;
            }
            myPM->readDiskBlock(charToInt(block,60),block);
        }
    }else{//look for file
    bool found = false;
        for (int j = 0; j < myPM->myPartitionSize; j++){//check if file exists
            myPM->readDiskBlock(j, block);
            if (filename[fnameLen-1] == block[0] && block[1]==1){//found possible match
                found = true;
                fblockloc = j;
                for(int i = 0; i< myPM->myPartitionSize;i++){
                    myPM-> readDiskBlock(i,block);
                    if(block[1] == 2){//found a dir. search it for the blocknum. 
                        myPM->readDiskBlock(charToInt(block,6),block);//read in actual dir block. 
                        while(true){
                            for(int x = 0; x < 10; x++){
                                if(charToInt(block,(x*6)+1) == j){//if the match is in a dir
                                    //then we are good for now
                                    found = false;
                                    break;
                                }
                            }  
                            if(charToInt(block,60) == 0){//there is nothing more to search
                                break;
                            }
                            if(!found){
                                break;
                            }
                            myPM->readDiskBlock(charToInt(block,60),block);
                        }
                    }
                }
            }
        }
        if(!found)return -1;
    }
    myPM->readDiskBlock(fblockloc, block);
    //passed all tests, now time to start deleting all the files
    if(charToInt(block,18) != 0){// we have to deal with iblock
        iblockloc = charToInt(block,18);
        myPM->readDiskBlock(iblockloc,block);
        for(int i = 15; i >= 0; i--){//iterate through all mblocks and get rid them 
            int curblk = charToInt(block, (i*4));
            if(curblk != 0 ){
                myPM->returnDiskBlock(curblk);
                myPM->writeDiskBlock(curblk, eblock);
            }
        }
        //delete the iblock
        myPM->returnDiskBlock(iblockloc);
        myPM->writeDiskBlock(iblockloc,eblock);
        //read back in file
        myPM->readDiskBlock(fblockloc,block);
    }
    //now deal with the mblocks in fblock
    for(int i = 2; i >= 0; i--){
        int curblk = charToInt(block, 6+(i*4));
        if(curblk !=0){
            myPM->returnDiskBlock(curblk);
            myPM->writeDiskBlock(curblk, eblock);
        }
    }
    //now delete the fblock
    myPM->returnDiskBlock(fblockloc);
    myPM->writeDiskBlock(fblockloc, eblock);
    return 0; //place holder so there is no warnings when compiling.
}
//return values
//-3 other failure
//-2 directory isn't empty
//-1 directory does not exist
//0 successful deletion
int FileSystem::deleteDirectory(char *dirname, int dnameLen)
{
    //validate input
    int blocknum, parent;
    if (dnameLen == 2)
    {
        int rv = directorynavigator(dirname,blocknum);
        if (rv == 0) return -1; //does not exist
    }

    if (!validname(dirname, dnameLen)) return -3; //invalid name return -3; //invalid name


    int fblockloc = 0;
    int k;
    bool fnd;



    //check if directory is empty
    char block[64];
    myPM->readDiskBlock(blocknum,block);
    //get pointer to directory inode
    int dinodeloc = charToInt(block,6);
    char dinode[64];
    myPM->readDiskBlock(dinodeloc,dinode);
    int firstaddr = charToInt(dinode,1);
    if (firstaddr != 0) return -2; //directory is NOT empty if this is true

    //if we are here, we may now delete the directory
    //first, we check if the length is 2. if it is, we're just deleting the file inode and clearing the directory inode it points to, as no directories point to this one
    if (dnameLen == 2)
    {
        for (int i = 0; i < 64; i++)
        {
            block[i] = '#';
            dinode[i] = '#';
        }
        myPM ->writeDiskBlock(blocknum,block); //file inode cleared
        myPM ->writeDiskBlock(dinodeloc,dinode); //directory inode cleared
    }
    else if (dnameLen > 2) //in this case, we also have to remove this directory from the parent directory's inode
    {
        char* newname;
        slicef(dirname,dnameLen,newname);
        parent = getpath(newname, dnameLen-2);
        if(parent == -1) return -3;
        blocknum = myPM->getFreeDiskBlock();
        myPM->readDiskBlock(parent,block);
        parent = charToInt(block,6);
        myPM->readDiskBlock(parent,block);//read in dir block

        while(true){
            bool found = false;

            for(int i = 0; i < 10; i++){
                if(block[i*6] == dirname[dnameLen-1] && block[(i*6)+5] == 2){
                    fblockloc = charToInt(block,(i * 6) + 1);
                    k = i * 6;
                    found = true;
                }

            }
            if(found){
                //remove this off the directory inode
                int x = k + 6;
                while (k < x)
                {
                    block[k] = 0;
                    k++;
                }
                myPM -> writeDiskBlock(parent,block);
                break;
            }
            if(charToInt(block,60) == 0){
                //if we're here, we cant delete a dir that doesn't exist
                return -1;
            }
            myPM->readDiskBlock(charToInt(block,60),block);
        }
        //if we are here, we go to this file inode
        myPM -> readDiskBlock(fblockloc,block);
        //check if this dir inode is empty, if not then we cant do this
        int dinodeaddr = charToInt(block,6);
        myPM ->readDiskBlock(dinodeaddr,block);
        for (int i = 0; i < 64; i++)
        {
            if (block[i] != 0) return -2;
        }

        myPM->writeDiskBlock(dinodeaddr,block);
        myPM -> readDiskBlock(fblockloc,block);
        for (int i = 0; i < 64; i++)
        {
            block[i] = '#';
        }
        myPM ->writeDiskBlock(fblockloc,block);
        return 0;

    }
    return 0; //place holder so there is no warnings when compiling.
}
//returns the location in the block of this entry, so we know where to delete for up above
//returns -1 if it cant find that entry
//yes this does change the block and blocknum we're working from, because we're trying to find the inode and the address that has this entry
int FileSystem::findsubdir(char *block, char key,int blocknum)
{
    int j;
    int entriesinthisnode;
    //first of all, count the entries in THIS inode, by looking if there's an address there AND its a directory we're pointing at
    for (int i = 0; i < 60; i+=6)
    {
        j = charToInt(block,i+1);
        if (j != 0 && block[i + 5] == 2) entriesinthisnode++;
    }
    //now check if this name is actually in the inode
    for (int i = 0; i < entriesinthisnode * 6; i+= 6)
    {
        if (block[i] == key) //we found it
        {
            return i;
        }
    }
    //if we're out of that loop and here, we need to recurse and look in the next directory inode
    int k = charToInt(block,60);
    blocknum = k;
    if (k == 0) return -1; //if there's not an address here, that means we cant find it AND there's no more inodes to go to (no address!)
    myPM -> readDiskBlock(k,block);
    return (findsubdir(block,key,blocknum));
}

//supplies file descriptor to be used by r/w/a 
//lock id will be -1 if user does not think it is locked
//returns -1 if file does not exist
//returns -2 if mode is invalid (not 'r' or 'w')
//returns -3 if the file is locked and "lockID" is incorrect 
//returns -4 for any other fail
//returns file discriptor if successfull 
//a read-write pointer is associated with the file discriptor, the pointer is 0 by deafult (the beginning of hte file)
int FileSystem::openFile(char *filename, int fnameLen, char mode, int lockId)
{
    bool found = false;
    if(!validname(filename,fnameLen)) return -4;
    for(int i = 0; i < lockedfiletable.size(); i++ ){
        if(lockedfiletable[i].fname == filename){//then its locked
            found = true;
            if(lockedfiletable[i].lockid == lockId){//then we can open
                i = lockedfiletable.size();
            }
            else{
                return -3;
            }
        }
    }
    if(!found && lockId != -1){
        return -3;
    }
    //step 0, check if the mode is valid
    if((mode != 'm' && mode != 'r') && mode != 'w')
    {
        //if we're here, we have an invalid mode!
        return -2;
    }
    //step 1: search for that file name
    int s;
    found = false;
    char block[64];
    int parent;
    int blocknum;
    if(fnameLen >2){//gotta deal with roots
        char* newname;
        slicef(filename,fnameLen,newname);
        parent = getpath(newname, fnameLen-2);
        if(parent == -1) return -1;
        myPM->readDiskBlock(parent,block);
        parent = charToInt(block,6);
        myPM->readDiskBlock(parent,block);//read in dir block
        while(true){
            found = false;
            for(int i = 0; i < 10; i++){
                if(block[i*6] == filename[fnameLen-1] && block[(i*6)+5] == 1){//then we found our file
                    blocknum = charToInt(block,(i*6)+1);
                    found = true;
                    break;
                }
            }
            if(found){
                break;
            }
            if(charToInt(block,60) == 0){//then the file does not exist since we reached the end of parent dir
                return -1;
            }
            myPM->readDiskBlock(charToInt(block,60),block);
        }
    }else{//look for file
    bool found = false;
        for (int j = 0; j < myPM->myPartitionSize; j++){//check if file exists
            myPM->readDiskBlock(j, block);
            if (filename[fnameLen-1] == block[0] && block[1]==1){//found possible match
                found = true;
                blocknum = j;
                for(int i = 0; i< myPM->myPartitionSize;i++){
                    myPM-> readDiskBlock(i,block);
                    if(block[1] == 2){//found a dir. search it for the blocknum. 
                        myPM->readDiskBlock(charToInt(block,6),block);//read in actual dir block. 
                        while(true){
                            for(int x = 0; x < 10; x++){
                                if(charToInt(block,(x*6)+1) == j){//if the match is in a dir
                                    //then we are good for now
                                    found = false;
                                    break;
                                }
                            }  
                            if(charToInt(block,60) == 0){//there is nothing more to search
                                break;
                            }
                            if(!found){
                                break;
                            }
                            myPM->readDiskBlock(charToInt(block,60),block);
                        }
                    }
                }
            }
        }
        if(!found)return -1;
    }
    //if we made it here we can push onto openfiletable
    masterfd++;
    open th(lockId, 0, masterfd, filename,mode,fnameLen,blocknum);
    openfiletable.push_back(th);
    return masterfd;
    return -4; //place holder so there is no warnings when compiling.
}

int FileSystem::closeFile(int fileDesc)
{
    bool found = false;
    for (int i = 0; i < openfiletable.size(); i++)
    {
        if (fileDesc == openfiletable[i].filedis)
        {
            found = true;
            //take off the openfiletable
            openfiletable.erase(openfiletable.begin()+i);
        }
    }

    if (found == false) return -1; //file descriptor invalid

    //successful
    return 0;
    //Doesnt work for any other reason
    return -2;
  }
//return -1 if the file discripter is invalid 
//return -2 if length is negative
//return -3 if the operation is not permitted 
//returns the number of bytes r/w/a if successful 
//read/write operates from the location of the rw pointer
//read from the byte pointed to by the rw pointer
//may read less than len if end of file is reached
//at the end of funtion update the rw pointer to the last byte r/w/a
int FileSystem::readFile(int fileDesc, char *data, int len)
{
    bool found = false;
    int oftableloc,fblockloc, readin = 0;
    char block[64];
    //step 1, search open file table for this file descriptor
    for (int j = 0; j < openfiletable.size(); j++){//check if file is open
        if (fileDesc == openfiletable[j].filedis){//file found
            if(openfiletable[j].mode == 'w'){//check to see if we can read the file
                //its write only
                return -1;
            }else{
                //its read only or r/w
                found = true;
                oftableloc = j;
                j = openfiletable.size();
            }
        }
    }
    if (len < 0) return -2;//len is negative
    //the operation is not permitted
    //find the file
    fblockloc = openfiletable[oftableloc].blocation;
    myPM->readDiskBlock(fblockloc,block);
    while(true){ 
        double holder = (double)openfiletable[oftableloc].rwpointer/64;
        int blocknum = holder;//ok stince c++ trunkates
        int blockpointer = (holder - blocknum)*64;//where in the block we should write to. 
        //think about it as we have (blocknum) full blocks and either need a new block or we have a block that is not 'full' yet
        //if blockpointer = 0 get new block;
        //else just write to current block;
        char currentBlock[64];
        int curBlk;
        char iblock[64];
        int iblk;   
        if(blocknum<3){//if we dont need to go to the indirect node 
            //sorry for this ugly statment. it just forces you to check the correct block.
            //without that if statment, unless blockpointer was zero you would always come up 1 block short
            //I realize that if blockpointer >0 then its pointless to check if the block its self is zero buy why add another if.  
            curBlk = charToInt(block,(6+(blocknum*4)));
            if(curBlk == 0){
                //wer are done
                return readin;
            }
            //at this point we are ready to start writing
        }else{//we gotta go into the indirect block
            curBlk = charToInt(block,18);
            //at this point we are ready to get information from Iblock
            iblk = curBlk;
            myPM->readDiskBlock(iblk, iblock);
            //have to subtract 3 from blocknum here since there are 3 blocks in the file inode
            curBlk = charToInt(iblock,(((blocknum-3)*4)));
            if(curBlk == 0){
                //wer are done
                return readin;
            }
            //here we now have the block we are writing to in the iblock
        }
        //-----------------------------------------------
        myPM->readDiskBlock(curBlk,currentBlock);
        while(true){
            if(readin == len || currentBlock[blockpointer] == '#'){
                break;
            }
            data[readin] = currentBlock[blockpointer];
            blockpointer++;
            readin++;
            openfiletable[oftableloc].rwpointer++;
            if((blockpointer%64) == 0){
                break;
            }
        }
        if(readin == len || currentBlock[blockpointer] == '#'){
                return readin;
            }
    }
  return 0; //place holder so there is no warnings when compiling.
}

//return -1 if the file discripter is invalid 
//return -2 if length is negative
//return -3 if the operation is not permitted 
//returns the number of bytes r/w/a if successful 
//read/write operates from the location of the rw pointer
//write from the byte pointed to by the rw pointer
//may increase size of the file
//overwrites the existing data in the file
//at the end of funtion update the rw pointer to the last byte r/w/a
int FileSystem::writeFile(int fileDesc, char *data, int len)
{
    //step 1, search open file table for this file descriptor
    if (len < 0) return -2;
    bool found = false;
    bool ronly = false;
    int s;
    char block[64];
    int nextblock;
    int oftableloc =0;
    int fblockloc = 0;
    for (int j = 0; j < openfiletable.size(); j++){//check if file is open
        if (fileDesc == openfiletable[j].filedis){//file found
            if(openfiletable[j].mode == 'r'){//check to see if we can read the file
                //its read only
                ronly = true;
            }else{
                //its write only or r/w
                found = true;
                oftableloc = j;
                j = openfiletable.size();
            }
        }
    }
    if (ronly == true) return -3;
    if (found == false) return -1; //file descriptor invalid

    fblockloc = openfiletable[oftableloc].blocation;
    myPM->readDiskBlock(fblockloc,block);
    //since the file is open and since we found its location do some math
    //to figure out what block we should be checking.
    int written = 0;
        int iloc = charToInt(block,18);
    while(true){ 
        double holder = (double)openfiletable[oftableloc].rwpointer/64;
        int blocknum = holder;//ok stince c++ trunkates
        int blockpointer = (holder - blocknum)*64;//where in the block we should write to. 
        //think about it as we have (blocknum) full blocks and either need a new block or we have a block that is not 'full' yet
        //if blockpointer = 0 get new block;
        //else just write to current block;
        char mblock[64],iblock[64];//block
        int mloc;//fblockloc,iloc

        if((blocknum) >= 19){//there are no more plces to write 
            return -3;
        }
        if(blocknum < 3){//then we dont need the iblock
            //so get mloc from block
            mloc = charToInt(block,6+(blocknum*4));
            if(mloc == 0){//then we need a new address
                mloc = myPM->getFreeDiskBlock();
                intToChar(block, mloc,6+(blocknum*4));
                intToChar(block,charToInt(block,2)+1,2);//increase size.
                myPM->writeDiskBlock(fblockloc,block);
            }else{
                //we should fill the one already there. 
            }
            myPM->readDiskBlock(mloc,mblock);
        }else{//we need the iblock
            if(iloc == 0){//then we need to make an i block
                iloc = myPM->getFreeDiskBlock();
                intToChar(block, iloc, 18);
                myPM->writeDiskBlock(fblockloc,block);
            }else{
                //use the iblock that is there
            }
            myPM->readDiskBlock(iloc,iblock);
            //now get mloc from iblock
            mloc = charToInt(iblock,(blocknum - 3)*4); 
            if(mloc == 0){//then we need a new address
                mloc = myPM->getFreeDiskBlock();
                intToChar(iblock, mloc,(blocknum - 3)*4);
                intToChar(block,charToInt(block,2)+1,2);//increase size.
                myPM->writeDiskBlock(fblockloc,block);
                myPM->writeDiskBlock(iloc,iblock);
            }else{
                //we should fill the one already there. 
            }
            myPM->readDiskBlock(mloc,mblock);
        }
        //alright now we are ready to write to mblock; 
        while(true){
            mblock[blockpointer] = data[written];
            blockpointer++;
            written++;
            openfiletable[oftableloc].rwpointer++;
            if((blockpointer%64) == 0){//might cause an off by one error
                break;
            }
            if(written == len){
                break;
            }
        }
        myPM->writeDiskBlock(mloc,mblock);
        if(written == len){
            break;
        }
    }
    return written;

}
//return -1 if the file discripter is invalid 
//return -2 if length is negative
//return -3 if the operation is not permitted 
//returns the number of bytes r/w/a if successful 
//read/write operates from the location of the rw pointer
//append data to the end of the file
//at the end of funtion update the rw pointer to the last byte r/w/a
int FileSystem::appendFile(int fileDesc, char *data, int len)
{
    bool found = false;
    char block[64];
    int s = 0;
    int oftableloc = 0;
    int fblockloc = 0;
    int mblock;
    int iblock;
    int size;
    int save;
    int blocknum;//the number of full blocks, will be 1 less than the one we are checking. 
    for (int j = 0; j < openfiletable.size(); j++){//check if file is open
        if (fileDesc == openfiletable[j].filedis){//file found
            found = true;
            oftableloc = j;
            j = openfiletable.size();
        }
    }
    if (found == false) return -1; //file descriptor invalid
    fblockloc = openfiletable[oftableloc].blocation;
    myPM->readDiskBlock(fblockloc,block);
    size = charToInt(block,2);
    blocknum = size -2;
    if(blocknum == -1){//the file is empty. 
        int ret = writeFile(fileDesc, data, len);; //because of the checks above it should always output the number of char appended
        if(ret == -3){
            openfiletable[oftableloc].rwpointer = save;
            return -3;
        }
        return ret;
    }else if(blocknum > 3){//then we need to use Iblock
        iblock = charToInt(block, 18);
        myPM-> readDiskBlock(iblock,block);
        size = charToInt(block,(blocknum-3)*4);
        myPM-> readDiskBlock(size,block);
    }else{//we dont need to use Iblock
        size = charToInt(block, (6 + (blocknum*4)));
        myPM-> readDiskBlock(size,block);
    }
    //at this point we have the most recent block. 
    //use mblock to check what position rwpointer should be
    for(int i = 0; i < 64; i++){//iterate through block until we find a #
        if(block[i] == '#'){//we found the end of what is written
            save = openfiletable[oftableloc].rwpointer;
            openfiletable[oftableloc].rwpointer = ((blocknum)*64)+i;
            i = 64;
        }
    }
    int ret = writeFile(fileDesc, data, len);; //because of the checks above it should always output the number of char appended
    if(ret == -3){
        openfiletable[oftableloc].rwpointer = save;
        return -3;
    }
    return ret;
}

//returns -1 if the fd, offset or flag is invalid, 
//retuns -2 if an offset moves the rwpointer out of bounds
//returns -3 if the mode is read
//else return the num of bytes deleted
//if flag == 0 move the rw pointer offset bytes
//else set rw pointer to offset
int FileSystem::truncFile(int fileDesc, int offset, int flag)
{
    //validate input
    //search for file on open file table
    int myfiledesc = -1;
    bool found = false;
    int s = 0;
    int fblockloc = 0;
    char block[64];
    int index;
    int size;
    int partialblockaddr;
    for (int i = 0; i < openfiletable.size(); i++)
    {
        if (openfiletable[i].filedis == fileDesc) {
            myfiledesc = fileDesc;
            found = true;
            index = i;
            i = openfiletable.size();
        }
    }
    if (!found || (offset < 0 && flag != 0)) return -1;//not found or flag is invalid
    if (openfiletable[index].mode == 'r') return -3; //read only
    //more validation, but first we need to get the inode
    fblockloc = openfiletable[index].blocation;
    myPM->readDiskBlock(fblockloc,block);
        //need to adjsut this for the partially written block
    size = charToInt(block,2)-1;

    if(flag == 0){//move rwpointer by offset
        if(openfiletable[index].rwpointer + offset < 0) return -2;
        if(openfiletable[index].rwpointer + offset > size*64) return -2;
        openfiletable[index].rwpointer = openfiletable[index].rwpointer + offset;
    }else{//set rwpointer to offset
        //already checked if offset < 0
        if(offset > size*64)return -2;
        openfiletable[index].rwpointer = offset;
    }
    //if we are here, we are now able to do stuff
    int bytesdeleted = 0;
    //determine what block to start at
    double holder = (double)openfiletable[index].rwpointer / 64;
    int blocknum = holder;
    int blockpointer = (holder-blocknum)*64;
    //find the partial
    int blkloc;
    if (blocknum < 3){
        blkloc = charToInt(block,6 +(blocknum)*4);
    }
    else if (blocknum >= 3){
        //go to the indirect inode
        int iaddr = charToInt(block,18);
        myPM -> readDiskBlock(iaddr, block);
        //get blocknum from ibloc
        blkloc = charToInt(block,(blocknum-3)*4);
    }
    //get rid of what we need to get rid of. 
    myPM->readDiskBlock(blkloc,block);
    for(blockpointer; blockpointer < 64; blockpointer++){
        if(block[blockpointer] == '#')break;//we are done
        block[blockpointer] = '#';
        bytesdeleted++;
    }
    myPM->writeDiskBlock(blkloc,block);
    if(block[0]=='#'){//then we deleted everything from that block
        blocknum--;//to offset the ++ below
        bytesdeleted-=64;//to offset deleting the whole block.
    }
    blocknum++;//since the partial block is now delt with. 
    if(size == blocknum) return bytesdeleted;//we are done
    //reload fblock into block
    myPM->readDiskBlock(fblockloc,block);
    int iaddr;
    int sizechange = 0;
    if (blocknum < 3){
        for(blocknum; blocknum < 3; blocknum++){
            if(size == blocknum) break;//if we are done deleting things
            //return block to memory
            myPM->returnDiskBlock(charToInt(block,6 + (blocknum*4)));
            //zero out the mem address
            intToChar(block, 0, 6 + (blocknum*4));
            myPM->writeDiskBlock(fblockloc,block);
            bytesdeleted += 64;
            sizechange++;
        }
    }
    bool deliblock = false;
    if (blocknum >= 3){
        if(blocknum==3)deliblock = true;//we are deleting the base iblock so just get rid of the iblock
        //go to the indirect inode
        iaddr = charToInt(block,18);
        myPM -> readDiskBlock(iaddr, block);
        for(blocknum; blocknum < 19; blocknum++){
            if(size == blocknum) break;
            myPM->returnDiskBlock(charToInt(block,((blocknum-3)*4)));
            //zero out the mem address
            intToChar(block, 0, ((blocknum-3)*4));
            myPM->writeDiskBlock(iaddr, block);
            bytesdeleted += 64;
            sizechange++;
        }
        if(deliblock){
        //load back in the fblock
        myPM->readDiskBlock(fblockloc,block);
        //return the indirect block address and zero it out in fblock
        myPM->returnDiskBlock(charToInt(block, 18));
        intToChar(block, 0, 18);
        myPM->writeDiskBlock(fblockloc,block);
        }
    }

    //decreaes size by sizechange
    myPM->readDiskBlock(fblockloc,block);
    size = charToInt(block,2)-sizechange;
    intToChar(block, size,2);
    myPM->writeDiskBlock(fblockloc,block);

    return bytesdeleted;
}

//move r/w pointer
//flag conditions: 
//0 - move offset forward
//else pointer = offset
//returns -1 if the fd is or flag is invalid
    //note a negitive offset is only valid when flag is zero
//returns -2 if you try to go outside bounds (end of file or begining) 
//returns 0 if successful
int FileSystem::seekFile(int fileDesc, int offset, int flag)
{
    bool found = false;
    int fdloc = 0;
    int fileloc = 0;
    char block[64];
    int iloc;
    int minsize = 0;
    int maxsize;
    if(offset < 0 && flag != 0){//flag invalid 
        return -1;
    }
    for (int j = 0; j < openfiletable.size(); j++){//check file discriptor. 
        if (fileDesc == openfiletable[j].filedis){//file found
            fdloc = j;
            j = openfiletable.size();
            found = true;
        }
    }
    if(!found){
        return -1;//fd invalid
    }
    fileloc = openfiletable[fdloc].blocation;
    myPM->readDiskBlock(fileloc,block);
    iloc = charToInt(block, 18);
    if(iloc == 0){//we dont need to use the indirect block
        for(int i = 0; i < 3; i++){
            if(charToInt(block, (6 + i*4)) != 0){//there is an address
                maxsize++;
            }
        }
        maxsize = maxsize*64;
    }else{//we need to use the indirect block
        myPM->readDiskBlock(iloc, block);
        maxsize = 3;//to account for the first 3 datablocks
        for(int i = 0; i < 16; i++){
            if(charToInt(block, (i*4)) != 0){//there is an address
                maxsize++;
            }
        }
        maxsize = maxsize*64;
    }
    if(flag == 0){//move the rwpointer by
        if((openfiletable[fdloc].rwpointer + offset) > maxsize || 
            (openfiletable[fdloc].rwpointer + offset) < minsize){
                return -2;//outsie bounds
        }else{
            openfiletable[fdloc].rwpointer = openfiletable[fdloc].rwpointer + offset;
        }
    }else{//move rwpointer to
        if(offset > maxsize || offset < minsize){
            return -2;//outside bounds
        }else{
            openfiletable[fdloc].rwpointer = offset;
        }
    }
    return 0; //place holder so there is no warnings when compiling.
}   
/* 
    returns -1 if name invalid
    returns -2 if the file does not exist
    returns -3 if there already exists a file with the name fname2
    returns -4 if the file is opened or locked
    returns -5 for any other reason
    returns 0 if correct
    finds file with fname1 and replaces it with fname2
 */
int FileSystem::renameFile(char *filename1, int fnameLen1, char *filename2, int fnameLen2)
{
    char block[64];
    bool found = false;
    int fblockloc;
    //check if names are valid 
    if(!validname(filename1,fnameLen1)) return -1;
    if(!validname(filename2,fnameLen2)) return -1;
    //check if file1 exists
    for (int i = 0; i < myPM->myPartitionSize; i++){
        myPM->readDiskBlock(i,block);
        if (filename1[fnameLen1 - 1] == block[0] && block[1] == 1){//we found the file w2e nned
            found = true;
            fblockloc = i;
            i = myPM -> myPartitionSize;
        }
    }
    if(!found) return -2;
    //check if file2 already exists
    found = false;
    for (int i = 0; i < myPM->myPartitionSize; i++){
        myPM->readDiskBlock(i,block);
        if (filename2[fnameLen2 - 1] == block[0] && block[1] == 1){//there already exists a file with that name
            return -3;
        }
    }
    //check if file is open
    for (int j = 0; j < openfiletable.size(); j++){//check if 1file is open
        if (filename1 == openfiletable[j].fname){//file is open
                return -4;
        }
    }
    //check if file is locked
    for (int j = 0; j < lockedfiletable.size(); j++){//check if 1file is open
        if (filename1 == lockedfiletable[j].fname){//file is locked
                return -4;
        }
    }
    myPM->readDiskBlock(fblockloc,block);
    block[0] = filename2[fnameLen2-1];
    myPM->writeDiskBlock(fblockloc,block);


    return 0; //place holder so there is no warnings when compiling.
}

int FileSystem::renameDirectory(char *dirname1, int dnameLen1, char *dirname2, int dnameLen2)
{
    int parent, blocknum;
    char block[64];
    //validation
    if (!validname(dirname1, dnameLen1)) return -1; //invalid name
    if (!validname(dirname2, dnameLen2)) return -1; //invalid name

    if(dnameLen1 >2){//gotta deal with roots
        char* newname;
        slicef(dirname1,dnameLen1,newname);
        parent = getpath(newname, dnameLen1-2);
        if(parent == -1) return -2;
        //blocknum = myPM->getFreeDiskBlock();
        myPM->readDiskBlock(parent,block);
        parent = charToInt(block,6);
        myPM->readDiskBlock(parent,block);//read in dir block
        while(true){
            bool found = false;
            int k;
            for(int i = 0; i < 10; i++){
                if(block[i*6] == dirname2[dnameLen2-1] && block[(i*6)+5] == 2)
                {
                    //if we are here, the renamed directory already exists
                    return -3;
                }
                if(block[i*6] == dirname1[dnameLen1-1] && block[(i*6)+5] == 2)
                {
                    //if we are here, we found what we are looking for
                    found = true;
                    block[i * 6] = dirname2[dnameLen2-1];
                    k = i * 6;
                    break;
                }
            }
            if(found){
                myPM->writeDiskBlock(parent,block); //write to parent inode
                char fnode[64];
                int fnodeblk = charToInt(block, k + 1);
                int x = myPM -> readDiskBlock(fnodeblk,fnode);
                if (x != 0) return -4;
                fnode[0] = dirname2[dnameLen2 - 1];
                myPM->writeDiskBlock(fnodeblk,fnode); //write to file inode
                break;
            }
            if (charToInt(block,60) == 0)
            {
                //if we are here, we are out of things to search for, and the directory does not exist
                return -2;
            }
            myPM->readDiskBlock(charToInt(block,60),block);
        }
    }else{//look for file, and we are looking to rename a root directory
        for (int j = 0; j < myPM->myPartitionSize; j++){//check if file already exists
            myPM->readDiskBlock(j, block);
            if (dirname1[dnameLen1-1] == block[0] && block[1]==2){//found possible match
                bool found = true;
                for(int i = 0; i< myPM->myPartitionSize;i++){
                    myPM-> readDiskBlock(i,block);
                    if(block[1] == 2){//found a dir. search it for the blocknum.
                        myPM->readDiskBlock(charToInt(block,6),block);//read in actual dir block.
                        while(true){
                            for(int x = 0; x < 10; x++){
                                if(charToInt(block,(x*6)+1) == j){//if the match is in a dir
                                    //then we are good for now
                                    found = false;
                                    break;
                                }

                            }
                            if(charToInt(block,60) == 0){//there is nothing more to search
                                break;
                            }
                            myPM->readDiskBlock(charToInt(block,60),block);
                        }
                    }
                }
                if(found)
                {
                    //read back in j
                    myPM -> readDiskBlock(j,block);
                    block[0] = dirname2[dnameLen2 - 1];
                    myPM -> writeDiskBlock(j,block);
                    return 0;
                }
            }
        }

    }
    /*
    int newdir = myPM->getFreeDiskBlock();
    intToChar(block,newdir,6);//give it a starting dir block
    myPM->writeDiskBlock(blocknum,block);
    myPM->readDiskBlock(newdir,block);
    for(int i = 0; i < 64; i++) block[i] = '0';
    myPM->writeDiskBlock(newdir,block);*/
    return 0; //some other failure happened
}

int FileSystem::getAttribute(char *filename, int fnameLen, int location  /*... and other parameters as needed */)
{
    bool found = false;
    
    if(!validname(filename,fnameLen)) return -3;

    int s;
    char block[64];

    char getblock[64];
    int read;
    int write;
    int attributelocation;
    char *attribute;

    //Search for that file name
    for (int i = 0; i < myPM->myPartitionSize; i++)
    {
        s = myPM->readDiskBlock(i, block);

        if (filename[fnameLen - 1] == block[0])
        {
            found = true;
            //congrats! We have the file.
            i = myPM->myPartitionSize;
            //Get Attributes
            if (location == 0)
            {
                //Allocated bytes 23-43
                read = myPM->readDiskBlock(i, getblock);

                int z = 0;
                //Get attribute
                while (getblock[z + 22] != '0')
                {
                    attribute[z] = getblock[z + 22];
                    z++;
                }
                for (int q = 0; q > 5; q++)
                {
                    //Compare attribute to list of attribute
                    if (attribute == attributeColor[q])
                    {
                        //Return array location of attribute
                        return q;
                    }
                }
            }
            else if (location == 1)
            {
                //Allocated bytes 44-64

                read = myPM->readDiskBlock(i, getblock);

                int z = 0;
                while (getblock[z + 44] != '0')
                {
                    attribute[z] = getblock[z + 44];
                    z++;
                }
                for (int q = 0; q > 3; q++)
                {
                    if (attribute == attributeSize[q])
                    {
                        return q;
                    }
                }
            }
            else
            {
                //Invalid location
                return -5;
            }
        }
        if (found == false)
        {
            return -1;
        }

        return -4; //place holder so there is no warnings when compiling.
    }
    return -4;
}
/* 
int parent;
    int blocknum;
    if(fnameLen >2){//gotta deal with roots
        char* newname;
        slicef(filename,fnameLen,newname);
        parent = getpath(newname, fnameLen-2);
        if(parent == -1) return -1;
        myPM->readDiskBlock(parent,block);
        parent = charToInt(block,6);
        myPM->readDiskBlock(parent,block);//read in dir block
        while(true){
            found = false;
            for(int i = 0; i < 10; i++){
                if(block[i*6] == filename[fnameLen-1] && block[(i*6)+5] == 1){//then we found our file
                    blocknum = charToInt(block,(i*6)+1);
                    found = true;
                    break;
                }
            }
            if(found){
                break;
            }
            if(charToInt(block,60) == 0){//then the file does not exist since we reached the end of parent dir
                return -1;
            }
            myPM->readDiskBlock(charToInt(block,60),block);
        }
    }else{//look for file
    bool found = false;
        for (int j = 0; j < myPM->myPartitionSize; j++){//check if file exists
            myPM->readDiskBlock(j, block);
            if (filename[fnameLen-1] == block[0] && block[1]==1){//found possible match
                found = true;
                blocknum = j;
                for(int i = 0; i< myPM->myPartitionSize;i++){
                    myPM-> readDiskBlock(i,block);
                    if(block[1] == 2){//found a dir. search it for the blocknum. 
                        myPM->readDiskBlock(charToInt(block,6),block);//read in actual dir block. 
                        while(true){
                            for(int x = 0; x < 10; x++){
                                if(charToInt(block,(x*6)+1) == j){//if the match is in a dir
                                    //then we are good for now
                                    found = false;
                                    break;
                                }
                            }  
                            if(charToInt(block,60) == 0){//there is nothing more to search
                                break;
                            }
                            if(!found){
                                break;
                            }
                            myPM->readDiskBlock(charToInt(block,60),block);
                        }
                    }
                }
            }
        }
        if(!found)return -1;
    }

 */
int FileSystem::setAttribute(char *filename, int fnameLen, char *attribute, int aLen, int location /* ... and other parameters as needed*/ )
{
    bool found = false;
    
    if(!validname(filename,fnameLen)) return -3;
    
    int s;
    char block[64];

    char newblock[64];
    int read;
    int write;
    int attribloc;

    bool foundatt = false;
    int attributelocation;
    
    if (location == 0)
    {
        //Check if attribute is valid color

        for (int q = 0; q > 5; q++)
        {
            if (attribute == attributeColor[q])
            {
                attributelocation = q;
                found = true;
            }
        }
        if(found == false){
            return -5;
        }
    }
    else if (location == 1)
    {
        //Check if attribute is valid size

        for (int q = 0; q > 3; q++)
        {
            if (attribute == attributeSize[q])
            {
                attributelocation = q;
                foundatt = true;
            }
        }
        if(foundatt == false){
            return -5;
        }
    }

    //Search for that file name
    for (int i = 0; i < myPM->myPartitionSize; i++)
    {
        s = myPM->readDiskBlock(i, block);

        if (filename[fnameLen - 1] == block[0])
        {
            found = true;
            //congrats! We have the file.
            i = myPM->myPartitionSize;
            //Set Attributes
            if (location == 0)
            {
                //Set attribute in first location
                //Allocated bytes 23-43

                //Create new char array.
                //Read block into array.
                read = myPM->readDiskBlock(i, newblock);
                //Add attribute.
                attribloc = 23;
                for (int z = 0; z < aLen; z++)
                {
                    newblock[attributelocation] = attribute[z];
                    attributelocation++;
                }
                //Write Disk Block
                write = myPM->writeDiskBlock(i, newblock);
                //Successful
            return 1;
            }
            else if (location == 1)
            {
                //Set attribute in second location
                //Allocated bytes 44-64

                //Create new char array.
                //Read block into array.
                read = myPM->readDiskBlock(i, newblock);
                //Add attribute.
                attribloc = 44;
                for (int z = 0; z < aLen; z++)
                {
                    newblock[attribloc] = attribute[z];
                    attribloc++;
                }
                //Write Disk Block
                write = myPM->writeDiskBlock(i, newblock);
                //Successful
            return 1;
            }
            else
            {
                //Invalid location int
                return -6;
            }
        }
    }
    if (found == false)
    {
        return -1;
    }

    return -4; //place holder so there is no warnings when compiling.
}

//return values
//1: directory is found, block location is put in blocknum
//0: directory not found
//-1: directory name invalid
//-2: directory inode doesnt exist, check your stuff please
int FileSystem::directorynavigator(char* path, int &blocknum)
{
    //first search the whole partition for the directory, the first one should be it
    char block[64];
    bool found = false;
    int fblockloc;
    int k = 0;
    int pathlength = 0;
    //calculate the path length
    while(path[k] != 0)
    {
        pathlength++;
        k++;
    }
    if(pathlength %2 == 1){//its odd
        pathlength--;
    }
    //check if names are valid
    if (!validname(path, pathlength)) return -1; //your path is invalid

    //check if node exists AND its a directory
    for (int i = 0; i < myPM->myPartitionSize; i++){
        myPM->readDiskBlock(i,block);
        if (path[1] == block[0] && block[1] == 2){//we found the first directory
            found = true;
            fblockloc = i;
            i = myPM -> myPartitionSize;
        }
    }
    if(!found) return 0;
    if (pathlength == 2 && found) //if the first level directory was what we're looking for
    {
        blocknum = fblockloc;
        return 1;
    }
    //now that we're here, we must go to that directory inode and keep looking
    int addr = charToInt(block, 6);
    if (addr == 0) return -2; //its not this function's responsibility to make the proper structures
    //go to block
    char dirinode[64];
    int dirfound;
    myPM->readDiskBlock(addr,dirinode);
    for (int k = 3; k < pathlength; k+=2)
    {
        dirfound = 0;
        vector<char> dirnames;
        vector<int> dirpointers;
        buildlist(dirinode,dirnames,dirpointers);
        //we now have a list of just directories
        //scan it
        int nextaddr = 0;

        for (int i = 0; i < dirnames.size(); i++)
        {
            if (path[k] == dirnames[i]) //we have found the next directory
            {
                fblockloc = dirpointers[i];
                myPM->readDiskBlock(fblockloc,block);
                dirfound = 1;
                break;
            }
        }
        //now we read the directory inode so we can do this again
        if (dirfound == 1)
        {
            nextaddr = charToInt(block,6);
            myPM->readDiskBlock(nextaddr,dirinode);
        }
        else if (dirfound == 0) return 0;//we couldnt find that directory, ending the search

    }
    //if we are here, we have found the directory, and we will now finish up and give block number
    blocknum = fblockloc;
    return 1;
}
void FileSystem::buildlist(char* block,vector<char> names, vector<int>pointers)
{
    //we use this function to make a list of all the directories this inode points to
    for (int i = 0; i < 60; i += 6)
    {
        if (block[i] == 0) break; //no more stuff, we're done here
        char what = block[i + 5];
        if (what == 2)
        {
            names.push_back(block[i]);
            pointers.push_back(charToInt(block, i+1));
        }
    }
    if (block[60] != 0) //theres more stuff
    {
        char newblock[64];
        int addr = charToInt(block,60);
        myPM->readDiskBlock(addr,newblock);
        buildlist(newblock,names,pointers);
    }
}
//returns blocknum if found
//returns -1 if not found
