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



void 
PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   PageTable::kernel_mem_pool = _kernel_mem_pool;
   PageTable::process_mem_pool = _process_mem_pool;
   PageTable::shared_size = _shared_size;

   if(DEBUGGER_EN) {Console::puts("Page Table Initialization: DONE.\n");}
}

PageTable::PageTable()
{
   paging_enabled = 0; // Ensure paging is disabled

   // Allocate Single Frame to First Level  PT - Page Directory
   page_directory = (unsigned long*) (kernel_mem_pool->get_frames(1) * PAGE_SIZE);

   // Allocate Single Frame to Second Level - Page Table
   unsigned long* page_table_ptr = (unsigned long*) (kernel_mem_pool->get_frames(1) * PAGE_SIZE);

   unsigned long shared_pages = shared_size / PAGE_SIZE;
   unsigned long idx {0};

   // Update the First Page Directory Entry and mark the remaining as invalid in directory
   page_directory[0] = (unsigned long) page_table_ptr | RW_MASK_EN | VALID_MASK_EN;
   
   for(idx = 1; idx < shared_pages; ++idx) {
      page_directory[idx] = (unsigned long) 0 | RW_MASK_EN;
   }

   // Setup direct mapped pages
   unsigned long page_address {0};

   for(idx = 0; idx < shared_pages; ++idx, page_address+=PAGE_SIZE) {
      page_table_ptr[idx] = page_address | RW_MASK_EN | VALID_MASK_EN;
   }

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
   if(DEBUGGER_EN) {Console::puts("PAGE_FAULT_HANDLER: Handling Page Fault.");}
   
   // Extract error code
   unsigned int err_code = _r->err_code;
   
   if(DEBUGGER_EN) {Console::puts("| error code: "); Console::puti(err_code); Console::puts("\n");}

   // Obtain pointer to the current page directory
   unsigned long* directory_ptr = (unsigned long*) read_cr3();
   
   unsigned long faulted_page_addr = read_cr2();
   unsigned long pde_idx = (faulted_page_addr >> PDE_OFFSET); // Page Table Directory Index
   unsigned long pte_idx = (faulted_page_addr >> PTE_OFFSET) & (PTE_IDX_MASK); // Page Table Entry Index

   unsigned long* page_table_ptr {nullptr};
   unsigned long idx{0};   

   if((err_code & 0x1) == 0)  { // Page not present
      
      if((directory_ptr[pde_idx] & VALID_MASK_EN) == 0) { // Level-1 page fault : PDE not present
         directory_ptr[pde_idx] = (unsigned long)((kernel_mem_pool->get_frames(1) * PAGE_SIZE) | RW_MASK_EN | VALID_MASK_EN);

         page_table_ptr = (unsigned long*)(directory_ptr[pde_idx] & PDE_FLAG_CLEAR_MASK); 
         
         for(idx = 0; idx < 1024; ++idx) {
            page_table_ptr[idx] =  (unsigned long)0 | UK_MASK_EN;
         }
      }
      else {   // Level 2 page fault: PTE not present
         page_table_ptr = (unsigned long*)(directory_ptr[pde_idx] & PDE_FLAG_CLEAR_MASK); 
         page_table_ptr[pte_idx] = (unsigned long)((process_mem_pool->get_frames(1) * PAGE_SIZE) | RW_MASK_EN | VALID_MASK_EN);
      }
   }
   else {
      // TODO: Placeholder for protection fault
      Console::puts("PAGE_FAULT_HANDLER: Unexpected Protection Fault\n");
      assert(0);
   }
   if(DEBUGGER_EN) {Console::puts("PAGE_FAULT_HANDLER: Done.\n");}

}

