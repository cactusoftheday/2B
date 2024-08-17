#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>
#include <search.h>
#include "help.h"
#include <getopt.h>


typedef struct{
    struct char_stack *URLFrontier;
    struct char_stack *PNGURL;
    struct char_stack *VisitedURLs;
    int m;
    // int i;
}thread_data_t;

void *web_crawler(void* arg);

int main( int argc, char** argv ) 
{   
    /* Start Timer */
    double times[2];
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) /* Start Timer */
    {
        perror("gettimeofday");
        abort();
    }
    times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

    xmlInitParser();

    /* Parse Inputs */
    int t = 1;
    int m = 50;
    char* logfile = NULL;
    char* url = argv[argc-1];
    if (parseInputs( argc, argv, &t, &m, &logfile ) == -1){ exit(0);}
    printf("t = %d, m = %d, file = %s, seedURL = %s\n", t, m, logfile, url);

    /* Create the three stacks - MAYBE EDIT SIZE */
    struct char_stack *URLFrontier = create_stack(50000);
    struct char_stack *PNGURL = create_stack(m);
    struct char_stack *VisitedURLs = create_stack(50000);

    sem_t sem;
    sem_init(&sem, 0, 1);

    /* Intialize the hashmap */
    hcreate(500);

    /* Push the seed URL to the frontier */
    char* hold_url  = strdup(url);
    push(URLFrontier, hold_url);

    /* Intialize semaphores */

    // char* url_front = NULL;
    // pop(URLFrontier, &url_front);
    // ENTRY e, *ep;
    // e.key = url_front;
    // ep = hsearch(e, FIND);
    // if (ep == NULL) {
    //     // Not visited, mark as visited and process the URL
    //     push(VisitedURLs, e.key);
    //     ep = hsearch(e, ENTER);
        
    //     sendRequest(e.key, URLFrontier, PNGURL);
    // }
    /* Start threads */
    pthread_t threads[t];
    for (int i = 0; i < t; i++)
    {
        thread_data_t *data = malloc(sizeof(thread_data_t));
        data->m = m;
        data->PNGURL = PNGURL;
        data->URLFrontier = URLFrontier;
        data->VisitedURLs = VisitedURLs;
        // data->i = i;
        int thread_err = pthread_create(&threads[i], NULL, web_crawler, (void *)data);
        if (thread_err) {
            fprintf(stderr, "Error: pthread_create() returned %d\n", thread_err);
            return 1;
        }
    }

    for (int i = 0; i < t; i++)
    {
        pthread_join(threads[i], NULL);
        printf("Thread ID %p joined.\n", (void*)threads[i]); 
    }

    /* Write m PNG URLs to png_urls.txt*/
    FILE *file = fopen("png_urls.txt", "w");
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    while (!is_empty(PNGURL))
    {
        char* hold_url = NULL; 
        pop(PNGURL, &hold_url);
        fprintf(file, "%s\n", hold_url);
        free(hold_url);
    }
    fclose(file);

    /* If specified, write all visited URLs to logfile */
    if(logfile != NULL)
    {
        FILE *file = fopen(logfile, "w");
        if (file == NULL) {
            perror("Error opening file");
            return EXIT_FAILURE;
        }

        while (!is_empty(VisitedURLs))
        {
            char* hold_url = NULL;
            pop(VisitedURLs, &hold_url);
            fprintf(file, "%s\n", hold_url);
            free(hold_url);
        }
        fclose(file);
    }
    else
    {
        while (!is_empty(VisitedURLs))
        {
            char* hold_url = NULL;
            pop(VisitedURLs, &hold_url);
            free(hold_url);
        }
    }

    /* Total execution time */
    if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
    times[1] = (tv.tv_sec) + tv.tv_usec/1000000.;
    printf("findpng2 execution time: %.6lf seconds\n", times[1] - times[0]);


    /* FREE STUFF */
    xmlCleanupParser();

    destroy_stack(URLFrontier);
    destroy_stack(PNGURL);
    destroy_stack(VisitedURLs);

    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&help);
    pthread_cond_destroy(&cond);
}

void *web_crawler(void* arg) {
    thread_data_t *data = (thread_data_t *)arg;
    // printf("Thread %d entered.\n", data->i);
    
    while (1) {
        pthread_mutex_lock(&lock);

        while ((is_empty(data->URLFrontier)) && active_threads > 0) {
            pthread_cond_wait(&cond, &lock);
        }

        if ((is_empty(data->URLFrontier) && active_threads == 0) || (pngs >= (data->m - active_threads))) {
            pthread_mutex_unlock(&lock);
            break;
        }

        // if ((pngs >= data->m))
        // {
        //     printf ("thread %d reached max pngs\n", data->i);
        //     pthread_mutex_unlock(&lock);
        //     break;
        // }

        active_threads = active_threads + 1;

        // printf("Thread %d entered while loop.\n", data->i);
        char* url_front = NULL;
        pop(data->URLFrontier, &url_front);
        ENTRY e, *ep;
        e.key = url_front;
        ep = hsearch(e, FIND);
        if (ep == NULL) {
            push(data->VisitedURLs, e.key);
            ep = hsearch(e, ENTER);
            printf("Before unlock\n");
            pthread_mutex_unlock(&lock);
            printf("Before req\n");
            sendRequest(e.key, data->URLFrontier, data->PNGURL, data->m);
            printf("after req\n");
            // printf ("Total PNGs so far = %d, m = %d\n", pngs, data->m);
            pthread_mutex_lock(&lock);
            active_threads = active_threads - 1;
            pthread_cond_signal(&cond); // Signal other threads to check the stack
            if (pngs >= data->m) {
                pthread_mutex_unlock(&lock);
                break;
            }
            pthread_mutex_unlock(&lock);
            
        } else {
            free(url_front);
            active_threads = active_threads - 1;
            pthread_cond_signal(&cond); // Signal other threads to check the stack
            if (pngs >= data->m) {
                pthread_mutex_unlock(&lock);
                break;
            }
            pthread_mutex_unlock(&lock);
        }
    }

    free(data);
    pthread_exit(NULL);
}
