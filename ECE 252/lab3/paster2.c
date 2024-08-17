// fork the number of producers and consumers. parent process waits and then does concatenation.

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <curl/curl.h>
#include <pthread.h>
#include "paster2.h"
#include <getopt.h>
#include "starter/png_util/zutil.c"
#include "starter/png_util/lab_png.h"
#include "starter/png_util/crc.c"
#include <zlib.h>
#include <sys/stat.h>


#define MAX_SIZE 1048576
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
int catpng(RECV_BUF segments[], int numSegments, char* name) {
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

    FILE *output = fopen(name, "w+");
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

void ensure_directory_exists(const char *path) {
    struct stat st = {0};

    if (stat(path, &st) == -1) {
        mkdir(path, 0700);
    }
}

void retrieve ( char url[256], RECV_BUF *p_shm_recv_buf )
{
    CURL *curl_handle;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    /* init a curl session */
    curl_handle = curl_easy_init();

    if (curl_handle == NULL) {
        fprintf(stderr, "curl_easy_init: returned NULL\n");
        return;
    }

    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    /* register write call back function to process received data */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl); 
    
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)p_shm_recv_buf);

    /* register header call back function to process received header data */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)p_shm_recv_buf);

    /* some servers requires a user-agent field */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");


    /* get it! */
    res = curl_easy_perform(curl_handle);

    if( res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
        //printf("URL is: %s\n", url);
        printf("%lu bytes received in memory %p, seq=%d.\n", p_shm_recv_buf->size, p_shm_recv_buf->buf, p_shm_recv_buf->seq);
    
    }
    /* cleaning up */
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
}


int produce ( int fork_i, int B, int P, int N, ISTACK *pstack, sem_t *sems) 
{
    
    char url[1024];
    /* **** URL conversion *** */
    char server_url[3][256];
    char hold_url[256];
    /* Sets url to appropriate url to access from given n */
    if (N == 1) 
    {
        sprintf(hold_url, "%s%d", IMG_URL, 1);
        for (int i = 0; i < 3; i++)
        {
            /* String Stuff */
            strcat(server[i], hold_url);
            strcpy(server_url[i], server[i]);
        }
    } 
    else if (N == 2) 
    {
        sprintf(hold_url, "%s%d", IMG_URL, 2);
        for (int i = 0; i < 3; i++)
        {
            /* String Stuff */
            strcat(server[i], hold_url);
            strcpy(server_url[i], server[i]);
        }
    }
    else
    {
        sprintf(hold_url, "%s%d", IMG_URL, 3);
        for (int i = 0; i < 3; i++)
        {
            /* String Stuff */
            strcat(server[i], hold_url);
            strcpy(server_url[i], server[i]);
        }
    }
    /* **** URL conversion *** */

    for (int i = 0; i < 50; i++) /* Map it to each producer */ /*wait(spaces) wait(mutex) post(items) post(mutex) --- wait(items)....*/
    {
        if (i%P == (fork_i)) /* If we are in producer i, denoted as fork_i */
        {
            int server = i%3;       // modulo the number of producers with the img segment
            char hold_url[256];
            sprintf(hold_url, "%s%d", IMG_URL_PART_3, i);
            sprintf(url, "%s%s", server_url[server], hold_url);
            //printf("Before URL print \n");
            printf("URL #%d is %s\n", i, url);

            //printf("ID was generated: %d \n", shmid);
            //shmid_producer = shmget(IPC_PRIVATE, MAX_SIZE, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
            RECV_BUF *p_shm_recv_buf = malloc(sizeof(RECV_BUF));
            recv_buf_init(p_shm_recv_buf, MAX_SIZE);
            retrieve(url, p_shm_recv_buf);
            //printf("After recv buf init \n");
            //printf("Recv_buf: %s \n", p_shm_recv_buf->buf);
            
            sem_wait(&sems[0]);
            //pthread_mutex_lock(&lock);
            sem_wait(&sems[2]);
            if(!is_full(pstack)){
                printf("push %d \n", p_shm_recv_buf->seq);
                push(pstack, p_shm_recv_buf->seq);
            }
            //pthread_mutex_unlock(&lock);
            char fname[256];
            snprintf(fname, sizeof(fname), "./strips/strip%d.png", p_shm_recv_buf->seq);
            //printf("%s\n", fname);
            ensure_directory_exists("./strips");
            catpng(p_shm_recv_buf, 1, fname); 
            recv_buf_cleanup(p_shm_recv_buf);
            free(p_shm_recv_buf);
            sem_post(&sems[2]);
            sem_post(&sems[1]);
        }
        else
        {
            continue;
        }
    }

    return 0;
}

