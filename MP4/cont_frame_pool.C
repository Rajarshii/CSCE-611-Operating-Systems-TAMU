/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

ContFramePool* ContFramePool::head = nullptr;

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{
    /**
     * Assuming a 32KiB Memory
     */
    assert(_n_frames <= FRAME_SIZE * 8);    

    base_frame_no = _base_frame_no;
    framePoolSize = _n_frames;
    numFreeFrames = _n_frames;
    info_frame_no = _info_frame_no;    
    
    /**
     * If _info_frame_no is zero then we keep management info in the first
     * frame, else we use the provided frame to keep management info
     */
    if(info_frame_no == 0) {
        frameStateBitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        frameStateBitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }
    
    /**
     * Everything ok. Proceed to mark all frame as free. 
     */ 
    for(int fno = 0; fno < _n_frames; fno++) {
        set_state(fno, FrameState::Free);
    }
    
    /**
     * Mark the first frame as being used if it is being used
     */
    if(_info_frame_no == 0) {
        set_state(0, FrameState::Used);
        numFreeFrames -= 1;
    }
    
    /**
     * Updating the frame pool linked-list.
     */
    if(head == nullptr) {
        head = this;
        head->next = nullptr;
    } else {
        ContFramePool* current = head;
        while(current->next != nullptr) {
            current = current->next;
        }
        current->next = this;
        this->next = nullptr;
    }


    /**
     * Done doing stuff.
     */
    
    Console::puts("Frame Pool initialized\n");    
}

/**
 * Returns the frame state.
 */
ContFramePool::FrameState 
ContFramePool::get_state(unsigned long _frame_no)
{
    unsigned int bitmap_row = (_frame_no >> 2);           // Find the memory row
    unsigned int bitmap_col = (_frame_no & 0b11) << 1;    // Find the memory column
    unsigned char state_bits = (frameStateBitmap[bitmap_row] >> bitmap_col) & 0b11;
    switch(state_bits) {
        case 0b00: return ContFramePool::FrameState::Free;
        case 0b01: return ContFramePool::FrameState::Used;
        case 0b10: return ContFramePool::FrameState::HoS;
        default:
            Console::puts("GET_STATE: Invalid State:"); Console::puti(state_bits);
            assert(0);
            return ContFramePool::FrameState::Free;
    }
}

/**
 * Set the state of a frame.
 */
void 
ContFramePool::set_state(unsigned long _frame_no, ContFramePool::FrameState _state) 
{
    unsigned int bitmap_row = (_frame_no >> 2);           // Find the memory row
    unsigned int bitmap_col = (_frame_no & 0b11) << 1;    // Find the memory column
    //unsigned int clear_mask = ~(3 << bitmap_col);
    frameStateBitmap[bitmap_row] &= ~(3 << bitmap_col);
    switch(_state) {
        case ContFramePool::FrameState::Free: 
            // Already cleared - can exit
            break;
        case ContFramePool::FrameState::Used: 
            frameStateBitmap[bitmap_row] |= (1 << bitmap_col);
            break;
        case ContFramePool::FrameState::HoS: 
            frameStateBitmap[bitmap_row] |= (2 << bitmap_col);
            break;
        default:
            Console::puts("SET_STATE: Invalid State");
            assert(0);
    }    
}


