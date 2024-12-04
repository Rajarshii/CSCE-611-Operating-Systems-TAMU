/*
     File        : nonblocking_disk.H

     Author      : Rajarshi Das

     Date        : MM/DD/2024
     Description : Non Blocking Disk

*/

#ifndef _NONBLOCKING_DISK_H_
#define _NONBLOCKING_DISK_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MAX_DISK_DELAY 2

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "simple_disk.H"
#include "thread.H"
#include "queue.H"
#include "scheduler.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

static int DISK_DELAY = MAX_DISK_DELAY;

/*--------------------------------------------------------------------------*/
/* N o n B l o c k i n g D i s k  */
/*--------------------------------------------------------------------------*/

class NonBlockingDisk : public SimpleDisk {
private:
   Queue<Thread>* blocked_thread_q;    // Blocked Thread Queue
   unsigned int blocked_thread_qsize;  // Size of the blocked thread queue

   /**
    * Check if there are blocked threads in the queue.
    */
   bool check_blocked_threads();

   /**
    * Return the blocked thread at the top of the queue.
    */
   Thread* pop_thread();

protected:
   /**
    * Overriden is_ready( ) function.
    */
   virtual bool is_ready() override;

   /**
    * Overriden wait until ready function. 
    */
   void wait_until_ready() override;

public:
   NonBlockingDisk(DISK_ID _disk_id, unsigned int _size); 
   /* Creates a NonBlockingDisk device with the given size connected to the 
      MASTER or DEPENDENT slot of the primary ATA controller.
      NOTE: We are passing the _size argument out of laziness. 
      In a real system, we would infer this information from the 
      disk controller. */

   /* DISK OPERATIONS */

   virtual void read(unsigned long _block_no, unsigned char * _buf);
   /* Reads 512 Bytes from the given block of the disk and copies them 
      to the given buffer. No error check! */

   virtual void write(unsigned long _block_no, unsigned char * _buf);
   /* Writes 512 Bytes from the buffer to the given block on the disk. */

   /**
    * Declare the scheduler as a friend class. 
    */
   friend class Scheduler;
};

#endif
