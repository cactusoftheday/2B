/*****************************
* @authors: Joshwyn P, Isaac H
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/curl.h>
#include <pthread.h>
#include <getopt.h>
#include "paster.h"

/*/////////////////////////////////////////////////////*/
/* Global Variables */
/*/////////////////////////////////////////////////////*/
#define TOTAL_SEGMENTS 50                           /* Total number of strips to get */ 
RECV_BUF segments[TOTAL_SEGMENTS];                  /* To store all unique segments */ 
bool segments_received[TOTAL_SEGMENTS] = {false};   /* To track received segments */
int unique_segments_count = 0;
bool is_done_full_image = false;
pthread_mutex_t lock;

char server[3][256] = {"http://ece252-1", "http://ece252-2", "http://ece252-3"};
/*/////////////////////////////////////////////////////*/

/*/////////////////////////////////////////////////////*/
/* Function Definitions */
/*/////////////////////////////////////////////////////*/
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


/**
 * @brief write callback function to save a copy of received data in RAM.
 *        The received libcurl data are pointed by p_recv, 
 *        which is provided by libcurl and is not user allocated memory.
 *        The user allocated memory is at p_userdata. One needs to
 *        cast it to the proper struct to make good use of it.
 *        This function maybe invoked more than once by one invokation of
 *        curl_easy_perform().
 */

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


int recv_buf_init(RECV_BUF *ptr, size_t max_size)
{
    void *p = NULL;
    
    if (ptr == NULL) {
        return 1;
    }

    p = malloc(max_size);
    if (p == NULL) {
	return 2;
    }
    
    ptr->buf = p;
    ptr->size = 0;
    ptr->max_size = max_size;
    ptr->seq = -1;              /* valid seq should be non-negative */
    return 0;
}

int recv_buf_cleanup(RECV_BUF *ptr)
{
    if (ptr == NULL) {
	return 1;
    }
    
    free(ptr->buf);
    ptr->size = 0;
    ptr->max_size = 0;
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

void *fetch_strip(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    CURLcode res;

    while (1) {
        pthread_mutex_lock(&lock);
        if (is_done_full_image) {
            pthread_mutex_unlock(&lock);
            break;
        }
        pthread_mutex_unlock(&lock);

        res = curl_easy_perform(data->curl_handle);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            break;
        }

        pthread_mutex_lock(&lock);
        int seq = data->recv_buf.seq;
        if (seq >= 0 && seq < TOTAL_SEGMENTS && !segments_received[seq]) {
            /* Copy the data into the corresponding segment buffer */
            segments[seq].buf = malloc(data->recv_buf.size);
            memcpy(segments[seq].buf, data->recv_buf.buf, data->recv_buf.size);
            segments[seq].size = data->recv_buf.size;
            segments[seq].max_size = data->recv_buf.size;
            segments_received[seq] = true;
            unique_segments_count++;
            printf("Received segment %d (total unique: %d)\n", seq, unique_segments_count);
            if (unique_segments_count >= TOTAL_SEGMENTS) {
                is_done_full_image = true;
            }
        }
        pthread_mutex_unlock(&lock);

        // Reset recv_buf for the next fetch
        data->recv_buf.size = 0;
        data->recv_buf.seq = -1;
    }

    curl_easy_cleanup(data->curl_handle);
    free(data->recv_buf.buf);
    free(data);
    pthread_exit(NULL);
}

/*/////////////////////////////////////////////////////*/

/*******************CATPNG CODE HERE********************/
//modified code
#include "./starter/png_util/lab_png.h"
#include "./starter/png_util/zutil.c"
#include "./starter/png_util/crc.c"

#define PNG_SIGNATURE_SIZE 8
#define CHUNK_TYPE_SIZE 4
#define CHUNK_CRC_SIZE 4
#define CHUNK_LENGTH_SIZE 4

unsigned int hexToDec(unsigned char hex[], int length) {
    int result = 0;
    for (int i = 0; i < length; i++) {
        result |= hex[i] << (8 * (length - 1 - i));
    }
    return result;
}

int getWidth(RECV_BUF* buf) {
    unsigned char width[4];
    memcpy(width, buf->buf + 16, 4);
    int result = hexToDec(width, 4);
    return result;
}

int getHeight(RECV_BUF* buf) {
    unsigned char height[4];
    memcpy(height, buf->buf + 20, 4);
    int result = hexToDec(height, 4);
    return result;
}

