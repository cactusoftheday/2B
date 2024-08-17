#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT "2520"
#define PLANS_FILE "deathstarplans.dat"

typedef struct {
    char * data;
    int length;
} buffer;

extern int errno;

/* This function loads the file of the Death Star
   plans so that they can be transmitted to the
   awaiting Rebel Fleet. It takes no arguments, but
   returns a buffer structure with the data. It is the
   responsibility of the caller to deallocate the 
   data element inside that structure.
   */ 
buffer load_plans( );

int main( int argc, char** argv ) {

    if ( argc != 2 ) {
        printf( "Usage: %s IP-Address\n", argv[0] );
        return -1;
    }
    printf("Planning to connect to %s.\n", argv[1]);

    buffer buf = load_plans();

    struct addrinfo hints; 
    struct addrinfo *res; 

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(argv[1], PORT, &hints, &res);

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    int status = connect(sockfd, res->ai_addr, res->ai_addrlen);
    
    freeaddrinfo(res); //res no longer needed
    if (res == NULL) {
        printf("Failed to connect \n");
        return -1;
    }

    unsigned long totalSent = 0;
    unsigned int bytesSent;
    while (totalSent < buf.length) {
        bytesSent = send(sockfd, buf.data + totalSent, buf.length - totalSent, 0);
        if (bytesSent == -1) {
            printf("Failed to send to server. \n");
            close(sockfd);
            free(buf.data);
        }
        printf("We sent %d byte(s) \n", bytesSent);
        totalSent += bytesSent;
    }

    char response[65];
    int bytesReceived = recv(sockfd, response, 65, 0);
    if (bytesReceived == -1) {
        printf("Failed to receive response from server. \n");
        close(sockfd);
        free(buf.data);
        return -1;
    }

    response[bytesReceived] = '\0'; //end with null char
    printf("Response: %s\n", response);
    
    close(sockfd);
    free(buf.data);

    return 0;
}

buffer load_plans( ) {
    struct stat st;
    stat( PLANS_FILE, &st );
    ssize_t filesize = st.st_size;
    char* plansdata = malloc( filesize );
    int fd = open( PLANS_FILE, O_RDONLY );
    memset( plansdata, 0, filesize );
    read( fd, plansdata, filesize );
    close( fd );

    buffer buf;
    buf.data = plansdata;
    buf.length = filesize;

    return buf;
}
