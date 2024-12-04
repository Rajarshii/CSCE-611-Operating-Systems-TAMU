/*
     File        : nonblocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "nonblocking_disk.H"

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

extern Scheduler* SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

NonBlockingDisk::NonBlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
    blocked_thread_q = new Queue<Thread>();
    blocked_thread_qsize = 0;
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void NonBlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  SimpleDisk::read(_block_no, _buf);
}


void NonBlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  SimpleDisk::write(_block_no, _buf);
}

bool NonBlockingDisk::is_ready() {
	//Console::puts("DELAY: "); Console::puti(DISK_DELAY); Console::puts("\n");  
  // If it is 0, reset the delay, and mark it as ready.
	if(DISK_DELAY != 0) {
    DISK_DELAY--;
		return false;
	}

	DISK_DELAY = MAX_DISK_DELAY;

	Console::puts("."); // output current status. 
  unsigned char status = Machine::inportb(0x1F7);
	Console::putui((unsigned int)status);
	return ((status & 0b00001000) != 0);
}

void NonBlockingDisk::wait_until_ready() {
  if(!NonBlockingDisk::is_ready()) {
    this->blocked_thread_q->enqueue(Thread::CurrentThread());

    this->blocked_thread_qsize++;

    SYSTEM_SCHEDULER->yield();
  }
}


bool NonBlockingDisk::check_blocked_threads() {
  return (NonBlockingDisk::is_ready() && (this->blocked_thread_qsize > 0));
}


Thread* NonBlockingDisk::pop_thread() {
  Thread* thread_ = this->blocked_thread_q->dequeue();

  this->blocked_thread_qsize--;

  return thread_;
}


