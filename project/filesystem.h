#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include <vector>

class open{

public:
    int lockid;
    int rwpointer;
    int filedis;
    char *fname;
    int fnlen;
    char mode;
    bool locked = false;
    int blocation;

    open(int lid, int rwp, int fd, char *fn,char m,int nlen, int loc)
    {
        fnlen = nlen;
        lockid = lid;
        rwpointer = rwp;
        filedis = fd;
        fname = fn;
        mode = m;
        blocation = loc;
    }
};

class lock {
    public : 
        int lockid;
        char *fname;
        lock(char *fn, int id){
            fname = fn;
            lockid = id;
        }
};

class FileSystem {
  DiskManager *myDM;
  PartitionManager *myPM;
  char myfileSystemName;
  int myfileSystemSize;
  std::vector<open> openfiletable;
  vector<lock> lockedfiletable;
  int masterfd = 0;
  const char *attributeColor[5] = {"red", "blue", "yellow", "green", "purple"};
  const char *attributeSize[3] = {"small", "medium", "large"};

  /* declare other private members here */

  public:
    FileSystem(DiskManager *dm, char fileSystemName);
    int createFile(char *filename, int fnameLen);
    int createDirectory(char *dirname, int dnameLen);
    int lockFile(char *filename, int fnameLen);
    int unlockFile(char *filename, int fnameLen, int lockId);
    int deleteFile(char *filename, int fnameLen);
    int deleteDirectory(char *dirname, int dnameLen);
    int openFile(char *filename, int fnameLen, char mode, int lockId);
    int closeFile(int fileDesc);
    int readFile(int fileDesc, char *data, int len);
    int writeFile(int fileDesc, char *data, int len);
    int appendFile(int fileDesc, char *data, int len);
    int seekFile(int fileDesc, int offset, int flag);
    int truncFile(int fileDesc, int offset, int flag);
    int renameFile(char *filename1, int fnameLen1, char *filename2, int fnameLen2);
    int renameDirectory(char *dirname1, int dnameLen1, char *dirname2, int dnameLen2);
    int getAttribute(char *filename, int fnameLen, int location  /* ... and other parameters as needed */);
    int setAttribute(char *filename, int fnameLen, char *attribute, int aLen, int location /* ... and other parameters as needed */);
    /* declare other public members here */
    int directorynavigator(char* path, int &blocknum);
    void buildlist(char* block,vector<char> names, vector<int>pointers); //this is a recursive algorithm that builds a list of subdirectories
    int findsubdir(char* block, char key, int blocknum); //this is a recursive algorithm that searches for an entry you specified
    int getfreedirentry(char* block,int &blocknum);
    int findfile(char*filename, int len);
    bool samepath(char* myfname,int mylen,int foundblocknum);
    int getpath(char *fpath, int len);
    int pathhelper(char* path, int len, int parent);
};


#endif
