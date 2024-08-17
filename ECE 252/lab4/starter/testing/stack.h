#include <stdio.h>
#include <stdlib.h>

typedef struct url_stack
{
    int size;           
    int pos;           
    char **items;
} ISTACK;

int sizeof_shm_stack(int size);
ISTACK *create_stack(int size);
void destroy_stack(ISTACK *p);
int is_empty(ISTACK *p);
int push(ISTACK *p, char* url);
int is_full(ISTACK *p);
int pop(ISTACK *p, char **url);


int sizeof_shm_stack(int size)
{
    return (sizeof(ISTACK) + sizeof(char*) * size);
}


ISTACK *create_stack(int size)
{
    int mem_size = 0;
    ISTACK *pstack = NULL;
    
    if ( size == 0 ) {
        return NULL;
    }
    
    mem_size = sizeof_shm_stack(size);
    pstack = malloc(mem_size);

    if ( pstack == NULL ) {
        perror("malloc");
    } else {
        char *p = (char *)pstack;
        pstack->items = (char **) (p + sizeof(ISTACK));
        pstack->size = size;
        pstack->pos  = -1;
    }

    return pstack;
}

void destroy_stack(ISTACK *p)
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

int is_full(ISTACK *p)
{
    if ( p == NULL ) {
        return 0;
    }
    return ( p->pos == (p->size -1) );
}

int is_empty(ISTACK *p)
{
    if ( p == NULL ) {
        return 0;
    }
    return ( p->pos == -1 );
}

int push(ISTACK *p, char* url)
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

int pop(ISTACK *p, char **url)
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