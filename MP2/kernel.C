/*
    File: kernel.C

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 2024/08/20


    This file has the main entry point to the operating system.

*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MB * (0x1 << 20)
#define KB * (0x1 << 10)
/* Makes things easy to read */

#define KERNEL_POOL_START_FRAME ((2 MB) / (4 KB))
#define KERNEL_POOL_SIZE ((2 MB) / (4 KB))
#define PROCESS_POOL_START_FRAME ((4 MB) / (4 KB))
#define PROCESS_POOL_SIZE ((28 MB) / (4 KB))
/* Definition of the kernel and process memory pools */

#define MEM_HOLE_START_FRAME ((15 MB) / (4 KB))
#define MEM_HOLE_SIZE ((1 MB) / (4 KB))
/* We have a 1 MB hole in physical memory starting at address 15 MB */

#define TEST_START_ADDR_PROC (4 MB)
#define TEST_START_ADDR_KERNEL (2 MB)
/* Used in the memory test below to generate sequences of memory references. */
/* One is for a sequence of memory references in the kernel space, and the   */
/* other for memory references in the process space. */

#define N_TEST_ALLOCATIONS 32
/* Number of recursive allocations that we use to test.  */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "machine.H"     /* LOW-LEVEL STUFF   */
#include "console.H"

#include "assert.H"
#include "cont_frame_pool.H"  /* The physical memory manager */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

void test_memory(ContFramePool * _pool, unsigned int _allocs_to_go);

/*--------------------------------------------------------------------------*/
/* MAIN ENTRY INTO THE OS */
/*--------------------------------------------------------------------------*/

