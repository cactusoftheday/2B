#define _XOPEN_SOURCE 500 

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <openssl/sha.h>
#include "dontmodify.h"

unsigned int random_seed = 252;
int task_id_counter = 0;
node* list_head = NULL;

/* Generate random string code based off original by Ates Goral */
char* random_string(const int len, unsigned int * generator_seed) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    char* s = malloc( (len+1) * sizeof( char ));

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand_r(generator_seed) % (sizeof(alphanum) - 1)];
    }
    s[len] = 0;
    return s;
}

void post_tasks( int howmany ) {
    printf("Adding %d tasks to the list of active tasks.\n", howmany);
    for ( int i = 0; i < howmany; ++i ) {
        task* t = malloc( sizeof( task ));
        t->id = ++task_id_counter;
        t->input = random_string( HASH_BUFFER_LENGTH, &random_seed );
        node* n = malloc( sizeof( node ));
        n->task = t;
        n->next = list_head;
        list_head = n;
    }
}

void do_work( task* todo ) {
     unsigned char* output_buffer = calloc( HASH_BUFFER_LENGTH , sizeof ( unsigned char ) );
     SHA256( todo->input, HASH_BUFFER_LENGTH, output_buffer );
     printf("Task %d was completed!\n", todo->id);
     free( output_buffer );
     free( todo->input );
     free( todo );
}

task* take_task( ) {
    if (list_head == NULL) {
        /* Something has gone wrong */
        printf("Trying to take from an empty list!\n");
        exit( -1 );
    }
    node* head = list_head;
    task* t = head->task;
    list_head = list_head->next;
    free( head );
    return t;
}
