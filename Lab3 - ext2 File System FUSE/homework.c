/*
 * file:        homework.c
 * description: skeleton file for CS 5600 system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2023
 */

#define FUSE_USE_VERSION 30
#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fuse3/fuse.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include "fs5600.h"

#define PATH_MAX 32
#define NUM_DIRENT 32
#define MAX_INODE 256
#define INDIR_PTRS 256
#define DOUBLE_INDIR_PTRS 65536
/*
 * Global variable
 */
struct fs_super superBlock;
void* blockBitmap;
void* inodeBitmap;
struct fs_inode* inodes;
/* disk access. All access is in terms of 4KB blocks; read and
 * write functions return 0 (success) or -EIO.
 */
extern int block_read(void *buf, int blknum, int nblks);
extern int block_write(void *buf, int blknum, int nblks);

/* how many buckets of size M do you need to hold N items? 
 */
int div_round_up(int n, int m) {
    return (n + m - 1) / m;
}

/* quick and dirty function to split an absolute path (i.e. begins with "/")
 * uses the same interface as the command line parser in Lab 1
 */
int split_path(const char *path, int argc_max, char **argv, char *buf, int buf_len)
{
    int i = 0, c = 1;
    char *end = buf + buf_len;

    if (*path++ != '/' || *path == 0)
        return 0;
        
    while (c != 0 && i < argc_max && buf < end) {
        argv[i++] = buf;
        while ((c = *path++) && (c != '/') && buf < end)
            *buf++ = c;
        *buf++ = 0;
    }
    return i;
}

/* I'll give you this function for free, to help 
 */
void inode_2_stat(struct stat *sb, struct fs_inode *in)
{
    memset(sb, 0, sizeof(*sb));
    sb->st_mode = in->mode;
    sb->st_nlink = 1;
    sb->st_uid = in->uid;
    sb->st_gid = in->gid;
    sb->st_size = in->size;
    sb->st_blocks = div_round_up(in->size, BLOCK_SIZE);
    sb->st_atime = sb->st_mtime = sb->st_ctime = in->mtime;
}
/*
 * Allocates a block
 */
int allocateBlock(int blockNum){
    /*
     * blockNum = 1 -> Block Bitmap
     * blockNum = 2 -> Inode Bitmap
     */
    int availableBlock = -1;
    int32_t range = 0;
    unsigned char *map;
    if(blockNum == 1){
        map = blockBitmap;
        range = superBlock.disk_size;
    }else{
        map = inodeBitmap;
        range = superBlock.inodes_len * (BLOCK_SIZE/sizeof (struct fs_inode));
    }
    for(int i = 0; i < range ; i++){
        if(!bit_test(map, i)){
            availableBlock = i;
            break;
        }
    }
    if(availableBlock == -1){
        return -ENOSPC;
    }
    bit_set(map,availableBlock);
    block_write(map, blockNum, 1);

    return availableBlock;
}
/*
 * Helper function to retrieve the actual block number
 */
