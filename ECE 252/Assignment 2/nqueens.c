// from the Cilk manual: http://supertech.csail.mit.edu/cilk/manual-5.4.6.pdf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

int n;

int safe(char * config, int i, int j)
{
    int r, s;

    for (r = 0; r < i; r++)
    {
        s = config[r];
        if (j == s || i-r==j-s || i-r==s-j)
            return 0;
    }
    return 1;
}

//int count = 0;
int * count = NULL;

void *worker(void* arg) {
    int thread_id = (int) arg;
    count[thread_id] = 0;
    char *config = malloc(n * sizeof(char));

    int start = thread_id;
    int end = (thread_id == n - 1) ? n : start + 1;

    for(int i = start; i < end; i++) {
        config[0] = i;
        nqueens(config, n, 1, thread_id);
    }
    
    free(config);
    pthread_exit(NULL);
}

void nqueens(char *config, int n, int i, int tid)
{
    char *new_config;
    int j;

    if (i==n)
    {
        count[tid]++;
    }
    
    /* try each possible position for queen <i> */
    for (j=0; j<n; j++)
    {
        /* allocate a temporary array and copy the config into it */
        new_config = malloc((i+1)*sizeof(char));
        memcpy(new_config, config, i*sizeof(char));
        if (safe(new_config, i, j))
        {
            new_config[i] = j;
	        nqueens(new_config, n, i+1, tid);
        }
        free(new_config);
    }
    return;
}

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        printf("%s: number of queens required\n", argv[0]);
        return 1;
    }

    char *config;
    pthread_t *threads;
    int rc, i, sum = 0;
    n = atoi(argv[1]);
    config = malloc(n * sizeof(char));

    threads = (pthread_t *) malloc(n * sizeof(pthread_t));
    count = (int *) malloc(n * sizeof(int));

    for(int i = 0; i < n; i++) {
        rc = pthread_create(&threads[i], NULL, worker, (void*) i);
        if(rc != 0) {
            return -1;
        }
    }

    for(int i = 0; i < n; i++) {
        rc = pthread_join(threads[i], NULL);
        if(rc != 0) {
            return -1;
        }
        sum += count[i];
    }

    printf("running queens %d\n", n);
    //nqueens(config, n, 0);
    printf("# solutions: %d\n", sum);

    free(threads);
    free(count);

    return 0;
}
