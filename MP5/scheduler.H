/* 
    Author: R. Bettati, Joshua Capehart
            Department of Computer Science
            Texas A&M University
			
	    A thread scheduler.

*/
#ifndef SCHEDULER_H
#define SCHEDULER_H

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "thread.H"
#include "interrupts.H"
#include "console.H"

/*--------------------------------------------------------------------------*/
/* !!! IMPLEMENTATION HINT !!! */
/*--------------------------------------------------------------------------*/
/*
    One way to proceed is to implement the FIFO scheduling policy inside
    class 'Scheduler'. 

    If you plan to implement a Round-Robin Scheduler, derive it from class
    'Scheduler', say as class 'RRScheduler'. The class 'RRScheduler' is 
    really just a FIFO scheduler with THREE MODIFICATIONS:
    1. It manages a timer, which fires at the end-of-quantum (EOQ). 
    (For details on how to set up a timer and how to handle timer interrupts 
    see the 1-second timer in 'kernel.C'.)  The timer is set up in the
    constructor.
    2. It uses an additional function, the EOQ handler. This function gets
    called whenever an EOQ timer event fires. The EOQ handler forces the 
    current thread to call the scheduler's 'yield' function.
    3. The 'yield' function must be modified to account for unused quantum
    time. If a thread voluntarily yields, the EOQ timer must be reset in order
    to not penalize the next thread.
 
    (Note that this qualifies as programming at about the level of a baboon.
     Much better woudl be to have the abstract class 'Scheduler' implement 
     the basic scheduling MECHANISMS and provide abstract funtions to define
     the queue management POLICIES in derived classes, 
     such as 'FIFOScheduler'.)
    
 */
/**
 * Thread Control Block - Contains the thread and the pointer to the next TCB.
 * Normally, I would make thread and next private and provide interfaces to update/ read them.
 * But for the sake of simplicity here, I haven't done that.
 */

class ThreadControlBlock {
public:
   Thread* thread;                  /* Thread control block*/
   ThreadControlBlock* next;        /* Pointer to the next TCB in the FIFO*/
   /**
    * Default Constructor
    */
   ThreadControlBlock() : thread{nullptr}, next{nullptr} {}   
   /**
    * Initialize with thread
    */
   ThreadControlBlock(Thread* _thread) : thread{_thread}, next{nullptr} {}   
};

/**
 * This is the ready queue implementation.
 * It is implemented as a simple linked list. 
 */

class ReadyQueue {
private:
   ThreadControlBlock* head;

public:
   /**
    * Default Constructor
    */
   ReadyQueue() : head{nullptr} {}

   /**
    * Destructor - need this in case it goes out of scope, and we need to free memory.
    * Normally I would've used unique_pointer or smart_pointer, but libraries aren't available.
    * Memory leaks are the worst.
    * 
    */
   ~ReadyQueue() {
      while(head != nullptr) {
         ThreadControlBlock* temp = head;
         head = head->next;
         delete temp;
      }
   }

   /**
    * Add thread to queue at the end of the list
    */
   void enqueue(Thread* _thread) {
      // Empty Queue
      if(head == nullptr) {
         head = new ThreadControlBlock(_thread);
      }
      else {
         ThreadControlBlock* current = head;

         while(current->next != nullptr) {
            current = current->next;
         }
         
         current->next = new ThreadControlBlock(_thread);
      }
   }

   /**
    * Remove element(thread) from top of queue (head)
    */
   Thread* dequeue() {
      // Empty queue
      if(head == nullptr) return nullptr;

      ThreadControlBlock* current = head; 
      
      Thread* top = current->thread;

      head = head->next;

      delete current;

      return top;
   }

};

/*--------------------------------------------------------------------------*/
/* SCHEDULER */
/*--------------------------------------------------------------------------*/

class Scheduler {

/* The scheduler may need private members... */
private:
   ReadyQueue rdy_q;
   unsigned int rdy_q_size;

/**
 * A lot of repeated use of interrupt enable logic - hence I decided to create an API to it.
 * Made it protected, so as to not make it publicly accessible. 
 */
protected:
   inline void disable_interrupts();
   inline void enable_interrupts();
  
public:

   Scheduler();
   /* Setup the scheduler. This sets up the ready queue, for example.
      If the scheduler implements some sort of round-robin scheme, then the 
      end_of_quantum handler is installed in the constructor as well. */

   /* NOTE: We are making all functions virtual. This may come in handy when
            you want to derive RRScheduler from this class. */
  
   virtual void yield();
   /* Called by the currently running thread in order to give up the CPU. 
      The scheduler selects the next thread from the ready queue to load onto 
      the CPU, and calls the dispatcher function defined in 'Thread.H' to
      do the context switch. */

   virtual void resume(Thread * _thread);
   /* Add the given thread to the ready queue of the scheduler. This is called
      for threads that were waiting for an event to happen, or that have 
      to give up the CPU in response to a preemption. */

   virtual void add(Thread * _thread);
   /* Make the given thread runnable by the scheduler. This function is called
      after thread creation. Depending on implementation, this function may 
      just add the thread to the ready queue, using 'resume'. */

   virtual void terminate(Thread * _thread);
   /* Remove the given thread from the scheduler in preparation for destruction
      of the thread. 
      Graciously handle the case where the thread wants to terminate itself.*/
  
};
	
class RRScheduler : public Scheduler, public InterruptHandler {
private:
   /**
    * Private members, similar to the Scheduler.
    * Ideally, the queue should have been part of an abstract class, or a protected member, but, oh well..
    */
   ReadyQueue rr_rdy_q;
   unsigned int rr_rdy_q_size;
   /**
    * Keeping track of ticks
    */
   unsigned int tick;
   unsigned int Hz;

   void set_frequency(unsigned int _Hz);

public:

   RRScheduler();

   /**
    * Custom interrupt handler for getting and receiving ticks.
    * Overrides the handle interrupt of class InterruptHandler.
    */
   virtual void handle_interrupt(REGS* _regs);

   /* Called by the currently running thread in order to give up the CPU. 
      The scheduler selects the next thread from the ready queue to load onto 
      the CPU, and calls the dispatcher function defined in 'Thread.H' to
      do the context switch. */
   virtual void yield() override;

   /* Add the given thread to the ready queue of the scheduler. This is called
      for threads that were waiting for an event to happen, or that have 
      to give up the CPU in response to a preemption. */
   virtual void resume(Thread * _thread) override;

   /* Make the given thread runnable by the scheduler. This function is called
      after thread creation. Depending on implementation, this function may 
      just add the thread to the ready queue, using 'resume'. */
   virtual void add(Thread * _thread) override;

   /* Remove the given thread from the scheduler in preparation for destruction
      of the thread. 
      Graciously handle the case where the thread wants to terminate itself.*/
   virtual void terminate(Thread * _thread) override;


};

#endif