int getBlock(struct fs_inode *in, int blockNum){

    assert(blockNum >= 0);
    assert(blockNum < 65536);

    if(blockNum < 6){
       return in->ptrs[blockNum];
    }else if(blockNum < N_DIRECT + INDIR_PTRS){
        int32_t ptrs[INDIR_PTRS];
        assert(block_read(&ptrs, in->indir_1, 1) == 0);
        return ptrs[blockNum - N_DIRECT];
    }else {
        int32_t ptrs[INDIR_PTRS];
        assert(block_read(&ptrs, in->indir_2, 1) == 0);
        int inDirOne = (blockNum-N_DIRECT-INDIR_PTRS) / 256;
        assert(block_read(&ptrs, ptrs[inDirOne], 1) == 0);
        int inDirTwo = (blockNum-N_DIRECT-INDIR_PTRS) % 256;
        return ptrs[inDirTwo];
    }
}
int setBlock(struct fs_inode *in, int blockNum, int blockId){
    assert(blockNum >= 0);
    assert(blockNum < 65536);

    if(blockNum < 6){
        in->ptrs[blockNum] = blockId;
    }else if(blockNum < N_DIRECT + INDIR_PTRS){
        if(in->indir_1 == 0){
            in->indir_1 = blockId;
            int dataBlock = allocateBlock(1);
            if(dataBlock < 0){
                return -ENOSPC;
            }
            blockId = dataBlock;
        }
        int32_t ptrs[INDIR_PTRS];
        assert(block_read(&ptrs, in->indir_1, 1) == 0);
        ptrs[blockNum - N_DIRECT] = blockId;
        assert(block_write(&ptrs, in->indir_1, 1) == 0);
    }else {
        if(in->indir_2 == 0){
            in->indir_2 = blockId;
            int dataBlock1 = allocateBlock(1);
            if(dataBlock1 < 0){
                return -ENOSPC;
            }
            int32_t ptrs[INDIR_PTRS];
            //memset here
            assert(block_read(&ptrs, in->indir_2, 1) == 0);
            ptrs[blockNum-N_DIRECT-INDIR_PTRS] = dataBlock1;
            assert(block_write(&ptrs, in->indir_2, 1) == 0);
            int dataBlock2 = allocateBlock(1); //#define
            if(dataBlock2 < 0){
                return -ENOSPC;
            }
            blockId = dataBlock2;
        }
        int32_t ptrsOne[INDIR_PTRS];
        int32_t ptrsTwo[INDIR_PTRS];
        assert(block_read(&ptrsOne, in->indir_2, 1) == 0);
        int inDirOne = (blockNum-N_DIRECT-INDIR_PTRS) / 256;
        if(ptrsOne[inDirOne] == 0){
            //Need to allocate a new block
            int dataBlock = allocateBlock(1); //#define
            if(dataBlock < 0){
                return -ENOSPC;
            }
            ptrsOne[inDirOne] = dataBlock;
            assert(block_write(&ptrsOne, in->indir_2, 1) == 0);
        }
        assert(block_read(&ptrsTwo, ptrsOne[inDirOne], 1) == 0);
        int inDirTwo = (blockNum-N_DIRECT-INDIR_PTRS) % 256;
        ptrsTwo[inDirTwo] = blockId;
        assert(block_write(&ptrsTwo, ptrsOne[inDirOne], 1) == 0);
    }
    return 0;
}
void* lab3_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    if(block_read(&superBlock, 0, 1) == -EIO){
        perror("Error reading superblock\n");
        exit(1);
    }
    blockBitmap = malloc(superBlock.blk_map_len * BLOCK_SIZE); //TODO(ABHIJITH) : Need to free this
    if(block_read(blockBitmap, 1, superBlock.blk_map_len) == -EIO){
        perror("Error reading Block Bitmap\n");
        exit(1);
    }
    inodeBitmap = malloc(superBlock.in_map_len * BLOCK_SIZE); //TODO(ABHIJITH) : Need to free this
    if(block_read(inodeBitmap,2, superBlock.in_map_len) == -EIO){
        perror("Error reading Inode Bitmap\n");
        exit(1);
    }
    inodes = malloc(superBlock.inodes_len * BLOCK_SIZE); //TODO(ABHIJITH) : Need to free this
    if(block_read(inodes,3, superBlock.inodes_len) == -EIO){
        perror("Error reading Inode table\n");
    }
    return NULL;
}

/*
 * Looks up specific inode
 */
int lookup(int inode, char* name){
    assert(inode > 0);
    assert(inode < MAX_INODE + 1);
    struct fs_inode *in = &inodes[inode];
    if(!S_ISDIR(in->mode)){
        return -ENOTDIR;
    }
    assert(in->size % BLOCK_SIZE == 0);
    for(int i = 0; i < (in->size)/BLOCK_SIZE ; i++){
        int blockNum = getBlock(in,i);
        struct fs_dirent dent[NUM_DIRENT];
        int ret = block_read(&dent,blockNum,1); //Optimize
        assert(ret == 0);
        for(int i= 0 ; i< NUM_DIRENT; i++){
            if(dent[i].valid == 1 && strcmp(name,dent[i].name) == 0){
                return dent[i].inode;
            }
        }
    }
    return -ENOENT;
}

/*
 * Helper function to find the inode number
 */
