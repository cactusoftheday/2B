#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/multi.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>
#include <search.h>
#include "helper.h"
#include "helper.c"
#include <getopt.h>
#include <math.h>

#define MAX_WAIT_MSECS 30*1000 /* Wait max. 30 seconds */
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

static size_t cb(char *d, size_t n, size_t l, void *p)
{
  /* take care of the data here, ignored in this example */
  (void)d;
  (void)p;
  return n*l;
}

int main(int argc, char** argv)
{
    CURL *eh=NULL;
    CURLMsg *msg=NULL;
    CURLcode return_code=0;
    int still_running=0, i=0, msgs_left=0;
    int http_status_code;
    const char *szUrl;

    /* Intialize the hashmap_lock */
    hcreate(500);
    xmlInitParser();

    /* Parse Inputs */
    int t = 1;
    char* logfile = NULL;
    char* url = argv[argc-1];
    if (parseInputs(argc, argv, &t, &m, &logfile) == -1) {
        exit(0);
    }

    /* Create the three stacks - MAYBE EDIT SIZE */
    struct char_stack *URLFrontier = create_stack(1500);
    struct char_stack *PNGURL = create_stack(m);
    struct char_stack *VisitedURLs = create_stack(500);
    char* url_front = strdup(url);

    /* Start Timer */
    double times[2];
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) /* Start Timer */
    {
        perror("gettimeofday");
        abort();
    }
    times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

    /* Push the seed URL to the frontier */
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    push(URLFrontier, url_front);
    printf("URL: %s\n", url_front);
    run(URLFrontier, VisitedURLs, PNGURL, t, m);

    /* Write m PNG URLs to png_urls.txt */
    FILE *file = fopen("png_urls.txt", "w");
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    while (!is_empty(PNGURL)) {
        char* hold_url = NULL;
        pop(PNGURL, &hold_url);
        fprintf(file, "%s\n", hold_url);
        free(hold_url);
    }
    fclose(file);

    /* If specified, write all visited URLs to logfile */
    if (logfile != NULL) {
        FILE *file = fopen(logfile, "w");
        if (file == NULL) {
            perror("Error opening file");
            return EXIT_FAILURE;
        }

        while (!is_empty(VisitedURLs)) {
            char* hold_url = NULL;
            pop(VisitedURLs, &hold_url);
            fprintf(file, "%s\n", hold_url);
            free(hold_url);
        }
        fclose(file);
    } else {
        while (!is_empty(VisitedURLs)) {
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
    times[1] = (tv.tv_sec) + tv.tv_usec / 1000000.;
    printf("findpng2 execution time: %.6lf seconds\n", times[1] - times[0]);

    destroy_stack(URLFrontier);
    destroy_stack(PNGURL);
    destroy_stack(VisitedURLs);
    curl_global_cleanup();
    xmlCleanupParser();

    return EXIT_SUCCESS;
}
