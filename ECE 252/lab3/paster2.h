#define IMG_URL_PART_3 "&part="
#define DUM_URL "https://example.com/"
#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 10240  /* 1024*10 = 10K */
#define NUM_CHILD 5
#define SHM_SIZE 1024 
#define SEM_PROC 1
#define BUF_INC  524288   /* 1024*512  = 0.5M */
#define NUM_SEMS 4

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define IMG_URL ".uwaterloo.ca:2530/image?img="
char server[3][256] = {"http://ece252-1", "http://ece252-2", "http://ece252-3"};

pthread_mutex_t lock;

int shmid_producer;
int shmid_consumer;
int shmid_ring;



/////////////////// FORK ////////////////////////
int worker(int n);

/**
 * @brief sleeps [(n+1)*1000] milliseconds
 */
int worker(int n)
{
    usleep((n+1)*1000);
    printf("Worker ID=%d, pid = %d, ppid = %d.\n", n, getpid(), getppid());

    return 0;
}
/////////////////// FORK ////////////////////////


// from 1-50 run CURL for producers and put into buffer (protected by the semaphore)

/*////////////////// CURL ///////////////////////*/

/* This is a flattened structure, buf points to 
   the memory address immediately after 
   the last member field (i.e. seq) in the structure.
   Here is the memory layout. 
   Note that the memory is a chunk of continuous bytes.

   On a 64-bit machine, the memory layout is as follows:
   +================+
   | buf            | 8 bytes
   +----------------+
   | size           | 8 bytes
   +----------------+
   | max_size       | 8 bytes
   +----------------+
   | seq            | 4 bytes
   +----------------+
   | padding        | 4 bytes
   +----------------+
   | buf[0]         | 1 byte
   +----------------+
   | buf[1]         | 1 byte
   +----------------+
   | ...            | 1 byte
   +----------------+
   | buf[max_size-1]| 1 byte
   +================+
*/
typedef struct recv_buf_flat {
    char *buf;       /* memory to hold a copy of received data */
    size_t size;     /* size of valid data in buf in bytes*/
    size_t max_size; /* max capacity of buf in bytes*/
    int seq;         /* >=0 sequence number extracted from http header */
                     /* <0 indicates an invalid seq number */
} RECV_BUF;

size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);
int write_file(const char *path, const void *in, size_t len);


/**
 * @brief  cURL header call back function to extract image sequence number from 
 *         http header data. An example header for image part n (assume n = 2) is:
 *         X-Ece252-Fragment: 2
 * @param  char *p_recv: header data delivered by cURL
 * @param  size_t size size of each memb
 * @param  size_t nmemb number of memb
 * @param  void *userdata user defined data structurea
 * @return size of header data received.
 * @details this routine will be invoked multiple times by the libcurl until the full
 * header data are received.  we are only interested in the ECE252_HEADER line 
 * received so that we can extract the image sequence number from it. This
 * explains the if block in the code.
 */
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata)
{
    int realsize = size * nmemb;
    RECV_BUF *p = userdata;
    
    if (realsize > strlen(ECE252_HEADER) &&
	strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0) {

        /* extract img sequence number */
	p->seq = atoi(p_recv + strlen(ECE252_HEADER));

    }
    return realsize;
}

size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata)
{
    size_t realsize = size * nmemb;
    RECV_BUF *p = (RECV_BUF *)p_userdata;
 
    if (p->size + realsize + 1 > p->max_size) {/* hope this rarely happens */ 
        /* received data is not 0 terminated, add one byte for terminating 0 */
        size_t new_size = p->max_size + max(BUF_INC, realsize + 1);   
        char *q = realloc(p->buf, new_size);
        if (q == NULL) {
            perror("realloc"); /* out of memory */
            return -1;
        }
        p->buf = q;
        p->max_size = new_size;
    }

    memcpy(p->buf + p->size, p_recv, realsize); /*copy data from libcurl*/
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}


/**
 * @brief write callback function to save a copy of received data in RAM.
 *        The received libcurl data are pointed by p_recv, 
 *        which is provided by libcurl and is not user allocated memory.
 *        The user allocated memory is at p_userdata. One needs to
 *        cast it to the proper struct to make good use of it.
 *        This function maybe invoked more than once by one invokation of
 *        curl_easy_perform().
 */

size_t write_cb_curl(char *p_recv, size_t size, size_t nmemb, void *p_userdata)
{
    size_t realsize = size * nmemb;
    RECV_BUF *p = (RECV_BUF *)p_userdata;
 
    if (p->size + realsize + 1 > p->max_size) {/* hope this rarely happens */ 
        fprintf(stderr, "User buffer is too small, abort...\n");
        abort();
    }

    memcpy(p->buf + p->size, p_recv, realsize); /*copy data from libcurl*/
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}