int main() {

    Console::init();
    Console::redirect_output(true); // comment if you want to stop redirecting qemu window output to stdout

    /* -- INITIALIZE FRAME POOLS -- */

    /* ---- KERNEL POOL -- */
    
    ContFramePool kernel_mem_pool(KERNEL_POOL_START_FRAME,
                                  KERNEL_POOL_SIZE,
                                  0);
  
    /* ---- PROCESS POOL -- */

    /*// In later machine problems, we will be using two pools. You may want to uncomment this out and test 
    // the management of two pools.

    unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);

    unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);
    
    ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                   PROCESS_POOL_SIZE,
                                   process_mem_pool_info_frame);*/
    
    //process_mem_pool.mark_inaccessible(MEM_HOLE_START_FRAME, MEM_HOLE_SIZE);
    

    /* -- MOST OF WHAT WE NEED IS SETUP. THE KERNEL CAN START. */

    Console::puts("Hello World!\n");

    /* -- TEST MEMORY ALLOCATOR */
    
    test_memory(&kernel_mem_pool, N_TEST_ALLOCATIONS);

    /* ---- Add code here to test the frame pool implementation. */
    
    // 2. Test process mem pool
    // test_memory(&process_mem_pool, N_TEST_ALLOCATIONS);

    /*
    // 3. Test four process mem pools
    unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE/4);

    unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);

    ContFramePool process_mem_pool0(PROCESS_POOL_START_FRAME,
                                   PROCESS_POOL_SIZE/4,
                                   process_mem_pool_info_frame); 
    ContFramePool process_mem_pool1(PROCESS_POOL_START_FRAME + PROCESS_POOL_SIZE/4,
                                   PROCESS_POOL_SIZE/4,
                                   process_mem_pool_info_frame); 
    ContFramePool process_mem_pool2(PROCESS_POOL_START_FRAME + PROCESS_POOL_SIZE/2,
                                   PROCESS_POOL_SIZE/4,
                                   process_mem_pool_info_frame); 
    ContFramePool process_mem_pool3(PROCESS_POOL_START_FRAME + PROCESS_POOL_SIZE/2 + PROCESS_POOL_SIZE/4,
                                   PROCESS_POOL_SIZE/4,
                                   process_mem_pool_info_frame);

    test_memory(&process_mem_pool0, N_TEST_ALLOCATIONS/8);
    test_memory(&process_mem_pool1, N_TEST_ALLOCATIONS/8);
    test_memory(&process_mem_pool2, N_TEST_ALLOCATIONS/8);
    test_memory(&process_mem_pool3, N_TEST_ALLOCATIONS/8);
    */
   
    /*
    // 4. get_frames : test maximum for kernel frames
    unsigned long all_frames = kernel_mem_pool.get_frames(511);
    */

    /*
    // 5. get_frames : test maximum for kernel and process frames
    unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);

    unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);
    
    ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                   PROCESS_POOL_SIZE,
                                   process_mem_pool_info_frame);     

    unsigned long all_frames = process_mem_pool.get_frames(7168);
    */

    /*
    // 6. Request more than available free frames - kernel

    unsigned long some_frames = kernel_mem_pool.get_frames(32);
    Console::puts("Frames requested successfully - 1!\n");
    unsigned long more_frames = kernel_mem_pool.get_frames(512);
    Console::puts("Frames requested successfully - 2!\n");
    */

   /*
    // 7. Request more than available frames - process
    unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);

    unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);
    
    ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                   PROCESS_POOL_SIZE,
                                   process_mem_pool_info_frame);     

    unsigned long some_frames = process_mem_pool.get_frames(4096);
    Console::puts("Frames requested successfully - 1!\n");

    unsigned long more_frames = process_mem_pool.get_frames(4096);
    Console::puts("Frames requested successfully - 2!\n");
    */

    
    /*// 8. Request frames - but non-contgious frames of requested size are not available
    unsigned long frame_ba1 = kernel_mem_pool.get_frames(32);
    unsigned long frame_ba2 = kernel_mem_pool.get_frames(64);
    unsigned long frame_ba3 = kernel_mem_pool.get_frames(383);
    Console::puts("Allocated\n");
    kernel_mem_pool.release_frames(frame_ba2);
    unsigned long frame_ba4 = kernel_mem_pool.get_frames(96);
    if(frame_ba4 == 0) {
        Console::puts("Could Not allocate frames - 64\n");
    }
    frame_ba4 = kernel_mem_pool.get_frames(32);
    if(frame_ba4 == 0) {
        Console::puts("Could Not allocate frames - 32\n");
    }
    else {
        Console::puts("Allocated frame of size - 32\n");
    }*/

   /*
    // 9. Trying to release frame numbers out of range
    kernel_mem_pool.release_frames(10000);
    */

   /*// 11. Mark frames inaccessible.
    unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);
    unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);

    ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                   PROCESS_POOL_SIZE,
                                   process_mem_pool_info_frame);
    
    process_mem_pool.mark_inaccessible(MEM_HOLE_START_FRAME, MEM_HOLE_SIZE);

    unsigned long frame_base = process_mem_pool.get_frames(7168);*/

    /*// 12. Try to mark used frames inaccessible.

    unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);
    unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);

    ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                   PROCESS_POOL_SIZE,
                                   process_mem_pool_info_frame);

    unsigned long frame_base = process_mem_pool.get_frames(3000);    
    process_mem_pool.mark_inaccessible(MEM_HOLE_START_FRAME, MEM_HOLE_SIZE);*/

    /*// 13. Mark inaccessible - out of bounds error

    unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);
    unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);

    ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                   PROCESS_POOL_SIZE,
                                   process_mem_pool_info_frame);

    process_mem_pool.mark_inaccessible(MEM_HOLE_START_FRAME*4, MEM_HOLE_SIZE);*/

    /*// 14. Unaligned frame release    
    unsigned long frame_ba1 = kernel_mem_pool.get_frames(100);
    kernel_mem_pool.release_frames(514);*/

    /*// 15. Mark inaccessible twice

    unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);
    unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);

    ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                   PROCESS_POOL_SIZE,
                                   process_mem_pool_info_frame);

    process_mem_pool.mark_inaccessible(MEM_HOLE_START_FRAME, MEM_HOLE_SIZE);    
    process_mem_pool.mark_inaccessible(MEM_HOLE_START_FRAME, MEM_HOLE_SIZE);*/    

    /*// 16. needed_info_frames - check the number of info frames required.
    unsigned long n_info_frames = ContFramePool::needed_info_frames(1); 
    Console::puts("\nFrames: "); Console::puti(1);Console::puts(" Info frames needed: "); Console::puti(n_info_frames);
    n_info_frames = ContFramePool::needed_info_frames(512);
    Console::puts("\nFrames: "); Console::puti(512);Console::puts(" Info frames needed: "); Console::puti(n_info_frames);
    n_info_frames = ContFramePool::needed_info_frames(2048);
    Console::puts("\nFrames: "); Console::puti(2048);Console::puts(" Info frames needed: "); Console::puti(n_info_frames);
    n_info_frames = ContFramePool::needed_info_frames(4096);
    Console::puts("\nFrames: "); Console::puti(4096);Console::puts(" Info frames needed: "); Console::puti(n_info_frames);
    n_info_frames = ContFramePool::needed_info_frames(7168);
    Console::puts("\nFrames: "); Console::puti(7168);Console::puts(" Info frames needed: "); Console::puti(n_info_frames); Console::puts("\n");
    */
    /* ---- End of tests for Frame Pool Implementation. ---- */

    /* -- NOW LOOP FOREVER */
    Console::puts("Testing is DONE. We will do nothing forever\n");
    Console::puts("Feel free to turn off the machine now.\n");

    for(;;);

    /* -- WE DO THE FOLLOWING TO KEEP THE COMPILER HAPPY. */
    return 1;
}

void test_memory(ContFramePool * _pool, unsigned int _allocs_to_go) {
    Console::puts("alloc_to_go = "); Console::puti(_allocs_to_go); Console::puts("\n");
    if (_allocs_to_go > 0) {
        // We have not reached the end yet. 
        int n_frames = _allocs_to_go % 4 + 1;               // number of frames you want to allocate
        unsigned long frame = _pool->get_frames(n_frames);  // we allocate the frames from the pool
        int * value_array = (int*)(frame * (4 KB));         // we pick a unique number that we want to write into the memory we just allocated
        for (int i = 0; i < (1 KB) * n_frames; i++) {       // we write this value int the memory locations
            value_array[i] = _allocs_to_go;
        }
        test_memory(_pool, _allocs_to_go - 1);              // recursively allocate and uniquely mark more memory
        for (int i = 0; i < (1 KB) * n_frames; i++) {       // We check the values written into the memory before we recursed 
            if(value_array[i] != _allocs_to_go){            // If the value stored in the memory locations is not the same that we wrote a few lines above
                                                            // then somebody overwrote the memory.
                Console::puts("MEMORY TEST FAILED. ERROR IN FRAME POOL\n");
                Console::puts("i ="); Console::puti(i);
                Console::puts("   v = "); Console::puti(value_array[i]); 
                Console::puts("   n ="); Console::puti(_allocs_to_go);
                Console::puts("\n");
                for(;;);                                    // We throw a fit.
            }
        }
        ContFramePool::release_frames(frame);               // We free the memory that we allocated above.
    }
}