int consume ( int fork_i, int C, int X, ISTACK *pstack, sem_t *sems, int shmid_deflated_count, int shmid_inflated_count, int shmid_inflated)
{
    for (int i = 0; i < 50; i++) /* Map it to each producer */ /*wait(spaces) wait(mutex) post(items) post(mutex) --- wait(items)....*/
    {
        if (i%C == (fork_i)) /* If we are in producer fork_i */
        {
            U64 *deflated_count = shmat(shmid_deflated_count, NULL, 0);
            U64 *inflated_count = shmat(shmid_inflated_count, NULL, 0);
            U8 *inflate = shmat(shmid_inflated, NULL, 0);

            int p_sequence_id;
            
            sem_wait(&sems[1]);
            usleep(X * 1000);
            //pthread_mutex_lock(&lock);
            sem_wait(&sems[2]);
            pop(pstack, &p_sequence_id);
            printf("we popped off! %d\n", p_sequence_id);
            sem_post(&sems[2]);
            //pthread_mutex_unlock(&lock);
            sem_post(&sems[0]);
            printf("After sems 0 \n");
            char fname[256];
            snprintf(fname, sizeof(fname), "./strips/strip%d.png", p_sequence_id);
            printf("After consumer snprintf \n");

            FILE *fp;
            fp = fopen(fname, "rb");
            printf("After file open \n");
            unsigned char IDAT_size[4];

            fseek(fp, 33, SEEK_SET);
            fread(IDAT_size, 4, 1, fp);
            U64 IDAT_length = hexToDec(IDAT_size, 4);
            U8 IDAT_data_length[IDAT_length]; //length of IDAT data

            fseek(fp, 41, SEEK_SET);
            fread(IDAT_data_length, IDAT_length, 1, fp);
            
            U64 size = 6 * (400 * 4 + 1);
            U8 dest[size];

            int inf = mem_inf(dest, &size, IDAT_data_length, IDAT_length);
            printf("Inflating result %d \n", inf);
            sem_wait(&sems[3]); //not sure if semaphores needed for this part
            //pthread_mutex_lock(&lock);
            printf("Entering sems 2\n");
            *deflated_count = *deflated_count + IDAT_length;
            for(int j = 0; j < size; j++) {
                inflate[size * p_sequence_id + j] = dest[j];
            }
            *inflated_count = *inflated_count + size;
            fclose(fp);
            sem_post(&sems[3]);
        }
        else
        {
            continue;
        }
    }
    return 0;
}


// from 1-50 pop the top item from the buffer (protected by semaphore) and decompress and put into size 5 multibuffer from lab2