unsigned long 
ContFramePool::get_frames(unsigned int _n_frames)
{
    /**
     * Assert protection
     */
    if((numFreeFrames < 0) || (_n_frames > numFreeFrames) || (_n_frames > framePoolSize)) {
        Console::puts("GET_FRAMES: Memory Allocation Failed!");
        Console::puts("\nnumFreeFrames: "); Console::puti(numFreeFrames);
        Console::puts("\n_n_frames: "); Console::puti(_n_frames);
        Console::puts("\nframePoolsize: "); Console::puti(framePoolSize);
        Console::puts("\nExit\n");
        assert(0);
    }
    
    /**
     * Find allocatable pool
     */

    unsigned int contBlockSize {0};
    unsigned long contFrameStart {0};
    bool allocatable {false};

    for(unsigned long fno = 0; fno < framePoolSize; ++fno) {
        if(get_state(fno) == ContFramePool::FrameState::Free) {
            if(contFrameStart == 0) {
                contFrameStart = fno;
            }
            ++contBlockSize;
            if(contBlockSize == _n_frames) {
                allocatable = true;
                break;
            }
        }
        else {
            contFrameStart = 0;
        }
    }

    /**
     * Unable to allocate pages - return
     */
    if(allocatable == false) {
        Console::puts("GET_FRAMES: Memory Allocation Failed!\n");
        return 0;
    }

    set_state((contFrameStart), ContFramePool::FrameState::HoS);
    for(int fno = contFrameStart + 1; fno < contFrameStart + _n_frames ; ++fno) {
        set_state(fno , ContFramePool::FrameState::Used);
    }

    numFreeFrames -= _n_frames;
    return (base_frame_no + contFrameStart);
}

void 
ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    /**
     * Check sanity
     */
    if((_base_frame_no < base_frame_no) || 
        ( (_base_frame_no + _n_frames) > (base_frame_no + framePoolSize))) {
            Console::puts("MARK_INACCESSIBLE: Out of Bound Error\n");
            assert(0);
    }
    
    unsigned long offset_base_frame_no = _base_frame_no - this->base_frame_no;    

    // Mark all frames in the range as being used.

    for(unsigned long fno = 0; fno < _n_frames; ++fno) {
        
        ContFramePool::FrameState frame_state = get_state(offset_base_frame_no + fno);

        if(frame_state == ContFramePool::FrameState::Free) {
            if(fno == 0) {
                set_state(offset_base_frame_no, FrameState::HoS);
                //Console::puts("HoS at:") ; Console::puti(offset_base_frame_no);
            }
            else {
                set_state(offset_base_frame_no + fno, FrameState::Used);
                //Console::puts("\nOthers at:") ; Console::puti(offset_base_frame_no + fno);
            }
            
            numFreeFrames--;
        } 
        else {
            Console::puts("MARK_INACCESSIBLE: Frame In Use\n");
            assert(0);
        }
    }    
}

void 
ContFramePool::release_frames(unsigned long _first_frame_no)
{
    ContFramePool* current = head;
    ContFramePool* previous = nullptr;
    bool frame_exists {false};
    /**
     * Iterate over all frame pools and link them
     */
    while(current != nullptr) {
        if((_first_frame_no >= current->base_frame_no) && 
            (_first_frame_no < current->base_frame_no + current->framePoolSize)) {
                current->release_frame_pool(_first_frame_no);
                frame_exists = true;
                break;
        }
        current = current->next;
    }

    if(frame_exists == false) {
        Console::puts("RELEASE_FRAMES: Frame Requested for Release Does Not Exists! Requested Frame: ");
        Console::putui(_first_frame_no);
        Console::puts("\n");
        assert(0);
    } 
}

void
ContFramePool::release_frame_pool(unsigned long _first_frame_no) {
    if(get_state(_first_frame_no - base_frame_no) != ContFramePool::FrameState::HoS) {
        Console::puts("RELEASE_FRAMES: Incorrect HoS state\n");
        assert(0);
    }
    /**
     * Instead of freeing the entire pool, free only those frames that are not free.
     */
    set_state(_first_frame_no, ContFramePool::FrameState::Free);
    numFreeFrames++;
    unsigned long fno = _first_frame_no + 1;
    while(get_state(fno - base_frame_no) != ContFramePool::FrameState::Free) {
        set_state(fno, ContFramePool::FrameState::Free);			
        numFreeFrames++; fno++;
    }
}

unsigned long 
ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    unsigned long num_info_frames {0};
    //num_info_frames = ((2*_n_frames) / (32*1024)) + (((2*_n_frames) % (32*1024)) > 0 ? 1 : 0); 
    num_info_frames = (_n_frames*2 + 32*1024 - 1) / (32*1024);
    return num_info_frames;
}