int findInodeNumber(int argc, char **argv){
    int inum = 1; //Starting from the root directory
    for(int i = 0; i < argc ; i++){
        inum = lookup(inum, argv[i]);
        if(inum < 0){
            return inum;
        }
    }
    return inum;
}

/*
 * Below helper function is used to factor out splitting absolute paths
 */
int pathSplitHelper(const char *path, char** tokens){
    char linebuf[1024];
    int n_tokens = split_path(path,PATH_MAX,tokens,linebuf,sizeof(linebuf));
    return n_tokens;
}

int lab3_getattr(const char *path, struct stat *sb, struct fuse_file_info *fi){
    //Parse path
    char *tokens[PATH_MAX];
    char linebuf[1024];
//    int n_tokens = pathSplitHelper(path, tokens);
    int n_tokens = split_path(path,PATH_MAX,tokens,linebuf,sizeof(linebuf));

    //Get inode number
    int inum = findInodeNumber(n_tokens,tokens);
    if(inum < 0){
        return inum;
    }
    assert(inum > 0);

    struct fs_inode *in = &inodes[inum];
    inode_2_stat(sb,in);
    //return
    return 0;
}

int lab3_readdir(const char *path, void *ptr, fuse_fill_dir_t filler, off_t offset,
                 struct fuse_file_info *fi, enum fuse_readdir_flags flags){
    //Parse path
    char *tokens[PATH_MAX];
    char linebuf[1024];
    int n_tokens = split_path(path,PATH_MAX,tokens,linebuf,sizeof(linebuf));
    //Get inode number
    int inum = findInodeNumber(n_tokens,tokens);
    if(inum < 0){
        return inum;
    }
    assert(inum > 0);

    //Read the block associated with the inode
    struct fs_inode *in = &inodes[inum];
    //Check if it's not a directory
    if(!S_ISDIR(in->mode)){
        return -ENOTDIR;
    }

    for(int i = 0; i < (in->size)/BLOCK_SIZE ; i++){
        int blockNum = getBlock(in,i);
        struct fs_dirent dent[NUM_DIRENT];
        int ret = block_read(&dent,blockNum,1); //Optimize
        assert(ret == 0);
        for(int i= 0 ; i< NUM_DIRENT; i++){
            if(dent[i].valid == 0){
                continue;
            }
            filler(ptr, dent[i].name, NULL, 0, 0);
        }
    }
    return 0;
}
int lab3_read(const char *path, char *buf, size_t len, off_t offset,
              struct fuse_file_info *fi){
    //Parse path
    char *tokens[PATH_MAX];
    char linebuf[1024];
    int n_tokens = split_path(path,PATH_MAX,tokens,linebuf,sizeof(linebuf));
    //Get inode number
    int inum = findInodeNumber(n_tokens,tokens);
    if(inum < 0){
        return inum;
    }
    assert(inum > 0);

    //Read the block associated with the inode
    struct fs_inode *in = &inodes[inum];

    //Check if directory. If true return -EISDIR
    if(S_ISDIR(in->mode)){
        return -EISDIR;
    }
    //No data to read if offset >= size of the file
    if(offset >= in->size){
        return 0;
    }

    //Check if asking length exceeds the actual size
    if((offset+len) > in->size){
        len = in->size - offset;
    }

    //Get the start and end block indexes
    int blockStartIndex = offset / BLOCK_SIZE;
    int blockEndIndex = (offset + len - 1) / BLOCK_SIZE;

    assert(blockEndIndex >= 0);
    assert(blockEndIndex >= blockStartIndex);

    int bytesLeftToRead = len;
    int startByteRead = offset % BLOCK_SIZE;
    char *bufPtr = buf;


    for(int i= blockStartIndex; i<= blockEndIndex; i++){
        int blockNumber = getBlock(in,i);
        if(blockNumber != 0){
            char tempBlock[BLOCK_SIZE];
            int readLength = BLOCK_SIZE - startByteRead;
            if(readLength >= bytesLeftToRead){
                readLength = bytesLeftToRead;
            }
            assert(block_read(&tempBlock, blockNumber, 1) == 0);
            memcpy(bufPtr, &(tempBlock[startByteRead]),readLength);
            bufPtr +=readLength;
            startByteRead = 0;
            bytesLeftToRead -= readLength;
        }
    }
    assert(bytesLeftToRead == 0);
    return len;
}

