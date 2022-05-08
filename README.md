# os_project_files

## James Lucid, James Hoffman, Matthew Baker

<p> What's working: Everything up to driver 4, partially working on 5, 6, 7, 8</p>
<p> What's not working: deletedirectory at the moment and some file commands with directories </p>

<p>Attributes used:</p>
We used two attributes, that being color. While this is only one attribute, the file can have two colors at the same time.


---

* base grade: 62/100
  * group eval:  individual grades will be emailed.
* create file (imples r/w in dm, pm r/w, getfree, and returnblock) (10 points):  no. -10
  * dm my empty comment in the constructor is still there.  so nothing was done for the superblock. read and write are working. 
  * pm, bitvector is put in block 0, so not an accident. get and free should work.  destructor is empty.
  * createfile itself.  so there is a search if it in the "root" directory. otherwise, searc for a directory.  just bad ...
    * the file creation has the "1", but it's a dot.  then a size of 0001, which should be zero.  so one of these if size?  directions were not followed at all.
* openfile (and close 3points of) opentable, etc ignoring unlock/lock (10 points):  more or less -2
  *  I believe this actually works for the most part.  not with directories, but root files do work. 
* readfile (10 points):  yes
  * file size is the table, not the file? 
  *  there is a algorithm that shold work actually.   if directories worked, I think it would too.
* writefile (10 points): mostly -1
  * so file size is not written to the file, at least not that I can find.
  * the rest I believe works. 
* truncfile (10 points):  yes
  * again, good algorith, it works.  it has a size change, so position 2 to 6 is size then?    
* seekfile (5 points):  likely works.
  * what is stored in posotion 18 that seek would use.  it's calc file size maybe. 
* appendfile ( 5 points, since just call see to end and write):  -1
  *  I don't see anywhere you move the seek pointer to end  but it does call write.
* createdir (10 points) => implies create,open work as well: -8
  * there is a poor attempt at it, but it doesn't work.  again even the format spec'd wasn't followed.  did you even read the directions?
* lock and unlock (5 points each, 10 points)  looped into open/close as well: yes
  *  it's a mess, but I think it actually works. 
* rename (5 points): -3
  * some basic changes are made, but this doesnt work. 
* deletefile (5 points), remember lock again: -1
  * without directories and following it that you know you deleted the correct file????? 
* deletedir (5 points), remember empty: -2
  *  this sort of works, but I could not find any evidence that it worked in the disk.  of course I could not find a directory, so there is that problem too. 
* attributes read/set (5 points): no -5
  * wait or attributes not written in readable?  I really don't understand how the set or get actually could work.
other notes:
  * disk is 
    * driver 1: well it's corrupted.  no sure what the point of continuing is.  no superblock. the bitvector is in block0.  there is no root directory.  it's a file.  The file is not following the standard for the layout.  is the 1 supposed to be the size?   can't be right, file b has an 11?  is one of those zero a default attribute?   totally broken and I didn't even have to slide the ghex window to know that.
    * driver 2: so the no root directory is not a mistake?  same issue for fs2 and fs3.  files are still wrong format. 
    * driver 3: I think truncfile works.  but the rest???  can't tell in the file.  all partitions are corrupted.
    * driver 4: so read and write are mostly working.  truncfile looks to work.  others, I just can't tell from the damaged disk.
    * driver 5: not sure what point of running these are.  mostly fails.  disk is corrupted and I can't tell if works.  there maybe a directory, but standard was not followed 
    * driver 6: maybe there is a directory?? but honesty, I don't see much different in the disk.  most are fails.
    * driver 7: run and all fails as expected.
    * driver 8: it says all fails.  looking through the disk, there are no colors.  blue small nothing.
  * other
    * before I even look at the code. The disk makes it very obvious, you all did not read the spec's or bother to follow most of them.  No root directories, didn't follow file stadard format (like directory one too, if i did find one directory).  bv is in the wrong spot.  basically this project without looking at code is a mess and almost total failure.  it's only the metric that might save you grade here.
    * getpath and pathhelper is the fix for forgetting a root directory?
    * the code itself is mess, some of it poorely commented or even the comments don't make sense.  things like "do stuff here" are not helpful comments.  variable names are pretty good at least, so you style will bad is not just flat out terrible -5