int main(int argc, char *argv[])
{
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <B> <P> <C> <X> <N>\n", argv[0]);
        exit(1);
        return 0;
    }
    
    int B = atoi(argv[1]);
    int P = atoi(argv[2]);
    int C = atoi(argv[3]);
    int X = atoi(argv[4]);
    int N = atoi(argv[5]);

    if (B < 1 || P < 1 || C < 1 || X < 0 || N < 1 || N > 3)
    {
        fprintf(stderr, "Invalid input for one or more of <B> <P> <C> <X> <N>\n");
        exit(1);
    }

    printf("\nValues entered --> <B>: %d <P>: %d <C>: %d <X>: %d <N>: %d\n\n", B, P, C, X, N);


    /////////////////// FORK ////////////////////////
    int i = 0;
    pid_t pid = 0;
    pid_t cpids[P+C];
    int state;
    double times[2];
    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0) /* Start Timer */
    {
        perror("gettimeofday");
        abort();
    }
    times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

    int shm_size_stack = sizeof_shm_stack(B);
    
    printf("shm_stack_size=%d\n", shm_size_stack);
    /* We do not use malloc() to create shared memory, use shmget() */
    shmid_ring = shmget(IPC_PRIVATE, shm_size_stack, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);    
    if (shmid_ring == -1 ) {
        perror("shmget");
        abort();
    }

    struct int_stack *pstack;    
    pstack = shmat(shmid_ring, NULL, 0);
    if ( pstack == (void *) -1 ) {
        perror("shmat");
        abort();
    }
    init_shm_stack(pstack, B);

    /* Producer & consumer shm */
    int shm_size = sizeof_shm_recv_buf(BUF_SIZE);
    
    printf("shm_size = %d.\n", shm_size);

    // catpng shared memories
    int shmid_deflated_count = shmget(IPC_PRIVATE, sizeof(U64), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    int shmid_inflated_count = shmget(IPC_PRIVATE, sizeof(U64), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    int shmid_inflated = shmget(IPC_PRIVATE, sizeof(U8) * (300 * (400 * 4 + 1)), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

    if(shmid_deflated_count == -1 || shmid_inflated_count == -1 || shmid_inflated == -1) {
        perror("shmget");
        abort();
    }

    //U64 *deflate_count = malloc(sizeof(U64));
    //U64 *inflate_count = malloc(sizeof(U64));
    //U8 *inflate = malloc(sizeof(U8));

    U64 *deflate_count = shmat(shmid_deflated_count, NULL, 0);
    U64 *inflate_count = shmat(shmid_inflated_count, NULL, 0);
    U8 *inflate = shmat(shmid_inflated, NULL, 0);

    memset(inflate, '\0', sizeof(U8) * (300 * (400 * 4 + 1))); //data of image saved here


    printf("Past catpng memset \n");  
    // shm semaphores intializations //
    sem_t *sems;        /* &sem[0] is to monitor how many empty spaces are left */
    void *buf;          /* &sem[1] is to monitor how many spaces are filled */

    /* allocate two shared memory regions */
    int shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    int shmid_sems = shmget(IPC_PRIVATE, sizeof(sem_t) * NUM_SEMS, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

    if (shmid == -1 || shmid_sems == -1) {
        perror("shmget");
        abort();
    }

    /* attach to shared memory regions */
    buf = shmat(shmid, NULL, 0);
    sems = shmat(shmid_sems, NULL, 0);
    if ( buf == (void *) -1 || sems == (void *) -1 ) {
        perror("shmat");
        abort();
    }

    /* initialize shared memory varaibles */
    memset(buf, 0, SHM_SIZE);
    if ( sem_init(&sems[0], SEM_PROC, B) != 0 ) {
        perror("sem_init(sem[0])");
        abort();
    }
    if ( sem_init(&sems[1], SEM_PROC, 0) != 0 ) {
        perror("sem_init(sem[1])");
        abort();
    }
    if ( sem_init(&sems[2], SEM_PROC, 1) != 0 ) {
        perror("sem_init(sem[2])");
        abort();
    }
    if ( sem_init(&sems[3], SEM_PROC, 1) != 0 ) {
        perror("sem_init(sem[3])"); //this is for writing to the inflate value
        abort();
    }

    pthread_mutex_init(&lock, NULL);

    for ( i = 0; i < P; i++) /* Fork the Producers */
    {
        pid = fork();

        if ( pid > 0 ) {        /* parent proc */
            cpids[i] = pid;
        } else if ( pid == 0 ) { /* child proc */
            //worker(i); // make the cURL thing
            produce(i, B, P, N, pstack, sems);
            exit(0);
        } else {
            perror("fork");
            abort();
        }
    }

    for ( i = P; i < C+P; i++) /* Fork the Consumers */
    {
        pid = fork();

        if ( pid > 0 ) {        /* parent proc */
            cpids[i] = pid;
        } else if ( pid == 0 ) { /* child proc */
            consume(i-P, C, X, pstack, sems, shmid_deflated_count, shmid_inflated_count, shmid_inflated); // consume
            exit(0);
        } else {
            perror("fork");
            abort();
        }
        
    }


    if ( pid > 0 ) {               /* parent process --> at very end */
        for ( i = 0; i < P+C; i++ ) {
            waitpid(cpids[i], &state, 0);
            if (WIFEXITED(state)) {
                printf("Child cpid[%d]=%d terminated with state: %d.\n", i, cpids[i], state);
            }
                
        }
    }
/////////////////// FORK END ////////////////////////

    /* @Isaac concat things */ 
    
    char first_url[256];
    RECV_BUF recv_buf;
    // CURLcode res;
    sprintf(first_url,"http://ece252-%d.uwaterloo.ca:2530/image?img=%d&part=%d",1,N,0);
    
    recv_buf_init(&recv_buf, MAX_SIZE);
    retrieve(first_url, &recv_buf);

    unsigned char IDAT_data_output[*deflate_count];
    int def = mem_def(IDAT_data_output, deflate_count, inflate, *inflate_count, Z_DEFAULT_COMPRESSION);
    printf("deflate value %d \n",def);

    FILE *output = fopen("all.png", "w+");

    unsigned char temp[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    fwrite(temp, 8, 1, output);

    //IHDR writing
    unsigned char IHDR_type[8];
    memcpy(IHDR_type, recv_buf.buf + 8, 8);
    fwrite(IHDR_type, 8, 1, output); //copy length chunk

    U32 width = (U32) htonl(400);
    U32 height = (U32) htonl(300);

    fwrite(&width, 4, 1, output);
    fwrite(&height, 4, 1, output);

    unsigned char IHDR_misc[5];
    memcpy(IHDR_misc, recv_buf.buf + 24, 5);
    fwrite(IHDR_misc, 5, 1, output);

    //calculate and write IHDR CRC
    unsigned char temp55[17];
    fseek(output, 12, SEEK_SET); //skip png header and length chunk
    fread(temp55, 17, 1, output);
    unsigned long IHDR_CRC = crc(temp55, 17);
    U32 IHDR_CRC32 = (U32) htonl(IHDR_CRC);
    fwrite(&IHDR_CRC32, 4, 1, output);

    U32 deflate32 = (U32) htonl(*deflate_count);
    fwrite(&deflate32, 4, 1, output);

    unsigned char IDAT_type[4] = {'I', 'D', 'A', 'T'};
    fwrite(IDAT_type, 4, 1, output);

    fwrite(&IDAT_data_output, 1, *deflate_count, output);

    //write IDAT CRC
    unsigned char temp2[*deflate_count + 4];
    fseek(output, 37, SEEK_SET); //skip header & IHDR
    fread(temp2, 1, *deflate_count + 4, output); //read type and data
    unsigned long IDAT_CRC = crc(temp2, *deflate_count + 4);
    U32 IDAT_CRC32 = (U32) htonl(IDAT_CRC);
    fwrite(&IDAT_CRC32, 4, 1, output);

    //IEND

    U32 len = 0;
    unsigned char chunkType[4] = {'I','E','N','D'};
    fwrite(&len, sizeof(len), 1, output); //0 length
    for(int i = 0; i < 4; i++) {
        fputc(chunkType[i], output); //put IEND in type
    }
    
    //write IEND CRC
    unsigned char temp3[4];
    fseek(output, 49 + *deflate_count, SEEK_SET);
    fread(temp3, 1, 4, output);
    unsigned long IEND_CRC = crc(temp3, 4);
    U32 IEND_CRC32 = (U32) htonl(IEND_CRC);
    fwrite(&IEND_CRC32, 4, 1, output);

    // unsigned char temp3[4];
    // memcpy(&temp3, output + 49 + *deflate_count, 4);
    // unsigned long IEND_CRC = crc(temp3, 4);
    // U32 IEND_CRC32 = (U32) htonl(IEND_CRC);
    // memcpy(output + 53 + *deflate_count, &IEND_CRC32, 4);

    //write_file("./all.png", output, *deflate_count + 57); //33 + 12 + 12

    fclose(output);
    recv_buf_cleanup(&recv_buf);

    /* Total execution time */
    if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
    times[1] = (tv.tv_sec) + tv.tv_usec/1000000.;
    printf("Paster2 execution time: %.6lf seconds\n", times[1] - times[0]);


    //destroy semaphores
    if ( shmdt(buf) != 0 ) {
        perror("shmdt");
        abort();
    }

    shmctl(shmid_deflated_count, IPC_RMID, NULL);
    shmctl(shmid_inflated_count, IPC_RMID, NULL);
    shmctl(shmid_inflated, IPC_RMID, NULL);
    shmctl(buf, IPC_RMID, NULL);
    if ( shmctl(shmid, IPC_RMID, NULL) == -1 ) {
        perror("shmctl");
        abort();
    }
    

    if (sem_destroy(&sems[0]) || sem_destroy(&sems[1]) || sem_destroy(&sems[2]) || sem_destroy(&sems[3])) {
        perror("sem_destroy");
        abort();
    } 

    if ( shmdt(sems) != 0 ) {
        perror("shmdt");
        abort();
    }

    shmctl(sems, IPC_RMID, NULL);

    pthread_mutex_destroy(&lock);

    return 0;
}