int lab3_utimens(const char *path, const struct timespec tv[2],
                 struct fuse_file_info *fi){
    return 0;
}
/*
 * Helper code for create function
 */
int createHelper(const char* path, mode_t mode){
    //Parse path
    char *tokens[PATH_MAX];
    char linebuf[1024];
    int n_tokens = split_path(path,PATH_MAX,tokens,linebuf,sizeof(linebuf));

    if(strlen(tokens[n_tokens-1]) > 27){
        return -ENAMETOOLONG;
    }
    //Check if the file/directory already exists
    int inum = findInodeNumber(n_tokens,tokens);
    if(inum > 0){
        return -EEXIST;
    }

    //Get the parent's inode and check if it is a directory
    inum = findInodeNumber(n_tokens - 1,tokens);
    if(inum < 0){
        return inum;
    }
    assert(inum > 0);

    struct fs_inode *parent = &inodes[inum];
    if(!S_ISDIR(parent->mode)){
        return -ENOTDIR;
    }

    //Get the Block number and the directory entry of the first valid entry
    int blockNum = 0;
    int dirIndex = 0;
    struct fs_dirent dent[NUM_DIRENT];
    for(int i = 0; i < (parent->size)/BLOCK_SIZE ; i++){
        bool validFound = false;
        blockNum = getBlock(parent,i);
        assert(block_read(&dent,blockNum,1) == 0);
        for(; dirIndex < NUM_DIRENT; dirIndex++){
            if(dent[dirIndex].valid == 0){
                validFound = true;
                break;
            }
        }
        if(validFound){
            break;
        }
        dirIndex = 0;
    }
    //No more space
    if(blockNum == DOUBLE_INDIR_PTRS && dirIndex == NUM_DIRENT){
        return -ENOSPC;
    }

    //Need two blocks, One for Inode and other for Data
    /*
     * blockNum = 1 -> Block Bitmap
     * blockNum = 2 -> Inode Bitmap
     */
    int iNodeBlock = allocateBlock(2);
    if(iNodeBlock < 0){
        return -ENOSPC;
    }
    int dataBlock = allocateBlock(1);
    if(dataBlock < 0){
        bit_clear(inodeBitmap, iNodeBlock);
        return -ENOSPC;
    }

    int32_t currentTime = time(NULL);
    struct fs_inode *newInode = &inodes[iNodeBlock];
    newInode->uid = 0;
    newInode->gid = 0;
    newInode->mode = mode;
    newInode->mtime = currentTime;
    if(mode == (mode | S_IFREG)){
        newInode->size = 0;
    }else{
        newInode->size = BLOCK_SIZE;
    }
    newInode->ptrs[0] = dataBlock;
    for(int i = 1; i < N_DIRECT; i++){
        newInode->ptrs[i] = 0;
    }
    newInode->indir_1 = 0;
    newInode->indir_2 = 0;

    assert(block_write(inodes, (1+superBlock.in_map_len+superBlock.blk_map_len), superBlock.inodes_len) == 0);
    //Update the parent dir
    dent[dirIndex].valid = 1;
    dent[dirIndex].inode = iNodeBlock;
    strcpy(dent[dirIndex].name, tokens[n_tokens - 1]);
    assert(block_write(&dent, blockNum, 1) == 0);
    //Update the data block
    char tempBlock[BLOCK_SIZE];
    memset(tempBlock, 0, sizeof(tempBlock));
    assert(block_write(&tempBlock, dataBlock, 1) == 0);

    return 0;


}
int lab3_create(const char *path, mode_t mode, struct fuse_file_info *fi){
    return createHelper(path, mode | S_IFREG);
}

int lab3_mkdir(const char *path, mode_t mode){
    return createHelper(path, mode | S_IFDIR);
}

