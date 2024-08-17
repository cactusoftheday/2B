#define HASH_BUFFER_LENGTH 32


typedef struct {
    int id;
    unsigned char* input;
} task;

typedef struct node {
    task* task;
    struct node* next;
} node;