/**
 * @brief calculate the actual size of RECV_BUF
 * @param size_t nbytes number of bytes that buf in RECV_BUF struct would hold
 * @return the REDV_BUF member fileds size plus the RECV_BUF buf data size
 */
int sizeof_shm_recv_buf(size_t nbytes)
{
    return (sizeof(RECV_BUF) + sizeof(char) * nbytes);
}

/**
 * @brief initialize the RECV_BUF structure. 
 * @param RECV_BUF *ptr memory allocated by user to hold RECV_BUF struct
 * @param size_t nbytes the RECV_BUF buf data size in bytes
 * NOTE: caller should call sizeof_shm_recv_buf first and then allocate memory.
 *       caller is also responsible for releasing the memory.
 */

int shm_recv_buf_init(RECV_BUF *ptr, size_t nbytes)
{
    if ( ptr == NULL ) {
        return 1;
    }
    
    ptr->buf = (char *)ptr + sizeof(RECV_BUF);
    ptr->size = 0;
    ptr->max_size = nbytes;
    ptr->seq = -1;              /* valid seq should be non-negative */
    
    return 0;
}

int recv_buf_init(RECV_BUF *ptr, size_t max_size)
{
    void *p = malloc(max_size);
    if (p == NULL) {
	    return 2;
    }
    
    ptr->buf = p;
    ptr->size = 0;
    ptr->max_size = max_size;
    ptr->seq = -1;              /* valid seq should be non-negative */
    return 0;
}


/**
 * @brief output data in memory to a file
 * @param path const char *, output file path
 * @param in  void *, input data to be written to the file
 * @param len size_t, length of the input data in bytes
 */

int write_file(const char *path, const void *in, size_t len)
{
    FILE *fp = NULL;

    if (path == NULL) {
        fprintf(stderr, "write_file: file name is null!\n");
        return -1;
    }

    if (in == NULL) {
        fprintf(stderr, "write_file: input data is null!\n");
        return -1;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror("fopen");
        return -2;
    }

    if (fwrite(in, 1, len, fp) != len) {
        fprintf(stderr, "write_file: imcomplete write!\n");
        return -3; 
    }
    return fclose(fp);
}

typedef struct int_stack
{
    int size;               /* the max capacity of the stack */
    int pos;                /* position of last item pushed onto the stack */
    int *items;             /* stack of stored integers */
} ISTACK;

/**
 * @brief calculate the total memory that the struct int_stack needs and
 *        the items[size] needs.
 * @param int size maximum number of integers the stack can hold
 * @return return the sum of ISTACK size and the size of the data that
 *         items points to.
 */

// int sizeof_shm_stack(int size)
// {
//     return (sizeof(ISTACK) + sizeof(int) * size);
// }

// /**
//  * @brief initialize the ISTACK member fields.
//  * @param ISTACK *p points to the starting addr. of an ISTACK struct
//  * @param int stack_size max. number of items the stack can hold
//  * @return 0 on success; non-zero on failure
//  * NOTE:
//  * The caller first calls sizeof_shm_stack() to allocate enough memory;
//  * then calls the init_shm_stack to initialize the struct
//  */
// int init_shm_stack(ISTACK *p, int stack_size)
// {
//     if ( p == NULL || stack_size == 0 ) {
//         return 1;
//     }

//     p->size = stack_size;
//     p->pos  = -1;
//     p->items = (int *) ((char *)p + sizeof(ISTACK));
//     return 0;
// }

// /**
//  * @brief create a stack to hold size number of integers and its associated
//  *      ISTACK data structure. Put everything in one continous chunk of memory.
//  * @param int size maximum number of integers the stack can hold
//  * @return NULL if size is 0 or malloc fails
//  */

// ISTACK *create_stack(int size)
// {
//     int mem_size = 0;
//     ISTACK *pstack = NULL;
    
//     if ( size == 0 ) {
//         return NULL;
//     }

//     mem_size = sizeof_shm_stack(size);
//     pstack = malloc(mem_size);

//     if ( pstack == NULL ) {
//         perror("malloc");
//     } else {
//         char *p = (char *)pstack;
//         pstack->items = (int *) (p + sizeof(ISTACK));
//         pstack->size = size;
//         pstack->pos  = -1;
//     }

//     return pstack;
// }

// /**
//  * @brief release the memory
//  * @param ISTACK *p the address of the ISTACK data structure
//  */

// void destroy_stack(ISTACK *p)
// {
//     if ( p != NULL ) {
//         free(p);
//     }
// }

// /**
//  * @brief check if the stack is full
//  * @param ISTACK *p the address of the ISTACK data structure
//  * @return non-zero if the stack is full; zero otherwise
//  */

