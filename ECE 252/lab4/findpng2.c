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
#include "helper.h"
#include <getopt.h>


typedef struct{
    struct char_stack *URLFrontier;
    struct char_stack *PNGURL;
    struct char_stack *VisitedURLs;
    int m;
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
    char* logfile = NULL;
    char* url = argv[argc-1];
    if (parseInputs( argc, argv, &t, &m, &logfile ) == -1){ exit(0);}

    /* Create the three stacks - MAYBE EDIT SIZE */
    struct char_stack *URLFrontier = create_stack(1500);
    struct char_stack *PNGURL = create_stack(m);
    struct char_stack *VisitedURLs = create_stack(500);

    /* Intialize the hashmap_lock */
    hcreate(500);

    /* Push the seed URL to the frontier */
    char* hold_url  = strdup(url);
    push(URLFrontier, hold_url);

    /* Start threads */
    pthread_t threads[t];
    for (int i = 0; i < t; i++)
    {
        thread_data_t *data = malloc(sizeof(thread_data_t));
        data->m = m;
        data->PNGURL = PNGURL;
        data->URLFrontier = URLFrontier;
        data->VisitedURLs = VisitedURLs;
        int thread_err = pthread_create(&threads[i], NULL, web_crawler, (void *)data);
        if (thread_err) {
            fprintf(stderr, "Error: pthread_create() returned %d\n", thread_err);
            return 1;
        }
    }

    for (int i = 0; i < t; i++)
    {
        pthread_join(threads[i], NULL); 
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
    pthread_mutex_destroy(&png_lock);
    pthread_mutex_destroy(&frontier_lock);
    pthread_mutex_destroy(&visited_lock);
    pthread_mutex_destroy(&hashmap_lock);
    pthread_cond_destroy(&cond);
}

void *web_crawler(void* arg) {
    thread_data_t *data = (thread_data_t *)arg;
    
    while (1) {
        pthread_mutex_lock(&frontier_lock);

        while ((is_empty(data->URLFrontier)) && active_threads > 0) {
            pthread_cond_wait(&cond, &frontier_lock);
        }

        if ((is_empty(data->URLFrontier) && active_threads == 0) || (__sync_fetch_and_add(&pngs, 0) >= (data->m/* - active_threads*/))) {
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&frontier_lock);
            break;
        }

        active_threads = active_threads + 1;

        char* url_front = NULL;

        pop(data->URLFrontier, &url_front);
        pthread_mutex_unlock(&frontier_lock);
        
        pthread_mutex_lock(&hashmap_lock);
        ENTRY e, *ep;
        e.key = url_front;
        ep = hsearch(e, FIND);
        if (ep == NULL) {
            ep = hsearch(e, ENTER);
            pthread_mutex_unlock(&hashmap_lock);
            
            pthread_mutex_lock(&visited_lock);
            push(data->VisitedURLs, e.key);
            pthread_mutex_unlock(&visited_lock);

            sendRequest(e.key, data->URLFrontier, data->PNGURL, data->m);

            pthread_mutex_lock(&frontier_lock);
            active_threads = active_threads - 1;
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&frontier_lock);
            
        } else {
            pthread_mutex_unlock(&hashmap_lock);
            free(url_front);

            pthread_mutex_lock(&frontier_lock);
            active_threads = active_threads - 1;
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&frontier_lock);
        }
    }

    free(data);
    pthread_exit(NULL);
}