int lab3_chmod(const char *path, mode_t new_mode, struct fuse_file_info *fi){
    //Parse path
    char *tokens[PATH_MAX];
    char linebuf[1024];
    int n_tokens = split_path(path,PATH_MAX,tokens,linebuf,sizeof(linebuf));
    //Get inode number
    int inum = findInodeNumber(n_tokens,tokens);
    if(inum < 0){
        return inum;
    }
    assert(inum > 0);

    //Read the block associated with the inode
    struct fs_inode *in = &inodes[inum];
    in->mode = (in->mode & S_IFMT) | new_mode;
    assert(block_write(inodes, (1+superBlock.in_map_len+superBlock.blk_map_len), superBlock.inodes_len) == 0);

    return 0;
}

int lab3_write(const char *path, const char *buf, size_t len, off_t offset,
               struct fuse_file_info *fi){
    //Parse path
    char *tokens[PATH_MAX];
    char linebuf[1024];
    int n_tokens = split_path(path,PATH_MAX,tokens,linebuf,sizeof(linebuf));
    //Get inode number
    int inum = findInodeNumber(n_tokens,tokens);
    if(inum < 0){
        return inum;
    }
    assert(inum > 0);

    //Read the block associated with the inode
    struct fs_inode *in = &inodes[inum];

    //Check if directory. If true return -EISDIR
    if(S_ISDIR(in->mode)){
        return -EISDIR;
    }

    int totalSize = offset + len;
    if(totalSize > in->size){
        //Need to allocate new blocks for space
        int requiredBlockCount = div_round_up(totalSize, BLOCK_SIZE);
        int existingBlockCount = div_round_up(in->size, BLOCK_SIZE);

        int actualBlocksNeeded = requiredBlockCount - existingBlockCount;

        if(actualBlocksNeeded > 0){
            int blockNumbers[actualBlocksNeeded];
            for(int i = 0; i < actualBlocksNeeded; i++){
                int blockNum = allocateBlock(1);
                if(blockNum < 0){
                    return -ENOSPC;
                }
                assert(blockNum > 0);
                blockNumbers[i] = blockNum;
            }
            for(int i = 0; i < requiredBlockCount; i++){
                if(i >= existingBlockCount){
                    int ret = setBlock(in, i, blockNumbers[i - existingBlockCount]);
                    if(ret == -1){
                        return -ENOSPC;
                    }
                }
            }
        }
    }
    in->size = totalSize;
    in->mtime = time(NULL);
    assert(block_write(inodes, (1+superBlock.in_map_len+superBlock.blk_map_len), superBlock.inodes_len) == 0);

    //Use the same read logic
    //Get the start and end block indexes
    int blockStartIndex = offset / BLOCK_SIZE;
    int blockEndIndex = totalSize / BLOCK_SIZE;
    if((totalSize%BLOCK_SIZE) == 0){
        blockEndIndex-=1;
    }
    if(blockEndIndex == 300){
        blockEndIndex = 299;
    }

    assert(blockEndIndex >= 0);
    assert(blockEndIndex >= blockStartIndex);

    int bytesLeftToWrite = len;
    int startByteRead = offset % BLOCK_SIZE;
    int bufferLoc = 0;


    for(int i= blockStartIndex; i<= blockEndIndex; i++){
        int blockNumber = getBlock(in,i);
        char tempBlock[BLOCK_SIZE];
        assert(block_read(&tempBlock, blockNumber, 1) == 0);

        int copyLength = BLOCK_SIZE - startByteRead;
        if(copyLength >= bytesLeftToWrite){
            copyLength = bytesLeftToWrite;
        }
        memcpy(&tempBlock[startByteRead], buf + bufferLoc,copyLength);
        assert(block_write(&tempBlock, blockNumber,1) == 0);
        bufferLoc +=copyLength;
        startByteRead = 0;
        bytesLeftToWrite -= copyLength;
    }
    assert(bytesLeftToWrite == 0);

    return len;
}
int unlinkHelper(const char *path){
    //Parse path
    char *tokens[PATH_MAX];
    char linebuf[1024];
    int n_tokens = split_path(path,PATH_MAX,tokens,linebuf,sizeof(linebuf));


    //Get the parent's inode and check if it is a directory
    int inum = findInodeNumber(n_tokens - 1,tokens);
    if(inum < 0){
        return inum;
    }
    assert(inum > 0);

    struct fs_inode *parent = &inodes[inum];
    if(!S_ISDIR(parent->mode)){
        return -ENOTDIR;
    }
    parent->mtime = time(NULL);

    assert(block_write(inodes, (1+superBlock.in_map_len+superBlock.blk_map_len), superBlock.inodes_len) == 0);

    for(int i = 0; i < (parent->size)/BLOCK_SIZE ; i++){
        bool foundFlag = false;
        int blockNum = getBlock(parent,i);
        struct fs_dirent dent[NUM_DIRENT];
        assert(block_read(&dent,blockNum,1) == 0);
        for(int i= 0 ; i< NUM_DIRENT; i++){
            if(dent[i].valid == 1 && strcmp(tokens[n_tokens-1],dent[i].name) == 0){
                dent[i].valid = 0;
                assert(block_write(&dent, blockNum, 1) == 0);
                foundFlag = true;
                break;
            }
        }
        if(foundFlag){
            break;
        }
    }

    return 0;
}
int lab3_unlink(const char *path){
    //Parse path
    char *tokens[PATH_MAX];
    char linebuf[1024];
    int n_tokens = split_path(path,PATH_MAX,tokens,linebuf,sizeof(linebuf));
    //Get inode number
    int inum = findInodeNumber(n_tokens,tokens);
    if(inum < 0){
        return inum;
    }
    assert(inum > 0);

    //Read the block associated with the inode
    struct fs_inode *in = &inodes[inum];
    //Check if directory. If true return -EISDIR
    if(S_ISDIR(in->mode)){
        return -EISDIR;
    }

    for(int i = 0; i < (in->size)/BLOCK_SIZE ; i++){
        int blockNum = getBlock(in,i);
        if(blockNum != 0){
            bit_clear(blockBitmap, blockNum);
        }
    }
    bit_clear(inodeBitmap, inum);
    assert(unlinkHelper(path) == 0);
    return 0;
}
int lab3_rmdir(const char *path){
    //Parse path
    char *tokens[PATH_MAX];
    char linebuf[1024];
    int n_tokens = split_path(path,PATH_MAX,tokens,linebuf,sizeof(linebuf));
    //Get inode number
    int inum = findInodeNumber(n_tokens,tokens);
    if(inum < 0){
        return inum;
    }
    assert(inum > 0);

    //Read the block associated with the inode
    struct fs_inode *in = &inodes[inum];
    //Check if directory. If not return -ENOTDIR
    if(!S_ISDIR(in->mode)){
        return -ENOTDIR;
    }
    for(int i = 0; i < (in->size)/BLOCK_SIZE ; i++){
        int blockNum = getBlock(in,i);
        struct fs_dirent dent[NUM_DIRENT];
        assert(block_read(&dent,blockNum,1) == 0);
        for(int i= 0 ; i< NUM_DIRENT; i++){
            if(dent[i].valid == 1){
                return -ENOTEMPTY;
            }
        }
    }
    for(int i = 0; i < (in->size)/BLOCK_SIZE ; i++){
        int blockNum = getBlock(in,i);
        struct fs_dirent dent[NUM_DIRENT];
        assert(block_read(&dent,blockNum,1) == 0);
        for(int i= 0 ; i< NUM_DIRENT; i++){
            bit_clear(blockBitmap,blockNum);
        }
    }
    bit_clear(inodeBitmap,inum);
    assert(unlinkHelper(path) == 0);
    return 0;
}

