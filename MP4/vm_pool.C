/*
 File: vm_pool.C
 
 Author:
 Date  : 2024/09/20
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
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

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    
    // Fill in the parameters
	base_address = _base_address;
	size = _size;
	frame_pool = _frame_pool;
	page_table = _page_table;

	vm_pool_next_ptr = nullptr;
	num_vmem_regions = 0;
	
	// Register the pool
	page_table->register_pool(this);
	
	// 1st Entry
	AllocRegion* region_ptr = (AllocRegion*)base_address;

	region_ptr[0].base_address = base_address;
	region_ptr[0].size = PageTable::PAGE_SIZE;
	alloc_regions = region_ptr;
	
	// Number of regions should be increased by 1 now
	num_vmem_regions += 1;
	
	// Available amount of virtual memory
	available_mem = available_mem - PageTable::PAGE_SIZE;
	
    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
	
	// Requested more memory than available
	if( _size > available_mem ) {
		Console::puts("VMPOOL: Allocation failed. Not enough memory.\n");
		assert(0);
	}

	unsigned long allocated_pages {0};

	// Size of pages to allocate
	allocated_pages = (_size + PageTable::PAGE_SIZE - 1) / PageTable::PAGE_SIZE; // Works since the numbers are positive

	// Directory entry for allocated region - based on the previous vm_region 
	alloc_regions[num_vmem_regions].base_address = alloc_regions[num_vmem_regions-1].base_address +  alloc_regions[num_vmem_regions-1].size;
	alloc_regions[num_vmem_regions].size = allocated_pages*PageTable::PAGE_SIZE;
	
	// Number of regions should be increased by 1 now
	num_vmem_regions += 1;
	
	// Available amount of virtual memory
	available_mem -= allocated_pages * PageTable::PAGE_SIZE;

    Console::puts("Allocated region of memory.\n");

	// return (allocated base addr)
	return alloc_regions[num_vmem_regions-1].base_address;
}

void VMPool::release(unsigned long _start_address) {
	int index = 0;
	int region_no = -1;
	unsigned long pages_to_free {0};
	
	// Identify the region
	for(index = 1; index < num_vmem_regions; ++index) {
		if(alloc_regions[index].base_address  == _start_address ) {
			region_no = index;
		}
	}
	if(region_no == -1) {
        Console::puts("Region not found.\n");
        assert(0);
    }
	// Pages to free
	pages_to_free = alloc_regions[region_no].size / PageTable::PAGE_SIZE;
	while(pages_to_free > 0) {
		// Free the page

        //Console::puts("Freeing Page at address: ");
        //Console::putui(_start_address);
        //Console::puts("\n");
		page_table->free_page(_start_address);
		
		// Freeing done, should change start address now
		_start_address += PageTable::PAGE_SIZE;
		
        // Onto next
		pages_to_free--;
	}
	
	// Free the information of regions
	for( index = region_no; index < num_vmem_regions; ++index) {
		alloc_regions[index] = alloc_regions[index+1];
	}
	
	// Recompute available memory
	available_mem += alloc_regions[region_no].size;
	
	// Recompute the number of vmem regions
	num_vmem_regions--;
    
    //Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    //Console::puts("Checking whether address is part of an allocated region.\n");    
    return (_address >= base_address) && (_address <= (base_address + size));
}

