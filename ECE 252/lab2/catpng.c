/********************************************************************
 * @file: catpng.c
 * @brief: Concatenate pngs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int getWidth(FILE* fp) {
    unsigned char width[4];
    fseek(fp, 16, SEEK_SET);
    fread(width, 4, 1, fp);
    int result = hexToDec(width, 4);
    return result;
}

int getHeight(FILE* fp) {
    unsigned char height[4];
    fseek(fp,20, SEEK_SET);
    fread(height, 4, 1, fp);
    int result = hexToDec(height, 4);
    return result;
}

int main(int argc, char *argv[]) 
{
    if(argc < 2) {
        printf("not enough arguments");
        return 1;
    }

    int width = 0;
    int catHeight = 0;
    unsigned char *IDAT_data = (unsigned char *)malloc(0 * sizeof(unsigned char));
    unsigned long IDAT_data_array_length = 0;
    U64 inflate = 0;
    U64 deflate = 0;
    for (int i = 1; i < argc; i++) {
        //can't count arg 0
        FILE *fp;
        fp = fopen(argv[i], "rb");
        
        width = getWidth(fp);
        unsigned char IDAT_size[4];

        fseek(fp, 33, SEEK_SET);
        fread(IDAT_size, 4, 1, fp);

        U64 IDAT_length = hexToDec(IDAT_size, 4);
        U8 IDAT_data_length[IDAT_length]; //length of IDAT data

        fseek(fp, 41, SEEK_SET);
        fread(IDAT_data_length, IDAT_length, 1, fp);

        deflate += IDAT_length;
        //we can assume all png widths are the same, no need to check different widths
        int currentWidth = getWidth(fp);
        int currentHeight = getHeight(fp);

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

        fclose(fp);
    }
    unsigned char IDAT_data_output[deflate];
    int def = mem_def(IDAT_data_output, &deflate, IDAT_data, inflate, Z_DEFAULT_COMPRESSION);

    //every write needs to go through htonl()

    FILE *file; //used to get the header and footer data again
    file = fopen(argv[1], "rb");

    FILE *output = fopen("all.png", "w+");
    unsigned char temp[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    fwrite(temp, 8, 1, output); //write "png" header

    unsigned char IHDR_type[8];
    fseek(file, 8, SEEK_SET);
    fread(IHDR_type, 8, 1, file);
    fwrite(IHDR_type, 8, 1, output); //copy length chunk

    //write height and width
    U32 IHDR_width = (U32) htonl(width);
    fwrite(&IHDR_width, 4, 1, output);

    U32 IHDR_height = (U32) htonl(catHeight);
    fwrite(&IHDR_height, 4, 1, output);

    //write bit depth, etc.
    unsigned char IHDR_misc[5];
    fseek(file, 24, SEEK_SET);
    fread(IHDR_misc, 5, 1, file);
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
    unsigned char IDAT_type[4];
    fseek(file, 37, SEEK_SET);
    fread(IDAT_type, 4, 1, file);
    fwrite(IDAT_type, 4, 1, output);

    //write IDAT data may need break point before this to see if bytes before are correct
    //https://hexed.it/
    fwrite(&IDAT_data_output, 1, deflate, output);

    //write IDAT CRC
    unsigned char temp2[deflate+4];
    fseek(output, 37, SEEK_SET); //skip header & IHDR
    fread(temp2, 1, deflate + 4, output); //read type and data
    unsigned long IDAT_CRC = crc(temp2, deflate);
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
    fclose(file);
    fclose(output);
    free(IDAT_data);

    return 0;
}