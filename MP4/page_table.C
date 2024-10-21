#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = nullptr;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = nullptr;
ContFramePool * PageTable::process_mem_pool = nullptr;
unsigned long PageTable::shared_size = 0;
VMPool* PageTable::vm_pool_hptr = nullptr;

void 
PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   PageTable::kernel_mem_pool = _kernel_mem_pool;
   PageTable::process_mem_pool = _process_mem_pool;
   PageTable::shared_size = _shared_size;

   if(DEBUGGER_EN) Console::puts("Page Table Initialization: DONE.\n");
}

PageTable::PageTable()
{
   paging_enabled = 0; // Ensure paging is disabled

    // Shared page size
    unsigned long shared_pages = PageTable::shared_size / PAGE_SIZE;

    // Allocate Single Frame to First Level - Page Directory
    page_directory = (unsigned long*) (kernel_mem_pool->get_frames(1) * PAGE_SIZE);

    // Allocate Single Frame to Second Level - Page Table
    unsigned long* page_table_ptr = (unsigned long*) (process_mem_pool->get_frames(1) * PAGE_SIZE);

    // Update the First Page Directory Entry and mark the remaining as invalid in directory
    page_directory[0] = (unsigned long)page_table_ptr | RW_MASK_EN | VALID_MASK_EN;

    // Since we use recursive PT lookup, point the last entry to the front of the Page Dir.
    page_directory[shared_pages - 1] = (unsigned long)page_directory | RW_MASK_EN | VALID_MASK_EN;

    unsigned long idx {0};

    for(idx = 1; idx < shared_pages - 1; ++idx) {
        page_directory[idx] = (unsigned long) 0 | RW_MASK_EN;
    }

    // Setup direct mapped pages
    unsigned long page_address {0};

    // Shared memory stuff
    for(idx = 0; idx < shared_pages; ++idx, page_address+=PAGE_SIZE) {
        page_table_ptr[idx] = page_address | RW_MASK_EN | VALID_MASK_EN;
    }
   if(DEBUGGER_EN) Console::puts("Setup Page.\n");
}


void 
PageTable::load()
{
   current_page_table = this;
   write_cr3((unsigned long)(current_page_table->page_directory)); // Reference: osdever.net
   if(DEBUGGER_EN) {Console::puts("Page Table Loaded!\n");}
}

void 
PageTable::enable_paging()
{
   write_cr0(read_cr0() | 0x80000000); // Reference: osdever.net 
   paging_enabled = 1;
   if(DEBUGGER_EN) {Console::puts("Paging Enabled!\n");}
}

void 
PageTable::handle_fault(REGS * _r)
{
   if(DEBUGGER_EN) {Console::puts("PAGE_FAULT_HANDLER: Handling Page Fault.\n");}
   
   // Extract the error code
	unsigned long error_code = _r->err_code;

	if( (error_code & 0x1) == 0) { // Page not present
        //Console::puts("PAGE_FAULT_HANDLER: ERROR_CODE = 0x0.\n");
        // Obtain pointer to the current page directory
        unsigned long* directory_ptr = (unsigned long*) read_cr3();

        unsigned long faulted_page_addr = read_cr2();
        if(DEBUGGER_EN) {
            Console::puts("Faulted Address: ");
            Console::putui(faulted_page_addr);
            Console::puts("\n");
        }

        unsigned long pde_idx = (faulted_page_addr >> PDE_OFFSET); // Page Table Directory Index
        unsigned long pte_idx = (faulted_page_addr >> PTE_OFFSET) & (PTE_IDX_MASK); // Page Table Entry Index
		
		unsigned long* page_table_ptr = nullptr; 
		unsigned long* page_dir_ptr = nullptr;
		
		// Check if logical address is valid and legit
		bool is_legal = false;
		
        // Parse VM Pool
        VMPool* current = PageTable::vm_pool_hptr;
        
        while (current != nullptr) {
            if (current->is_legitimate(faulted_page_addr) == true) {
                is_legal = true;
                break;
            }
            current = current->vm_pool_next_ptr;
        }
		
		if((current != nullptr) && (is_legal == false)) {
		  Console::puts("PAGE_FAULT_HANDLER: Address is not legitimate.\n");
		  assert(0);	  	
		}
		
		// Check page fault
		if ((directory_ptr[pde_idx] & VALID_MASK_EN) == 0) { // Level-1 page fault : PDE not present
            if(DEBUGGER_EN) Console::puts("Page Fault due to no PDE.\n");
            int idx {0};
			page_table_ptr = (unsigned long *)(process_mem_pool->get_frames(1) * PAGE_SIZE);

			// 1023 | 1023 | Offset
			unsigned long* page_dir_ptr = (unsigned long *)(PDE_FLAG_CLEAR_MASK);

			page_dir_ptr[pde_idx] = (unsigned long)(page_table_ptr) | RW_MASK_EN | VALID_MASK_EN;

			for(idx = 0; idx < 1024; ++idx) {
				page_table_ptr[idx] = UK_MASK_EN;
			}

            // While at it, allocate PTE to avoid another exception.
			page_dir_ptr = (unsigned long *)(process_mem_pool->get_frames(1) * PAGE_SIZE);
			
            // Effectively: {1023 | PDE | Offset}
			unsigned long* pte = (unsigned long *)((0x3FF << 22) | (pde_idx << PTE_OFFSET));
			
			pte[pte_idx] = ( (unsigned long)(page_dir_ptr) | RW_MASK_EN | VALID_MASK_EN);
            
		}
        else { // Level 2 page fault: PTE not present
            if(DEBUGGER_EN) Console::puts("Page Fault due to no PTE.\n");
			page_dir_ptr = (unsigned long *)(process_mem_pool->get_frames(1) * PAGE_SIZE);
			
			unsigned long* pte = (unsigned long *)((0x3FF << 22) | (pde_idx << PTE_OFFSET) );
			
			pte[pte_idx] = ( (unsigned long)(page_dir_ptr) | 0b11 );
		}
	}

   Console::puts("PAGE_FAULT_HANDLER: Done.\n");

}

void 
PageTable::register_pool(VMPool * _vm_pool)
{
    // First time initialization of VM
	if(PageTable::vm_pool_hptr == nullptr) {
		PageTable::vm_pool_hptr = _vm_pool;
	}
	else { // Non-first virtual mempool
        VMPool* current = PageTable::vm_pool_hptr;
        // Parse Linked List
        while (current->vm_pool_next_ptr != nullptr) {
            current = current->vm_pool_next_ptr;
        }   
		current->vm_pool_next_ptr = _vm_pool;
	}
    Console::puts("registered VM pool\n");
}

void 
PageTable::free_page(unsigned long _page_no) {
	// Get the page directory index
	unsigned long directory_idx = (_page_no & 0xFFC00000) >> PDE_OFFSET;
	
    // Find page table idx
	unsigned long tbl_idx = (_page_no & 0x003FF000 ) >> PTE_OFFSET;

	unsigned long* page_table_ptr = (unsigned long *)((0x000003FF << PDE_OFFSET) | (directory_idx << PTE_OFFSET));

	unsigned long frame_number = ((page_table_ptr[tbl_idx] & PDE_FLAG_CLEAR_MASK) / PAGE_SIZE );
	
	// Release frame
	process_mem_pool->release_frames(frame_number);
	
	// Mark PTE invalid
	page_table_ptr[tbl_idx] = (unsigned long) 0 | RW_MASK_EN;
	
	// Flush the TLB - we don't want residual stuff
	load();
	Console::puts("freed page\n");
}
