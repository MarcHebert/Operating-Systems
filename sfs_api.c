//Marc Hebert
//260574038

#include "constants.h"
#include "types.h"
#include "diskemu.h"
#include "superblock.h"
#include "filedescriptortable.h"
#include "directory.h"
#include "inode.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char defaultname[] = "MarcFS";
int errorstatus = 0;

sblock* sb;
icache* ic;
FDB* fd;
directory* d;

//shorcut/aliases
int write_1block(int block, char* buffer)
{
	return write_blocks(block, 1, buffer);
}

int read_1block(int block, char* buffer)
{
	return read_blocks(block, 1, buffer);
}


//initialize functions//
int init_superblock()
{
	return write_1block(SB_BLOCK_, sb =s_init());
}

int init_directory()
{
	d = d_initDir();
}

int init_icache()
{
	ic = i_initCache()
}



//regular
int sfs_open(char* filename)
{
	//check FDT or inodes to see if file exists
	//if DNE, create file & set size to 0
	// else open in append mode ( set file pointer to EOF using sfs_seek)
	
	
	//does file exist?
	int index = d_name2Index(filename);
	if(index<0)//file doesn't exist
	{
		//create the new file
		int id = i_newEntry(); //index is inode in this case
		if(id<0)//any open directory slots?
			return -1;
		d_addEntry(id, filename);
		f_activate(id);
		
	}
	else//file exists
	{
		int id = d_getInode(index)
		f_activate(id);
		f_setRW(id, i_getSize(id))//set RW pointer to end of file (append)
		
	}

	return id; //inode


	/*
	int inode = get_inode(filename);
	int nextfile = find_next_free_slot(open_file_table);
	make_open_file_table_entry(nextfile, inode);
	int filedesc = find_next_free_slot(file_desc_table);
	int read_pointer = 0;
	int write_pointer = 0;
	make_file_desc_entry(filedesc, nextfile, read_pointer, write_pointer)

	return filedesc;
	*/
}



//write//

int num_new_blocks_needed(int inode, int newBytes, int usedDataBlocks)
{
	return ((f_getRW(inode)+newBytes) - (usedDataBlocks * BLOCKSIZE_)) / BLOCKSIZE_ + 1;
}

int write_partblock(int block, const char* buffer, int byteOffset, int numBytes)
{
	//load block to write to
	char* readbuf = (char*) malloc(sizeof(char) * BLOCKSIZE_);
	read_1block(block,readbuf);

	//write where need be
	memcpy(byteOffset + readbuf, buffer, numBytes);//strncpy or memcpy? TBD
	//store modified block back
	write_1block(block, readbuf);
	free(readbuf);
} 

int num_indptrs(int inode)//number of used blocks pointed to by indirect data block
{
	int filesize = i_getSize(inode);//fetch filesize from icache
	int bytesBeforeInd = NUM_DIR_DATABLOCKS_ * BLOCKSIZE_;
	if (bytesBeforeInd>filesize)//no indirect block ptrs used
	{
		return 0;
	}
	else
	{
		return (filesize - bytesBeforeInd)/ NUM_PTRS_INDIR_ +1;
	} 
}

int load_indblock(int inode, char* buffer)//loads into indirect data block into buffer
{
	return read_blocks(i_getIndPointer(inode), 1, buffer);
}

int load_blocknums(int inode, int* blockNums)//load all block nums into blockNums array and returns total blocks used
{
	int numIndPtrs, numDirPtrs, x;
	

	//check if ind is used and if so how much
	if(numIndPtrs = num_indptrs(int inode)>0)//ind is used
	{
		char* buffer = (char*)malloc(BLOCKSIZE_);
		load_indblock(inode, buffer);

		numDirPtrs = NUM_DIR_DATABLOCKS_;

		//copy block nums from direct blocks
		for(x = 0; x < numDirPtrs;x++)
		{
			blockNums[x] = i_getPointer(inode, x);
		}
		//copy block nums from indirect block and attach to blockNums array
		memcpy((blockNums+ numDirPtrs), buffer, sizeof(int) * numIndPtrs);
		free(buffer);
	}
	else//just direct pointers used
	{
		numDirPtrs = (i_getSize(inode)/ BLOCKSIZE_);//normally round up by 1 but it's upper bound on array
		for(x = 0; x< numDirPtrs;x++)
		{
			blockNums[x] = i_getPointer(inode, x);
		}
	}
	return numDirPtrs + numIndPtrs;
}

