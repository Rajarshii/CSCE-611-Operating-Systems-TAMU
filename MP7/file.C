/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id) {
    Console::puts("Opening file.\n");
    fs = _fs;
    inode = fs->LookupFile(_id);
    current_position = 0;
    fs->ReadBlock(inode->block_id,block_cache);
}

File::~File() {
    Console::puts("Closing file.\n");
    /* Make sure that you write any cached data to disk. */
    /* Also make sure that the inode in the inode list is updated. */
    fs->WriteBlock(inode->block_id,block_cache);    // Write cached data to disk
    fs->WriteInode();                               // Inode/ Inode list update
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf) {
    Console::puts("reading from file\n");

    unsigned int l_fptr = 0;  // File pointer

    while((l_fptr < _n) && (EoF() == false)) {
        _buf[l_fptr++] = block_cache[current_position++];
        //l_fptr++;
        //current_position++;
    }

    return l_fptr;
}

int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("writing to file\n");

    unsigned int l_fptr = 0;  // File pointer
    
    if((current_position + _n) > inode->file_size) {
        inode->file_size = current_position + _n;

        if(inode->file_size > SimpleDisk::BLOCK_SIZE) { // Maximum File size is 1 Block = 512 Bytes
            inode->file_size = SimpleDisk::BLOCK_SIZE;
        }
    }

    // Keep filling the block cache until end-of-file 
    while(!EoF()) {
        block_cache[current_position++] = _buf[l_fptr++];
    }

    return l_fptr;
}

void File::Reset() {
    Console::puts("resetting file\n");
    current_position = 0;
}

bool File::EoF() {
    //Console::puts("checking for EoF\n");
    return (current_position == inode->file_size) ? true : false;
}
