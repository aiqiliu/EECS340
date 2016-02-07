#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <iostream>
#include <string>

#define BUFSIZE 81929

int write_n_bytes(int fd, char * buf, int count);

void error(int sock, char *msg) {
    perror(msg);
    minet_close(sock);
    exit(-1);
}

int main(int argc, char * argv[]) {
    char * server_name = NULL; // hostname
    int server_port = 0; // port number
    char * server_path = NULL;

    int sock = 0;
    int rc = -1;
    int datalen = 0;
    bool ok = true;
    struct sockaddr_in sa;
    FILE * wheretoprint = stdout;
    struct hostent * site = NULL; // structure containing info about the server
    char * req = NULL; 

    char buf[BUFSIZE + 1];
    char * bptr = NULL;
    char * bptr2 = NULL;
    char * endheaders = NULL;

    struct timeval timeout;
    fd_set set;

    /*parse args */
    if (argc != 5) {
        fprintf(stderr, "usage: http_client k|u server port path\n");
        exit(-1);
    }

    server_name = argv[2];
    server_port = atoi(argv[3]);
    server_path = argv[4];

    /* initialize minet */
    if (toupper(*(argv[1])) == 'K') {
        minet_init(MINET_KERNEL);
    } else if (toupper(*(argv[1])) == 'U') {
        minet_init(MINET_USER);
    } else {
        fprintf(stderr, "First argument must be k or u\n");
        exit(-1);
    }

    /* create socket */
    sock = minet_socket(SOCK_STREAM); //creates a socket with TCP protocol
    if (sock <0){ 
        error(sock, "Error opening the socket");
    }
    
    // Do DNS lookup
    /* Hint: use gethostbyname() */
    site = gethostbyname(server_name);
    if(site == NULL){
        error(sock, "ERROR, host does not exist\n");
    }
        
    /* set address */
    memset(&sa, 0, sizeof(sa)); //Sets the first "sizeof(sa)" bytes of thing pointed by sa to 0
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = * (unsigned long *)site ->h_addr_list[0];
    sa.sin_port = htons(server_port);
    
    /* connect socket */
    if (minet_connect(sock, &sa) != 0){ //connects sock to server
        error(sock, "Did not connect to socket\n");
    } 
    
    /* send request */
    req = (char *)malloc(strlen(server_path) + 15); 
    sprintf(req, "GET %s HTTP/1.0\n\n", server_path);
    if (write_n_bytes(sock, req, strlen(req)) < 0)  //sends the request to sock
    {
        free(req);
        error(sock, "Failed to write request\n");
    }
    free(req);
    
    
    /* wait till socket can be read */
    /* Hint: use select(), and ignore timeout for now. */
    FD_ZERO(&set); //zeroes out all of the file descriptors
    FD_SET(sock, &set); //sets and activates sock in the set of file descriptors
    
    int maxval = 0; 
    maxval = (maxval > sock) ? maxval : sock; //sets maxval to be the greatest integer of all sockets
    if (minet_select(maxval+1, &set, NULL, NULL, NULL) < 1) {
       error(sock, "Didn't select socket\n");
    }
    
    /* first read loop -- read headers */
    memset(buf, 0, BUFSIZE);
    int n = minet_read(sock, buf, BUFSIZE); //reading data from sock into buf
    if (n < 0){ 
        error(sock, "ERROR reading from socket");
    }

    /* examine return code */
    //Skip "HTTP/1.0"
    //remove the '\0'
    // Normal reply has return code 200

    bptr = buf;

    while (bptr[-1] != ' ') 
        bptr++;

    char num[4];
    strncpy(num, bptr, 3);
    num[4] = '\0';

    rc = atoi(num); //gets the return code

    if (rc != 200){
        wheretoprint = stderr;
        ok = false;
    }

    fprintf(wheretoprint, "Status: %d\n\n", rc); //prints the Status code

    //increment to the output after the status
    while (bptr[-1] != '\n'){
        bptr++;
    }    

    bptr = buf;
    int headerlen = 0;
    while (!(bptr[-2] == '\n' && bptr[0] == '\n')) {
      headerlen++;
      bptr++;
    }
    /* print first part of response */
    if (rc != 200){ //does not print header if rc is 200
      fprintf(wheretoprint, "%.*s", headerlen, buf); //prints to file datalen chars starting from bptr
    }

    /* second read loop -- print out the rest of the response */
    fprintf(wheretoprint, "%.*s", BUFSIZE-3, bptr); //prints to file BUFSIZE-3 chars starting from bptr2
    
    /*close socket and deinitialize */
    minet_close(sock);
    minet_deinit(); //deinitialize 

    if (ok) {
        return 0;
    } else {
        return -1;
    }
}
int write_n_bytes(int fd, char * buf, int count) {
    int rc = 0;
    int totalwritten = 0;

    while ((rc = minet_write(fd, buf + totalwritten, count - totalwritten)) > 0) {
        totalwritten += rc;
    }

    if (rc < 0) {
        return -1;
    } else {
        return totalwritten;
    }
}
