/********************************************************************
 * @file: findpng.c
 * @brief: List all pngs within a given directory
 * @author: Joshwyn
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>

#define PNG_SIGNATURE "\x89PNG\r\n\x1A\n"
#define PNG_SIG_SIZE    8 /* number of bytes of png image signature data */
#define CHUNK_LEN_SIZE  4 /* chunk length field size in bytes */          
#define CHUNK_TYPE_SIZE 4 /* chunk type field size in bytes */
#define CHUNK_CRC_SIZE  4 /* chunk CRC field size in bytes */
#define DATA_IHDR_SIZE 13 /* IHDR chunk data field size */

/******************************************************************************
 * STRUCTURES and TYPEDEFS 
 *****************************************************************************/
typedef unsigned char U8;
typedef unsigned int  U32;

typedef struct chunk {
    U32 length;  /* length of data in the chunk, host byte order */
    U8  type[4]; /* chunk type */
    U8  *p_data; /* pointer to location where the actual data are */
    U32 crc;     /* CRC field  */
} *chunk_p;

/* note that there are 13 Bytes valid data, compiler will padd 3 bytes to make
   the structure 16 Bytes due to alignment. So do not use the size of this
   structure as the actual data size, use 13 Bytes (i.e DATA_IHDR_SIZE macro).
 */
//#pragma pack(push, 1)
typedef struct data_IHDR {// IHDR chunk data 
    U32 width;        /* width in pixels, big endian   */
    U32 height;       /* height in pixels, big endian  */
    U8  bit_depth;    /* num of bits per sample or per palette index.
                         valid values are: 1, 2, 4, 8, 16 */
    U8  color_type;   /* =0: Grayscale; =2: Truecolor; =3 Indexed-color
                         =4: Greyscale with alpha; =6: Truecolor with alpha */
    U8  compression;  /* only method 0 is defined for now */
    U8  filter;       /* only method 0 is defined for now */
    U8  interlace;    /* =0: no interlace; =1: Adam7 interlace */
} *data_IHDR_p;
//#pragma pack(pop)

/* A simple PNG file format, three chunks only*/
typedef struct simple_PNG {
    struct chunk *p_IHDR;
    struct chunk *p_IDAT;  /* only handles one IDAT chunk */  
    struct chunk *p_IEND;
} *simple_PNG_p;


/******************************************************************************
 * Functions 
 *****************************************************************************/

/* Table of CRCs of all 8-bit messages. */
unsigned long crc_table[256];

/* Flag: has the table been computed? Initially false. */
int crc_table_computed = 0;

/* Make the table for a fast CRC. */
void make_crc_table(void)
{
    unsigned long c;
    int n, k;

    for (n = 0; n < 256; n++) {
        c = (unsigned long) n;
        for (k = 0; k < 8; k++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[n] = c;
    }
    crc_table_computed = 1;
}

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
   should be initialized to all 1's, and the transmitted value
   is the 1's complement of the final running CRC (see the
   crc() routine below)). */

unsigned long update_crc(unsigned long crc, unsigned char *buf, int len)
{
    unsigned long c = crc;
    int n;

    if (!crc_table_computed)
        make_crc_table();
    for (n = 0; n < len; n++) {
        c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    }
    return c;
}

/* Return the CRC of the bytes buf[0..len-1]. */
unsigned long crc(unsigned char *buf, int len)
{
    return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}


