/*ECE 252 Lab 1 Group 1 - Joshwyn Parekh & Isaac Huang*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>    /* for errno                   */
#include "crc.h"      /* for crc()                   */
#include "zutil.h"    /* for mem_def() and mem_inf() */
#include "lab_png.h"  /* simple PNG data structures  */

int is_dir(char* path) {
    struct stat buf;
    if(lstat(path, &buf) < 0) {
        perror("lstat error");
        exit(3);
    }

    else if(S_ISREG(buf.st_mode)) {
        return 0;
    }

    else if(S_ISDIR(buf.st_mode)) {
        return 1;
    }
    return -1;
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


/*Traverse subdirectories as required, and do not follow symbolic links*/
void traverse(char* name, int* png_flag) {
    DIR *p_dir;
    struct dirent *p_dirent;
    char path[1000];

    /* SHOULDN'T WE MEMSET PATH HERE TO ENSURE IT IS O ELSEWHERE????????*/

    if ((p_dir = opendir(name)) == NULL) {
        perror("couldn't open directory");
        exit(3);
    }

    while ((p_dirent = readdir(p_dir)) != NULL) {

        /* Generate Relative Path Name and Perform Concatenation to Form Recursive Call on Subdirectory below*/
        char *str_path = p_dirent->d_name;

        if (strcmp(str_path, ".") != 0 && strcmp(str_path, "..") != 0) {
            strcpy(path, name);
            strcat(path, "/");
            strcat(path, str_path);

            if(is_dir(path) == 1) {
                /*Subdirectory Traversal*/
                traverse(path, png_flag);
            }
            else if(is_dir(path) == 0 && is_png(path) == 0) {
                /*Have Found a PNG and set png-found flag*/
                *png_flag = 1;
                printf("%s\n", path);
            }
        }
        else if (str_path == NULL) {
            fprintf(stderr,"Null pointer found!");
            exit(3);
        }
    }

    if ( closedir(p_dir) != 0 ) {
        perror("closedir");
        exit(3);
    }
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        fprintf(stderr, "Usage: %s <directory name>\n", argv[0]);
        exit(1);
    }
    int* png_flag = malloc(sizeof(int));
    *png_flag = 0;

    traverse(argv[1], png_flag);

    if (*png_flag == 0){
        printf("No PNG file found\n");
    }

    free(png_flag);
    return 0;
}