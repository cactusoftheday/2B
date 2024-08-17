#include <stdio.h>
#include <stdlib.h>

typedef struct char_stack
{
    char **items;
    int pos;
    int size;                      
} CSTACK;

int sizeof_stack(int size);
CSTACK *create_stack(int size);
void destroy_stack(CSTACK *p);
int is_empty(CSTACK *p);
int push(CSTACK *p, char* url);
int is_full(CSTACK *p);
int pop(CSTACK *p, char **url);


int sizeof_stack(int size)
{
    return (sizeof(CSTACK) + sizeof(char*) * size);
}


CSTACK *create_stack(int size)
{
    int mem_size = 0;
    CSTACK *pstack = NULL;
    
    if ( size == 0 ) {
        return NULL;
    }
    
    mem_size = sizeof_stack(size);
    pstack = malloc(mem_size);

    if ( pstack == NULL ) {
        perror("malloc");
    } else {
        char *p = (char *)pstack;
        pstack->items = (char **) (p + sizeof(CSTACK));
        pstack->size = size;
        pstack->pos  = -1;
    }

    return pstack;
}

void destroy_stack(CSTACK *p)
{
    while(!is_empty(p))
    {
        char* hold_url = NULL;
        pop(p, &hold_url);
        free(hold_url);
    }
    
    if ( p != NULL ) {
        free(p);
    }
}

int is_full(CSTACK *p)
{
    if ( p == NULL ) {
        return 0;
    }
    return ( p->pos == (p->size -1) );
}

int is_empty(CSTACK *p)
{
    // pthread_mutex_lock(&frontier_lock);
    int hold;
    if ( p == NULL ) {
        printf("p is null\n");
        return 0;
    }
    if (p->pos == -1)
    {
        hold = 1;
    }
    else
    {
        hold = 0;
    }
    // pthread_mutex_unlock(&frontier_lock);
    return hold;   
}

int push(CSTACK *p, char* url)
{
    if ( p == NULL ) {
        return -1;
    }

    if ( !is_full(p) ) {
        ++(p->pos);
        p->items[p->pos] = url;
        return 0;
    } else {
        return -1;
    }
}

int pop(CSTACK *p, char **url)
{
    if ( p == NULL ) {
        return -1;
    }

    if ( !is_empty(p) ) {
        *url = (p->items[p->pos]);
        (p->pos)--;
        return 0;
    } else {
        return 1;
    }
}

int peek(CSTACK *p, int index, char **url){
    if ( p == NULL ) {
        return -1;
    }

    if ( !is_empty(p) ) {
        *url = (p->items[index]);
        return 0;
    } else {
        return 1;
    }
}