int get_png_data_IHDR(struct data_IHDR *out, FILE *fp, long offset, int whence) {
    // Seek to the offset where IHDR chunk is expected
    if (fseek(fp, offset, whence) != 0) {
        perror("fseek");
        return -1;
    }

    // Read the length of the IHDR chunk (should be 13 for IHDR)
    U32 length;
    if (fread(&length, sizeof(length), 1, fp) != 1) {
        perror("fread length");
        return -1;
    }

    length = ntohl(length);

    if (length != 13) {
        fprintf(stderr, "Invalid IHDR chunk length: %u\n", length);
        return -1;
    }

    // Read the IHDR chunk type (should be 'IHDR')
    U8 chunk_type[4];
    if (fread(chunk_type, sizeof(chunk_type), 1, fp) != 1) {
        perror("fread chunk type");
        return -1;
    }

    if (memcmp(chunk_type, "IHDR", 4) != 0) {
        fprintf(stderr, "Expected IHDR chunk, but found: %c%c%c%c\n", chunk_type[0], chunk_type[1], chunk_type[2], chunk_type[3]);
        return -1;
    }

    if (fread(out, DATA_IHDR_SIZE, 1, fp) != 1) {
        perror("fread IHDR data");
        return -1;
    }

    U32 actual_crc;
    if (fread(&actual_crc, sizeof(actual_crc), 1, fp) != 1) {
        perror("fread CRC");
        return -1;
    }
    actual_crc = ntohl(actual_crc);

    unsigned char crc_buffer[4 + DATA_IHDR_SIZE];
    memcpy(crc_buffer, chunk_type, 4);
    memcpy(crc_buffer + 4, out, DATA_IHDR_SIZE);

    unsigned long expected_crc = crc(crc_buffer, 4 + DATA_IHDR_SIZE);

    out->width = ntohl(out->width);
    out->height = ntohl(out->height);

    if (actual_crc != expected_crc) {
        fprintf(stderr, "IHDR chunk CRC error: computed %08lx, expected %08x\n", expected_crc, actual_crc);
        return -1;
    }

    return 0;
}


int is_png(char*possible_png_file)
{
    FILE *fp = fopen(possible_png_file, "rb");
    if (!fp) {
        return 0;
    }
    
    char buffer[PNG_SIG_SIZE];
    size_t bytesRead = fread(buffer, 1, PNG_SIG_SIZE, fp);
    fclose(fp);
    
    if (bytesRead != PNG_SIG_SIZE) {
        return 0;
    }
    
    //return memcmp(buffer, PNG_SIGNATURE, PNG_SIG_SIZE) == 0;
    if (memcmp(buffer, PNG_SIGNATURE, PNG_SIG_SIZE) == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}


/*/The scavenge function "scavenges" through the given directory printing out all paths to a valid png/*/
int scavenge(char*search_directory, char*home)
{
    /*/////////////// lists file names in directory ///////////////*/
    DIR *p_dir;
    struct dirent *p_dirent;
    char str[64];
    int empty_directory = 1;

    /* maybe move to main */
    if ((p_dir = opendir(search_directory)) == NULL) 
    {
        sprintf(str, "opendir(%s)", search_directory);
        perror(str);
        exit(2);
    }

    while ((p_dirent = readdir(p_dir)) != NULL) 
    {
        if (strcmp(p_dirent->d_name, ".") == 0)
        {
            continue;
        }
        if (strcmp(p_dirent->d_name, "..") == 0)
        {
            continue;
        }
        
        char Path[4096]; /*/ Overall path that has been taken /*/
        snprintf(Path, sizeof(Path), "%s/%s", search_directory, p_dirent->d_name);
        
        struct stat buf;
        stat(Path, &buf);
        
        if (S_ISREG(buf.st_mode)) /* Is Regular File */
        {
            if (is_png(Path) == 0) 
            {
                empty_directory = 0;
                char pngPath[4096];
                snprintf(pngPath, sizeof(pngPath), "%s/%s", home, p_dirent->d_name);
                printf("%s\n", pngPath);
            }
        } 
        else if (S_ISDIR(buf.st_mode)) /* Is Directory */
        {
            char newPath[4069];
            snprintf(newPath, sizeof(newPath), "%s/%s", home, p_dirent->d_name);
            int hold = scavenge(Path, newPath);
            empty_directory = hold && empty_directory;
        }
        else if (S_ISLNK(buf.st_mode))
        {
            continue;
        }
    }

    if ( closedir(p_dir) != 0 ) {
        perror("closedir");
        exit(3);
    }

    if (empty_directory == 1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
    /*//////////////////////////////////////////////////////////////*/

}


/****************************************************************************** 
 *****************************************************************************/


int main(int argc, char *argv[]) 
{
    if (argc == 1) {
        fprintf(stderr, "Usage: %s <directory name>\n", argv[0]);
        exit(1);
    }

    char*search_directory = argv[1];
    
    if (scavenge(search_directory, ".") == 1)
    {
        printf("findpng: No PNG file found\n");
    }
    
    return 0;

}
