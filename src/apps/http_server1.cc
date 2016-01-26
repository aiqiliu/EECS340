#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFSIZE 1024
#define FILENAMESIZE 100

using namespace std;

int handle_connection(int);
int writenbytes(int,char *,int);
int readnbytes(int,char *,int);

void error(int sock, char *msg) {
    perror(msg);
    minet_close(sock);
    exit(-1);
}

// MUST IMPLEMENT TEST CASES CORRECTLY, ex. 404 ERROR

int main(int argc,char *argv[])
{
  int server_port;
  int sock,sock2; 
  struct sockaddr_in sa,sa2; //
  int rc;

  /* parse command line args */
  if (argc != 3)
  {
    fprintf(stderr, "usage: http_server1 k|u port\n");
    exit(-1);
  }
  server_port = atoi(argv[2]);
  if (server_port < 1500)
  {
    fprintf(stderr,"INVALID PORT NUMBER: %d; can't be < 1500\n",server_port);
    exit(-1);
  }

  /* initialize minet */
  if (toupper(*(argv[1])) == 'K') {
    minet_init(MINET_KERNEL);
  } else if (toupper(*(argv[1])) == 'U') {
    minet_init(MINET_USER);
  } else {
    fprintf(stderr, "First argument must be k or u\n");
    exit(-1);
  }      

  /* initialize and make socket */
  if ((sock = minet_socket(SOCK_STREAM) == -1)) {
    perror("socket");
    exit(-1);
  }

  /* set server address*/
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port = htons(server_port);

  /* bind listening socket */
  if (minet_bind(sock, &sa) < 0) {
    perror("bind");
    exit(-1);
  }

  /* start listening */
  if (minet_listen(sock, 10) < 0) {
    perror("listen");
    exit(-1);
  }

  /* connection handling loop */
  while(1){
  /* handle connections */
    memset(&sa2, 0, sizeof(sa2));
    if ((sock2 = minet_accept(sock, &sa2)) < 0) //puts the connecting socket's file descriptor into sock2 and address into &sa2
    {
      error(sock, "Error accepting a connection\n");
    }
      rc = handle_connection(sock2);
    }
}

int handle_connection(int sock2)
{
  char filename[FILENAMESIZE+1];
  int rc;
  int fd;
  struct stat filestat;
  char buf[BUFSIZE+1];
  char *headers;
  char *endheaders;
  char *bptr;
  int datalen=0;
  char *ok_response_f = "HTTP/1.0 200 OK\r\n"\
                      "Content-type: text/plain\r\n"\
                      "Content-length: %d \r\n\r\n";
  char ok_response[100];
  char *notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"\
                         "Content-type: text/html\r\n\r\n"\
                         "<html><body bgColor=black text=white>\n"\
                         "<h2>404 FILE NOT FOUND</h2>\n"
                         "</body></html>\n";
  bool ok=true;

  /* first read loop -- get request and headers*/
  if(minet_read(sock2, buf, BUFSIZE) < 0)
  {
    error(sock2, "Failed to read request \n");
  }

  /* parse request to get file name */
  /* Assumption: this is a GET request and filename contains no spaces*/

  //MAY NEED TO CHECK GET, PATH FILE, AND HTTP VERSION ARE CORRECT
  
  int filenamelength = 0;
  char curr = buf[4]; //buf[4] is the start of the path right after GET 
  int currIndex = 4;
  while(curr != ' '){ //finds the character length of the path name
    currIndex++;
    curr = buf[currIndex];
    filenamelength++;
  }

  strncpy(&filename, &buf[4], filenamelength);
  if(filename == "")
  {
    printf("Must specify file name\n");
    ok = false;
  }

  /* try opening the file */
  char path[FILENAMESIZE + 1];
  memset(path, 0, FILENAMESIZE + 1);
  getcwd(path, FILENAMESIZE); //saves current working directy into path
  strncpy(path + strlen(path), filename, strlen(filename)); //combine the file directory with the filename
  
  char *filedata;
  
  if(stat(path, &filestat) < 0)//transfer info of path to filestat
  {
    ok = false;
    printf("Error opening file \n");
  } else {
    datalen = filestat.st_size;
    FILE *file = fopen(path, "r"); //read file at path
    filedata = (char *)malloc(datalen); 
    memset(filedata, 0, datalen);
    fread(filedata, 1, datalen, file); //read file into filedata
  }

  /* send response */
  if (ok) 
  {
    /* send headers */
    sprintf(ok_response, ok_response_f, datalen);
    if (writenbytes(sock2, ok_response, strlen(ok_response)) < 0) {
      error(sock2, "Failed to send response\n");
     }
    
     /* send file */
     if(writenbytes(sock2, filedata, datalen) < 0){
       error(sock2, "Can't send file");
      } else {
        minet_close(sock2);
        exit(0);
      }
  }  
  else { // send error response
    if(writenbytes(sock2, (char *)notok_response, strlen(notok_response)) < 0){
      error(sock2, "Can't send notok_response");
    } else {
      minet_close(sock2);
      exit(0);
    } 
  }
    
  // close socket and free space //
  minet_close(sock2);
  free(filedata);
  exit(-1);

  if (ok)
    return 0;
  else
    return -1;
}

int readnbytes(int fd,char *buf,int size)
{
  int rc = 0;
  int totalread = 0;
  while ((rc = minet_read(fd,buf+totalread,size-totalread)) > 0)
    totalread += rc;

  if (rc < 0)
  {
    return -1;
  }
  else
    return totalread;
}

int writenbytes(int fd,char *str,int size)
{
  int rc = 0;
  int totalwritten =0;
  while ((rc = minet_write(fd,str+totalwritten,size-totalwritten)) > 0)
    totalwritten += rc;

  if (rc < 0)
    return -1;
  else
    return totalwritten;
}