//does most of heavy lifting required by sfs_fwrite()
int write_helper(const char* buffer, int numBytes, int* blockNums, int offset)
{
	int x, writtenBytes =0; 
	int remainingBytes, veryLastBytes;

	//copy buffer
	char* readbuf = (char*) malloc(numBytes * sizeof(char*));
	memcpy(readbuf,buffer, numBytes);

	char* bufptr = readbuf;;// starts at head, iterates to every block addr

	//get number of bytes until next block if necessary else just remaining bytes
	remainingBytes = min(numBytes, BLOCKSIZE_ - offset);

	//write first block
	write_partblock(blockNums[x], bufptr, offset, remainingBytes);
	x++;
	writtenBytes = writtenBytes + remainingBytes;
	bufptr = bufptr + remainingBytes;
	//done

	//write remaining blocks
	while(BLOCKSIZE_ < numBytes - writtenBytes)
	{
		write_1block(blockNums[x], bufptr);
		
		//increment to next block
		x++;
		bufptr = bufptr + BLOCKSIZE_;
		writtenBytes = writtenBytes + BLOCKSIZE_;
	}

	//write possible ending partial block
	if(veryLastBytes = numBytes - writtenBytes > 0)
	{
		write_partblock(blockNums[x], bufptr, 0, veryLastBytes);
		writtenBytes = writtenBytes + veryLastBytes;
	}
	return writtenBytes;
}


int sfs_fwrite(int fileID, const char *buf, int length)
{
	//TODO
}

int sfs_fread(int fileID, char *buf, int length);
int sfs_fseek(int fileID, int offset)
{

	f_incdecRW(fileID, offset);//TODO add error handling
}

int sfs_remove(char *file)
{
	int index = d_name2Index(file);
	if(index<0)
		return -1;//file doesn't exist
	//will possibly change to not return error and instead do nothing
	int id = d_getInode(index);
	i_deleteEntry(id);// TODO error handle here
	//TODO release data blocks used by the file
	return d_removeEntry(index);
}

int sfs_get_next_filename(char* filename)
{
	if (d_getNextDirName(filename)<0)
	{
		return 0;
	}
	else
		return 1;
	/*
	int nextfile = d.list[dirIterIndx].active//check if next entry has file
	if (nextfile !=0)
	{
		strcpy(filename, d.list[dirIterIndx].fname);
		dirIterIndx++;
		if (dirIterIndx>=NUM_INODES_)//if last file possible
		{
			dirIterIndx = 0;
			nextfile = 0;
		}
	}
	else
	{
		dirIterIndx = 0;//if no more files, reset directory index to 0
	}
	return nextfile;
	*/
}
int sfs_GetFileSize(const char* path)
{
	//assuming path is name? otherwise will have to come back and fix
	/*char* name = null;
	while(strcmp(path,name)!=0)
	{
		if(sfs_get_next_filename(name)==0)
			//file doesn't exist
			return 0;
	}
	*/
	//fetch inode from dir and then pull size from cache
	int index = d_name2Index(path);
	if(index<0)//file doesn't exist
		return -1;
	return i_getSize(d_getInode(index));
}

int sfs_fclose(int fileID)
{
	f_deactivate(fileID);
}

int mksfs(int fresh)
{
	if(fresh==0)//new file system
	{
		errorstatus = init_fresh_disk(defaultname, BLOCKSIZE, NUM_BLOCKS);

		//setup directory
		d_initDir();

		//setup inodetable
		i_initCache();

		//write superblock to disk
		errorstatus = 

	}

	else//existing file system
	{
		errorstatus = init_disk(defualtname, BLOCKSIZE, NUM_BLOCKS);
		
		//read superblock
		void buf;
		read_blocks(0,1, (void*)&sb);
	}
	return errorstatus;
}