int lab3_rename(const char *src_path, const char *dst_path, unsigned int flags){
    char *tokens1[PATH_MAX];
    char linebuf1[1024];
    int n_tokens1 = split_path(src_path,PATH_MAX,tokens1,linebuf1,sizeof(linebuf1));
    char *tokens2[PATH_MAX];
    char linebuf2[1024];
    int n_tokens2 = split_path(dst_path,PATH_MAX,tokens2,linebuf2,sizeof(linebuf2));
    //Get inode number
    int sourceInum = findInodeNumber(n_tokens1 -1,tokens1);
    int destInum = findInodeNumber(n_tokens2 - 1,tokens2);
    //Check if the parent exists
    if(sourceInum < 0 || destInum < 0){
        return -ENOENT;
    }
    //Check if source and dest parent are same
    if(sourceInum != destInum){
        return -EINVAL;
    }

    //Get the source inode
    struct fs_inode *sourceInode = &inodes[sourceInum];
    //Check if directory. If not return -ENOTDIR
    if(!S_ISDIR(sourceInode->mode)){
        return -ENOTDIR;
    }
    //Check if the destination already exists
    for(int i = 0; i < (sourceInode->size)/BLOCK_SIZE ; i++){
        int blockNum = getBlock(sourceInode,i);
        struct fs_dirent dent[NUM_DIRENT];
        assert(block_read(&dent,blockNum,1) == 0);
        for(int i= 0 ; i< NUM_DIRENT; i++){
            if(dent[i].valid == 1 && strcmp(dent[i].name,tokens2[n_tokens2-1]) == 0){
                return -EEXIST;
            }
        }
    }
    //Find the source Index and block
    int sourceIndex = -1;
    int blockNum = -1;
    struct fs_dirent dent[NUM_DIRENT];
    for(int i = 0; i < (sourceInode->size)/BLOCK_SIZE ; i++){
        bool sourceFound = false;
        blockNum = getBlock(sourceInode,i);
        assert(block_read(&dent,blockNum,1) == 0);
        for(int i= 0 ; i< NUM_DIRENT; i++){
            if(dent[i].valid == 1 && strcmp(dent[i].name,tokens1[n_tokens1-1]) == 0){
                sourceIndex = i;
                sourceFound = true;
                break;
            }
        }
        if(sourceFound){
            break;
        }
    }
    if(sourceIndex == -1){
        return -ENOENT;
    }
    assert(blockNum > 0);

    strcpy(dent[sourceIndex].name, tokens2[n_tokens2-1]);
    assert(block_write(&dent, blockNum, 1) == 0);

    return 0;
}
int lab3_truncate(const char *path, off_t new_len, struct fuse_file_info *fi){
    if(new_len != 0){
        return -EINVAL;
    }
    //Parse path
    char *tokens[PATH_MAX];
    char linebuf[1024];
    int n_tokens = split_path(path,PATH_MAX,tokens,linebuf,sizeof(linebuf));
    //Get inode number
    int inum = findInodeNumber(n_tokens,tokens);
    if(inum < 0){
        return inum;
    }
    assert(inum > 0);

    //Read the block associated with the inode
    struct fs_inode *in = &inodes[inum];
    //Check if directory. If true return -EISDIR
    if(S_ISDIR(in->mode)){
        return -EISDIR;
    }
    //Set the size of inode as 0
    in->size = 0;

    //Set all the blocks other than the first block
    for(int i = 1; i < (in->size)/BLOCK_SIZE ; i++){
        int blockNum = getBlock(in,i);
        bit_clear(blockBitmap, blockNum);
        setBlock(in,blockNum,0);
    }

    int firstBlock = getBlock(in,0);
    assert(firstBlock != 0);
    char tempBlock[BLOCK_SIZE];
    memset(&tempBlock, 0, BLOCK_SIZE);
    assert(block_write(&tempBlock, firstBlock, 1) == 0);
    assert(block_write(inodes, (1+superBlock.in_map_len+superBlock.blk_map_len), superBlock.inodes_len) == 0);

    return 0;
}

/* for read-only version you need to implement:
 * - lab3_init
 * - lab3_getattr
 * - lab3_readdir
 * - lab3_read
 *
 * for the full version you need to implement:
 * - lab3_create
 * - lab3_mkdir
 * - lab3_unlink
 * - lab3_rmdir
 * - lab3_rename
 * - lab3_chmod
 * - lab3_truncate
 * - lab3_write
 */

/* operations vector. Please don't rename it, or else you'll break things
 * uncomment fields as you implement them.
 */
struct fuse_operations fs_ops = {
    .init = lab3_init,
    .getattr = lab3_getattr,
    .readdir = lab3_readdir,
    .read = lab3_read,
    .utimens = lab3_utimens,
    .create = lab3_create,
    .mkdir = lab3_mkdir,
    .unlink = lab3_unlink,
    .rmdir = lab3_rmdir,
    .rename = lab3_rename,
    .chmod = lab3_chmod,
    .truncate = lab3_truncate,
    .write = lab3_write,
};

