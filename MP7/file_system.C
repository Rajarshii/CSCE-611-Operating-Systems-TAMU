/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
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
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store 
   inodes from and to disk. */

/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("In file system constructor.\n");

    disk = nullptr;
    size = 0;

    // Create an Inode
    inodes = new Inode[SimpleDisk::BLOCK_SIZE];

    // Create free block list
    free_blocks = new unsigned char[SimpleDisk::BLOCK_SIZE];
}

FileSystem::~FileSystem() {
    Console::puts("unmounting file system\n");
    /* Make sure that the inode list and the free list are saved. */

    WriteInode();

    WriteFreeBlockList();

    delete[] inodes;
    delete[] free_blocks;
}


/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

int FileSystem::GetFreeBlock() {
    for(unsigned int id = 0; id < SimpleDisk::BLOCK_SIZE; ++id) { 
        if(free_blocks[id] == 0) {
            return id;
        }
    }
    return EMPTY_MARKER;
}

short FileSystem::GetFreeInode() {
    for(unsigned int id = 0; id < MAX_INODES; ++id) {
        if(inodes[id].id == EMPTY_MARKER) {
            return id;
        }
    }
    return EMPTY_MARKER;
}

bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system from disk\n");
    /* Here you read the inode list and the free list into memory */

    disk = _disk;

    ReadInode();

    ReadFreeBlockList();

    bool status = false;
    if((free_blocks[0] == 1) && free_blocks[1] == 1) {
        status = true;
    }

    return status;
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) { // static! // TODO
    Console::puts("formatting disk\n");
    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */

    // Sanity Check
    if(_disk == nullptr) {
        return false;
    }

    unsigned char l_cache[SimpleDisk::BLOCK_SIZE];

    // Size of the filesystem
    FILE_SYSTEM->size = _size;

    // Initialize Inode
    for(unsigned int id = 0; id < SimpleDisk::BLOCK_SIZE; ++id) {
        l_cache[id] = EMPTY_MARKER;
    }

    _disk->write(BLOCK_ID_INODE, l_cache);

    // Initialize Free list blocks
    for(unsigned int id = 0; id < SimpleDisk::BLOCK_SIZE; ++id) {
        l_cache[id] = char(0);
    }    

    // Mark the blocks for inodes and free list as used
    l_cache[BLOCK_ID_INODE] = 1;

    l_cache[BLOCK_ID_FREELIST] = 1;

    _disk->write(BLOCK_ID_FREELIST,l_cache);

    return true;
}

Inode * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file with id = "); Console::puti(_file_id); Console::puts("\n");
    /* Here you go through the inode list to find the file. */

    for(unsigned int id = 0 ; id < MAX_INODES; ++id) {
        if(inodes[id].id == _file_id) {
            return &inodes[id];
        }
    }
    Console::puts("requested file does not exist.\n");
    return nullptr;
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */
    
	int free_inode_id = 0;

    short free_block_id = 0;
	
	if(LookupFile(_file_id) != nullptr ) {
		Console::puts("file already exists! file creation failed!\n");
		return false;
    }
    
    // Try to get free block.
    free_block_id = GetFreeBlock();

    if( free_block_id == EMPTY_MARKER) {
		Console::puts("free blocks unavailable!\n");
		return false;	
    }
    
    // Try to get free inode
	free_inode_id = GetFreeInode();

    if( free_inode_id == EMPTY_MARKER) {
		Console::puts("free inodes unavailable!\n");
		return false;	
    }
	
    // Mark Inode and Free block as being used
    free_blocks[free_block_id] = 1;
	inodes[free_inode_id].id = _file_id;
    inodes[free_inode_id].block_id = free_block_id;	
	inodes[free_inode_id].file_size = 0;
    inodes[free_inode_id].fs = this;
	
    // Finally write the inode and free block list 
    WriteInode();

	WriteFreeBlockList();

    return true;
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* First, check if the file exists. If not, throw an error. 
       Then free all blocks that belong to the file and delete/invalidate 
       (depending on your implementation of the inode list) the inode. */

	Inode * inode = LookupFile(_file_id);

	if(inode != nullptr) {

		free_blocks[inode->block_id] = 0;
		
		inode->id = EMPTY_MARKER;
		inode->file_size = EMPTY_MARKER;
		inode->block_id = EMPTY_MARKER;
		
		WriteInode();
		
        WriteFreeBlockList();
		
		return true;
	}
	else {
		return false;
	}
}

// Utility Read Functions
void FileSystem::ReadInode() {
	disk->read(BLOCK_ID_INODE, (unsigned char*) inodes);
}

void FileSystem::ReadFreeBlockList() {
	disk->read(BLOCK_ID_FREELIST, free_blocks);
}

void FileSystem::ReadBlock(unsigned long block_id, unsigned char* _cache) {
	disk->read(block_id, _cache);
}

// Utility Write Functions
void FileSystem::WriteInode() {
	disk->write(BLOCK_ID_INODE, (unsigned char*) inodes);
}

void FileSystem::WriteFreeBlockList() {
	disk->write(BLOCK_ID_FREELIST, free_blocks);
}

void FileSystem::WriteBlock(unsigned long block_id, unsigned char* _cache) {
	disk->write(block_id, _cache);
}