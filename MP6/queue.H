#ifndef QUEUE_H
#define QUEUE_H

//#include "thread.H"

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

/**
 * Queue implemented as a simple linked list. 
 */

template <typename T>
class Queue {
private:
    struct Node {
        T* data;                  
        Node* next;
        Node(T* _data) : data{_data}, next{nullptr} {}   
    };
    Node* head;

public:
   /**
    * Default Constructor
    */
   Queue() : head{nullptr} {}

   /**
    * Destructor - need this in case it goes out of scope, and we need to free memory.
    * Normally I would've used unique_pointer or smart_pointer, but libraries aren't available.
    * Memory leaks are the worst.
    * 
    */
   ~Queue() {
      while(head != nullptr) {
         Node* temp = head;
         head = head->next;
         delete temp;
      }
   }

   /**
    * Add data to queue at the end of the list
    */
   void enqueue(T* _data) {
      // Empty Queue
      if(head == nullptr) {
         head = new Node(_data);
      }
      else {
         Node* current = head;

         while(current->next != nullptr) {
            current = current->next;
         }
         
         current->next = new Node(_data);
      }
   }

   /**
    * Remove element from top of queue (head)
    */
   T* dequeue() {
      // Empty queue
      if(head == nullptr) return nullptr;

      Node* current = head; 
      
      T* top = current->data;

      head = head->next;

      delete current;

      return top;
   }

};
#endif