#define _XOPEN_SOURCE 500 

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <openssl/sha.h>
#include "dontmodify.h"

#define NUM_ELVES 5

/* Global Variables -- Add more if you need! */
int total_tasks;
int active_tasks;
int remainingTasks;
pthread_mutex_t mutex;
sem_t empty_list;
sem_t full_list;
sem_t goHome;


/* Function Prototypes for pthreads */
void* dobby( void* );
void* house_elf( void * );

/* Don't change the function prototypes below;
   they are your API for how the house elves do work */

/* Removes a task from the list of things to do
   To be run by: house elf threads
   Takes no arguments
   NOT thread safe!
   return: a pointer to a task for an elf to do
*/
task* take_task();

/* Perform the provided task
   To be run by: house elf threads
   argument: pointer to task to be done
   IS thread safe
   return: nothing
*/  
void do_work( task* todo );

/* Put tasks in the list for elves to do
   To be run by: Dobby
   argument: how many tasks to put in the list
   NOT thread safe
   return: nothing
*/
void post_tasks( int howmany );

/* Used to unlock a mutex, if necessary, if a thread
   is cancelled when blocked on a semaphore
*/
void house_elf_cleanup( void * );



/* Complete the implementation of main() */

int main( int argc, char** argv ) {
   if ( argc != 2 ) {
         printf( "Usage: %s total_tasks\n", argv[0] );
         return -1;
   }
   /* Init global variables here */
   total_tasks = atoi( argv[1] );
   remainingTasks = total_tasks;
   active_tasks = 0;
   pthread_t threads[6];
  
   printf("There are %d tasks to do today.\n", total_tasks);
  
   /* Launch threads here */
   pthread_mutex_init(&mutex, NULL);
   sem_init(&empty_list, 0, 1);
   sem_init(&full_list, 0, 0);
   sem_init(&goHome, 0, total_tasks * -1 + 1); //go home when all tasks are done

   int rc;
   for(int i = 1; i <= NUM_ELVES; i++) {
      rc = pthread_create(&threads[i], NULL, house_elf, (void*) i);
      if(rc != 0) {
         return -1;
      }
   }
   pthread_create(&threads[0], NULL, dobby, threads);

   pthread_join(threads[0], NULL);

   sem_destroy(&empty_list);
   sem_destroy(&full_list);
   sem_destroy(&goHome);
   pthread_mutex_destroy(&mutex);
   
   return 0;
}

/* Write the implementation of dobby() */

void* dobby( void * arg ) {
   pthread_t *threads = (pthread_t*) arg;
   while(remainingTasks > 0) {
      sem_wait(&empty_list); //wait for empty
      pthread_mutex_lock(&mutex);
      active_tasks = (remainingTasks < 10) ? remainingTasks : 10;
      post_tasks(active_tasks);
      for (int i = 0; i < active_tasks; i++) {
         sem_post(&full_list); //make list "full" again for elves
      }
      remainingTasks -= active_tasks;
      pthread_mutex_unlock(&mutex);
   }
   sem_wait(&goHome);
   for (int i = 1; i <= 5; i++) {
      pthread_cancel(threads[i]);
   }
   for (int i = 1; i <= NUM_ELVES; i++) {
      pthread_join(threads[i], NULL);
   }
}  

/* Complete the implementation of house_elf() */

void* house_elf( void * ignore ) {
   /* The following function call registers the cleanup
      handler to make sure that pthread_mutex_t locks 
      are unlocked if the thread is cancelled. It must
      be bracketed with a call to pthread_cleanup_pop 
      so don't change this line or the other one */
   
   pthread_cleanup_push( house_elf_cleanup, NULL ); 
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   task* task;
   while( 1 ) {
      sem_wait(&full_list);
      pthread_mutex_lock(&mutex);
      task = take_task(); 
      active_tasks -= 1;
      if(active_tasks == 0) {
         sem_post(&empty_list);
      }
      pthread_mutex_unlock(&mutex);
      do_work(task);
      sem_post(&goHome);
   }
   free(task);
   /* This cleans up the registration of the cleanup handler */
   pthread_cleanup_pop( 0 ) ;
   return NULL;
}

/* Implement unlocking of any pthread_mutex_t mutex types
   that are locked in the house_elf thread, if any 
*/
void house_elf_cleanup( void* arg ) {
   pthread_mutex_unlock(&mutex); // i don't know why but the comment says to do this?
}