// int is_full(ISTACK *p)
// {
//     if ( p == NULL ) {
//         return 0;
//     }
//     return ( p->pos == (p->size -1) );
// }

// /**
//  * @brief check if the stack is empty 
//  * @param ISTACK *p the address of the ISTACK data structure
//  * @return non-zero if the stack is empty; zero otherwise
//  */

// int is_empty(ISTACK *p)
// {
//     if ( p == NULL ) {
//         return 0;
//     }
//     return ( p->pos == -1 );
// }

// /**
//  * @brief push one integer onto the stack 
//  * @param ISTACK *p the address of the ISTACK data structure
//  * @param int item the integer to be pushed onto the stack 
//  * @return 0 on success; non-zero otherwise
//  */

// int push(ISTACK *p, int item)
// {
//     if ( p == NULL ) {
//         return -1;
//     }

//     if ( !is_full(p) ) {
//         ++(p->pos);
//         p->items[p->pos] = item;
//         return 0;
//     } else {
//         return -1;
//     }
// }

// /**
//  * @brief push one integer onto the stack 
//  * @param ISTACK *p the address of the ISTACK data structure
//  * @param int *item output parameter to save the integer value 
//  *        that pops off the stack 
//  * @return 0 on success; non-zero otherwise
//  */

// int pop(ISTACK *p, int *p_item)
// {
//     if ( p == NULL ) {
//         return -1;
//     }

//     if ( !is_empty(p) ) {
//         *p_item = p->items[p->pos];
//         (p->pos)--;
//         return 0;
//     } else {
//         return 1;
//     }
// }

// void push_all(struct int_stack *p, int start)
// {
//     int i;
    
//     if ( p == NULL) {
//         abort();
//     }
    
//     for( i = 0; ; i++ )  {
//         int ret;
//         int item = start - i;

//         ret = push(p, item);
//         if ( ret != 0 ) {
//             break;
//         }
//         printf("item[%d] = 0x%4X pushed onto the stack\n", i, item);
 
//     }
//     printf("%d items pushed onto the stack.\n", i);
    
// }

// /* pop STACK_SIZE items off the stack */
// void pop_all(struct int_stack *p)
// {
//     int i;
//     if ( p == NULL) {
//         abort();
//     }

//     for ( i = 0; ; i++ ) {
//         int item;
//         int ret = pop(p, &item);
//         if ( ret != 0 ) {
//             break;
//         }
//         printf("item[%d] = 0x%4X popped\n", i, item);
//     }

//     printf("%d items popped off the stack.\n", i);

// }

typedef struct recv_buf_stack {
    int size;               // the max capacity of the stack
    int pos;                // position of last item pushed onto the stack
    int *items;        // stack of stored RECV_BUF elements
} RSTACK;

// Function to calculate the total memory needed for the RSTACK
int sizeof_shm_stack(int size) {
    return (sizeof(RSTACK) + sizeof(RECV_BUF) * size);
}

// Function to initialize the RSTACK member fields
int init_shm_stack(RSTACK *p, int stack_size) {
    if (p == NULL || stack_size == 0) {
        return 1;
    }

    p->size = stack_size;
    p->pos  = -1;
    p->items = (int *) ((char *)p + sizeof(RSTACK));
    return 0;
}

// Function to create a stack to hold size number of RECV_BUF elements
RSTACK *create_stack(int size) {
    int mem_size = 0;
    RSTACK *pstack = NULL;

    if (size == 0) {
        return NULL;
    }

    mem_size = sizeof_shm_stack(size);
    pstack = malloc(mem_size);

    if (pstack == NULL) {
        perror("malloc");
    } else {
        char *p = (char *)pstack;
        pstack->items = (int *) (p + sizeof(RSTACK));
        pstack->size = size;
        pstack->pos  = -1;
    }

    return pstack;
}

// Function to release the memory
void destroy_stack(RSTACK *p) {
    if ( p != NULL ) {
        free(p);
    }
}

// Function to check if the stack is full
int is_full(RSTACK *p) {
    if (p == NULL) {
        return 0;
    }
    return (p->pos == (p->size - 1));
}

// Function to check if the stack is empty
int is_empty(RSTACK *p) {
    if (p == NULL) {
        return 0;
    }
    return (p->pos == -1);
}

// Function to push one RECV_BUF element onto the stack
int push(RSTACK *p, int item) {
    if (p == NULL) {
        return -1;
    }

    if (!is_full(p)) {
        ++(p->pos);
        p->items[p->pos] = item;
        return 0;
    } else {
        return -1;
    }
}

// Function to pop one RECV_BUF element off the stack
int pop(RSTACK *p, int *p_item) {
    if (p == NULL) {
        return -1;
    }

    if (!is_empty(p)) {
        *p_item = p->items[p->pos];
        (p->pos)--;
        return 0;
    } else {
        return 1;
    }
}