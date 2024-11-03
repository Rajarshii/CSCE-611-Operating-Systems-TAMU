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
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
  rdy_q_size = 0;
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
  
  // non-empty rdy q. Get the thread from head, and start it.
  if(rdy_q_size != 0) {
    Thread* n_thread = rdy_q.dequeue();

    rdy_q_size--;

    Scheduler::enable_interrupts();     /* TODO re-check?*/

    Thread::dispatch_to(n_thread);
  }
  else {
    // Nothing to do here really
  }
}

void Scheduler::resume(Thread * _thread) {

  Scheduler::disable_interrupts();

  rdy_q.enqueue(_thread);           // Add the requested thread at the end of the queue.

  rdy_q_size++;

  Scheduler::enable_interrupts();

}

void Scheduler::add(Thread * _thread) {

  Scheduler::disable_interrupts();

  rdy_q.enqueue(_thread);
  rdy_q_size++;

  Scheduler::enable_interrupts();

}

void Scheduler::terminate(Thread * _thread) {
  Scheduler::disable_interrupts();

  unsigned int tid {0};

  for(tid = 0; tid < rdy_q_size; ++tid) {
    Thread* head = rdy_q.dequeue();

    if(head->ThreadId() != _thread->ThreadId()) {
      rdy_q.enqueue(head);
    }
    else {
      rdy_q_size--;
    }
  }
  Scheduler::enable_interrupts();
}


/**
 * Round Robin Scheduler Methods
 */

RRScheduler::RRScheduler() {
  rr_rdy_q_size = 0;
  tick = 0;           // Initial tick = 0
  Hz = 5;             // Keep it fixed. For ticks = 10ms long, we get 5*10 ms = 50ms duration

  InterruptHandler::register_handler(0,this);

  set_frequency(Hz);
}

void RRScheduler::set_frequency(unsigned int _Hz) {
  Hz = _Hz;
  assert(Hz); // Safety check. Why is it not there in simple timer?

  // The remaining things are same as simple timer.
  unsigned int div = 1193180 / Hz;

  Machine::outportb(0x43,0x34);

  Machine::outportb(0x40, (div & 0xFF));

  Machine::outportb(0x40, (div >> 8));
}

void RRScheduler::yield() {
	// Send an End Of Interrupt msg to PIC for the timer interrupt
	Machine::outportb(0x20, 0x20);
	
  RRScheduler::disable_interrupts();
	
  // Non-empty rdy q. Get the thread from the head and start it
	if(rr_rdy_q_size != 0) {
    Thread* new_thread = rr_rdy_q.dequeue();
		
		tick = 0;
		
    rr_rdy_q_size--;

    RRScheduler::enable_interrupts();

		Thread::dispatch_to(new_thread);
	}
  else {
    // Nothing to do here really. Should get optimized away.
  }
}

void RRScheduler::resume(Thread* _thread) {
  RRScheduler::enable_interrupts();

  rr_rdy_q.enqueue(_thread);

  rr_rdy_q_size++;

  RRScheduler::enable_interrupts();
}

void RRScheduler::add(Thread* _thread) {
  RRScheduler::disable_interrupts();

  rr_rdy_q.enqueue(_thread);

  rr_rdy_q_size++;

  RRScheduler::enable_interrupts();
}

void RRScheduler::terminate(Thread* _thread) {
  RRScheduler::disable_interrupts();

  unsigned int tid {0};

  for(tid=0; tid < rr_rdy_q_size; ++tid) {
    Thread* head = rr_rdy_q.dequeue();

    if(head->ThreadId() != _thread->ThreadId()) {
      rr_rdy_q.enqueue(head);
    }
    else {
      rr_rdy_q_size--;
    }
  }
  RRScheduler::enable_interrupts();
}

void RRScheduler::handle_interrupt(REGS* _regs) {
  tick++;
  if(tick >= Hz) {
    tick = 0;
    Console::puts("50 ns has passed\n");
    resume(Thread::CurrentThread());
    yield();
  }
}