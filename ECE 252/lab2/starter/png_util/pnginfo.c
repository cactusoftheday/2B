#include <stdio.h>    /* for printf(), perror()...   */
#include <stdlib.h>   /* for malloc()                */
#include <errno.h>    /* for errno                   */
#include "crc.h"      /* for crc()                   */
#include "zutil.h"    /* for mem_def() and mem_inf() */
#include "lab_png.h"  /* simple PNG data structures  */
#include <arpa/inet.h> /* For htonl, ntohl  */


int main(int argc, char *argv[]) 
{
    printf("Hello World!\n");

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    // Read the first 8 bytes to check PNG signature
    U8 signature[8];
    if (fread(signature, 1, 8, fp) != 8) {
        perror("fread signature");
        fclose(fp);
        return EXIT_FAILURE;
    }

    if (!is_png(signature, 8)) {
        printf("%s: Not a PNG file\n", filename);
        fclose(fp);
        return EXIT_FAILURE;
    }

    //error here
    struct data_IHDR ihdr;
    if (get_png_data_IHDR(&ihdr, fp, 8, SEEK_SET) != 0) {
        printf("%s: Failed to read IHDR chunk\n", filename);
        fclose(fp);
        return EXIT_FAILURE;
    }

    int width = get_png_width(&ihdr);
    int height = get_png_height(&ihdr);
    printf("%s: %d x %d\n", filename, width, height);

    fclose(fp);
    return EXIT_SUCCESS;
}

int is_png(U8 *buf, size_t n)
{
    if (n < 8) {
        return 0; // Not enough data to verify PNG signature
    }

    // PNG file signature
    U8 png_signature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    
    // Compare the file signature with the PNG signature
    if (memcmp(buf, png_signature, 8) == 0) {
        printf("It's a PNG!\n");
        return 1; // It's a PNG file
    } else {
        printf(":( It's NOT a PNG.\n");
        return 0; // Not a PNG file
    }
}

int get_png_height(struct data_IHDR *buf)
{
    return buf->height;
}

int get_png_width(struct data_IHDR *buf)
{
    return buf->width;
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

    length = ntohl(length); // Convert length to host byte order

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

    // Read the IHDR data
    if (fread(out, sizeof(struct data_IHDR), 1, fp) != 1) {
        perror("fread IHDR data");
        return -1;
    }

    // Convert the IHDR fields to host byte order
    out->width = ntohl(out->width);
    out->height = ntohl(out->height);

    // Read and verify the CRC
    U32 crc, expected_crc;
    if (fread(&crc, sizeof(crc), 1, fp) != 1) {
        perror("fread CRC");
        return -1;
    }

    // Calculate expected CRC
    expected_crc = update_crc(0xffffffffL, chunk_type, 4);
    expected_crc = update_crc(expected_crc, (U8 *)out, sizeof(struct data_IHDR));
    expected_crc ^= 0xffffffffL; // Finalize the CRC

    crc = ntohl(crc); // Convert CRC to host byte order

    // Debug prints for troubleshooting
    printf("Computed CRC: %08x, Expected CRC: %08x\n", expected_crc, crc);

    if (crc != expected_crc) {
        fprintf(stderr, "IHDR chunk CRC error: computed %08x, expected %08x\n", expected_crc, crc);
        return -1;
    }

    return 0;
}