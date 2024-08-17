#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/curl.h>

/*/////////////////////////////////////////////////////*/
/* #DEFINE */
/*/////////////////////////////////////////////////////*/
#define IMG_URL ".uwaterloo.ca:2520/image?img="
#define DUM_URL "https://example.com/"
#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
/*/////////////////////////////////////////////////////*/



/*/////////////////////////////////////////////////////*/
/* STRUCTS */
/*/////////////////////////////////////////////////////*/
typedef struct recv_buf2 {
    char *buf;       /* memory to hold a copy of received data */
    size_t size;     /* size of valid data in buf in bytes*/
    size_t max_size; /* max capacity of buf in bytes*/
    int seq;         /* >=0 sequence number extracted from http header */
                     /* <0 indicates an invalid seq number */
} RECV_BUF;

typedef struct {
    CURL *curl_handle;
    int thread_id;
    RECV_BUF recv_buf;
    char url[256];
} thread_data_t;

struct thread_args              /* thread input parameters struct */
{
    int x;
    int y;
};

struct thread_ret               /* thread return values struct   */
{
    int sum;
    int product;
};
/*/////////////////////////////////////////////////////*/



/*/////////////////////////////////////////////////////*/
/* Function Declarations */
/*/////////////////////////////////////////////////////*/
/*cURL*/
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);
int write_file(const char *path, const void *in, size_t len);
/**/
void *do_work(void *arg);  /* a routine that can run as a thread by pthreads */
/*/////////////////////////////////////////////////////*/