//numSegments = 50?
int catpng(RECV_BUF segments[], int numSegments) {
    int width = 0;
    int catHeight = 0;
    unsigned char *IDAT_data = (unsigned char *)malloc(0 * sizeof(unsigned char));
    unsigned long IDAT_data_array_length = 0;
    U64 inflate = 0;
    U64 deflate = 0;
    for (int i = 0; i < numSegments; i++) {
        RECV_BUF* buf = &segments[i];
        
        width = getWidth(buf);
        unsigned char IDAT_size[4];

        memcpy(IDAT_size, buf->buf + 33, 4);

        U64 IDAT_length = hexToDec(IDAT_size, 4);
        U8 IDAT_data_length[IDAT_length]; //length of IDAT data

        memcpy(IDAT_data_length, buf->buf + 41, IDAT_length);

        deflate += IDAT_length;
        //we can assume all png widths are the same, no need to check different widths
        int currentWidth = getWidth(buf);
        int currentHeight = getHeight(buf);

        catHeight += currentHeight;
        width = currentWidth; //doesn't really do anything but allows to use width at the very end for writing output

        U64 size = currentHeight * (currentWidth * 4 + 1);
        U8 dest[size];

        int inf = mem_inf(dest, &size, IDAT_data_length, IDAT_length);
        IDAT_data = (unsigned char *) realloc(IDAT_data, (catHeight * (currentWidth * 4 + 1)) * sizeof(unsigned char));
        
        for (int i = 0; i < size; i++) {
            IDAT_data[inflate + i]= dest[i];
        }
        inflate += size;
    }
    unsigned char IDAT_data_output[deflate];
    int def = mem_def(IDAT_data_output, &deflate, IDAT_data, inflate, Z_DEFAULT_COMPRESSION);

    //every write needs to go through htonl()

    RECV_BUF* buf = &segments[0];

    FILE *output = fopen("all.png", "w+");
    unsigned char temp[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    fwrite(temp, 8, 1, output); //write "png" header

    unsigned char IHDR_type[8];
    memcpy(IHDR_type, buf->buf + 8, 8);
    //fseek(file, 8, SEEK_SET);
    //fread(IHDR_type, 8, 1, file);
    fwrite(IHDR_type, 8, 1, output); //copy length chunk

    //write height and width
    U32 IHDR_width = (U32) htonl(width);
    fwrite(&IHDR_width, 4, 1, output);

    U32 IHDR_height = (U32) htonl(catHeight);
    fwrite(&IHDR_height, 4, 1, output);

    //write bit depth, etc.
    unsigned char IHDR_misc[5];
    memcpy(IHDR_misc, buf->buf + 24, 5);
    //fseek(file, 24, SEEK_SET);
    //fread(IHDR_misc, 5, 1, file);
    fwrite(IHDR_misc, 5, 1, output);

    //calculate and write IHDR CRC
    unsigned char temp55[17];
    fseek(output, 12, SEEK_SET); //skip png header and length chunk
    fread(temp55, 17, 1, output);
    unsigned long IHDR_CRC = crc(temp55, 17);
    U32 IHDR_CRC32 = (U32) htonl(IHDR_CRC);
    fwrite(&IHDR_CRC32, 4, 1, output);

    //write IDAT length
    U32 deflate32 = (U32) htonl(deflate);
    fwrite(&deflate32, 4, 1, output);

    //write IDAT type
    //unsigned char IDAT_type[4];
    //memcpy(IHDR_type, buf->buf + 37, 4);
    //fseek(file, 37, SEEK_SET);
    //fread(IDAT_type, 4, 1, file);
    //fwrite(IDAT_type, 4, 1, output);
    //problem area here. IDAT won't write correctly with the above code
    unsigned char IDAT_type[4] = {'I', 'D', 'A', 'T'};
    fwrite(IDAT_type, 4, 1, output);

    //write IDAT data may need break point before this to see if bytes before are correct
    //https://hexed.it/
    fwrite(&IDAT_data_output, 1, deflate, output);

    //write IDAT CRC
    unsigned char temp2[deflate+4];
    fseek(output, 37, SEEK_SET); //skip header & IHDR
    fread(temp2, 1, deflate + 4, output); //read type and data
    unsigned long IDAT_CRC = crc(temp2, deflate + 4);
    U32 IDAT_CRC32 = (U32) htonl(IDAT_CRC);
    fwrite(&IDAT_CRC32, 4, 1, output);

    //write IEND length and type
    U32 len = 0;
    unsigned char chunkType[4] = {'I','E','N','D'};
    fwrite(&len, sizeof(len), 1, output); //0 length
    for(int i = 0; i < 4; i++) {
        fputc(chunkType[i], output); //put IEND in type
    }

    //write IEND CRC
    unsigned char temp3[4];
    fseek(output, 49 + deflate, SEEK_SET);
    fread(temp3, 1, 4, output);
    unsigned long IEND_CRC = crc(temp3, 4);
    U32 IEND_CRC32 = (U32) htonl(IEND_CRC);
    fwrite(&IEND_CRC32, 4, 1, output);

    //clean up
    fclose(output);
    free(IDAT_data);
    return 0;
}

int main(int argc, char **argv)
{
    int c;
    int t = 1;
    int n = 1;
    char *str = "option requires an argument";
    
    /* GETOPT MAIN CODE */
    while ((c = getopt (argc, argv, "t:n:")) != -1) {
        switch (c) {
        case 't':
            t = strtoul(optarg, NULL, 10);
            printf("option -t specifies a value of %d.\n", t);
            if (t <= 0) {
                fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
                return -1;
            }
            break;
        case 'n':
            n = strtoul(optarg, NULL, 10);
	    printf("option -n specifies a value of %d.\n", n);
            if (n <= 0 || n > 3) {
                fprintf(stderr, "%s: %s 1, 2, or 3 -- 'n'\n", argv[0], str);
                return -1;
            }
            break;
        default:
            return -1;
        }
    }
    /*//////////////////// PARSES INPUT ////////////////////*/
    /* Now we can use t and n for other operations*/

    /* cURL MAIN CODE */
    //CURL *curl_handle;
    //CURLcode res;
    char url[3][256];
    char hold_url[256];
    RECV_BUF recv_buf;
    //char fname[256]; /* For file */
    //pid_t pid =getpid();
    pthread_t JIthread[t];
    
    //recv_buf_init(&recv_buf, BUF_SIZE);
    
    /* Sets url to appropriate url to access from given n */
    if (n == 1) 
    {
        sprintf(hold_url, "%s%d", IMG_URL, 1);
        for (int i = 0; i < 3; i++)
        {
            /* String Stuff */
            strcat(server[i], hold_url);
            strcpy(url[i], server[i]);
        }
    } 
    else if (n == 2) 
    {
        sprintf(hold_url, "%s%d", IMG_URL, 2);
        for (int i = 0; i < 3; i++)
        {
            /* String Stuff */
            strcat(server[i], hold_url);
            strcpy(url[i], server[i]);
        }
    }
    else
    {
        sprintf(hold_url, "%s%d", IMG_URL, 3);
        for (int i = 0; i < 3; i++)
        {
            /* String Stuff */
            strcat(server[i], hold_url);
            strcpy(url[i], server[i]);
        }
    }
    printf("%s: URLs are %s, %s, %s\n", argv[0], url[0], url[1], url[2]);


    pthread_mutex_init(&lock, NULL);
    curl_global_init(CURL_GLOBAL_DEFAULT);

    /* start JIthread */
    for (int i = 0; i < t; i++)
    {
        thread_data_t *data = malloc(sizeof(thread_data_t));
        /* init a curl session */
        data->curl_handle = curl_easy_init();
        if (data->curl_handle == NULL) {
            fprintf(stderr, "curl_easy_init: returned NULL\n");
            return 1;
        }
        //strcpy(data->url, url);
        data->thread_id = i;
        recv_buf_init(&data->recv_buf, BUF_SIZE);


        /* specify URL to get -- change to multiple servers */
        int hold = i%3;
        printf("Using server #%d\n", hold);
        curl_easy_setopt(data->curl_handle, CURLOPT_URL, url[hold]);

        /* register write call back function to process received data */
        curl_easy_setopt(data->curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
        /* user defined data structure passed to the call back function */
        curl_easy_setopt(data->curl_handle, CURLOPT_WRITEDATA, (void *)&data->recv_buf);

        /* register header call back function to process received header data */
        curl_easy_setopt(data->curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
        /* user defined data structure passed to the call back function */
        curl_easy_setopt(data->curl_handle, CURLOPT_HEADERDATA, (void *)&data->recv_buf);

        /* some servers requires a user-agent field */
        curl_easy_setopt(data->curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        int thread_time = pthread_create(&JIthread[i], NULL, fetch_strip, (void *)data);
        if (thread_time) {
            fprintf(stderr, "Error: pthread_create() returned %d\n", thread_time);
            return 1;
        }
    }

    /* Rejoin all threads */
    for (int i=0; i<t; i++) {
        pthread_join(JIthread[i], NULL);
        printf("Thread ID %p joined.\n", (void*)JIthread[i]); 
    }
    


    /* CONCATENATE IMAGE HERE */

    catpng(segments, TOTAL_SEGMENTS);

    /*/////////////////////////////////////////////////////*/

    for(int i = 0; i < TOTAL_SEGMENTS; i++) {
        recv_buf_cleanup(&segments[i]);
    }



    /* Writes to a new file -- Should be done at the end after img is concatenated */
    // sprintf(fname, "./output_%d_%d.png", recv_buf.seq, pid);
    // write_file(fname, recv_buf.buf, recv_buf.size);

    /* cleaning up */
    //curl_easy_cleanup(curl_handle);
    //recv_buf_cleanup(&recv_buf);
    pthread_mutex_destroy(&lock);
    curl_global_cleanup();
    
    
    /* cleaning up */
    
    //free(JIthread);
    
    /* the memory was allocated in the do_work thread for return values */
    /* we need to free the memory here */
    // for (int i=0; i<t; i++) {
    //     free(p_results[i]);
    // }



    return 0;
}