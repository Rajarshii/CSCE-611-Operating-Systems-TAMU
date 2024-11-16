/*
 File: scheduler.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "machine.H"

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

extern NonBlockingDisk* SYSTEM_DISK;

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
  rdy_qsize = 0;
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::disable_interrupts() {
  if(Machine::interrupts_enabled() == true) {
    Machine::disable_interrupts();
  }
}

void Scheduler::enable_interrupts() {
  if(Machine::interrupts_enabled() == false) {
    Machine::enable_interrupts();       /* Do I need to check anything else?*/
  }
}

void Scheduler::yield() {
  /**
   * Are interrupts enabled? If so disable them to handle ready queue.
   */
  Scheduler::disable_interrupts();
  
  if(SYSTEM_DISK->check_blocked_threads()) {
    Scheduler::enable_interrupts();

    Thread::dispatch_to(SYSTEM_DISK->pop_thread());
  }
  else {
    // non-empty rdy q. Get the thread from head, and start it.
    if(rdy_qsize != 0) {
      Thread* n_thread = rdy_q.dequeue();

      rdy_qsize--;

      Scheduler::enable_interrupts();     /* TODO re-check?*/

      Thread::dispatch_to(n_thread);
    }
    else {
      // Nothing to do here really
    }
  }
}

void Scheduler::resume(Thread * _thread) {

  Scheduler::disable_interrupts();

  rdy_q.enqueue(_thread);           // Add the requested thread at the end of the queue.

  rdy_qsize++;

  Scheduler::enable_interrupts();

}

void Scheduler::add(Thread * _thread) {

  Scheduler::disable_interrupts();

  rdy_q.enqueue(_thread);
  rdy_qsize++;

  Scheduler::enable_interrupts();

}

void Scheduler::terminate(Thread * _thread) {
  Scheduler::disable_interrupts();

  unsigned int tid {0};

  for(tid = 0; tid < rdy_qsize; ++tid) {
    Thread* head = rdy_q.dequeue();

    if(head->ThreadId() != _thread->ThreadId()) {
      rdy_q.enqueue(head);
    }
    else {
      rdy_qsize--;
    }
  }
  Scheduler::enable_interrupts();
}
