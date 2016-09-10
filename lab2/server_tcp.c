/*
 *  warning: this is just a sample server to permit testing the clients; it is not as optimized or robust as it should be
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>

#include <rpc/xdr.h>

#include <string.h>
#include <time.h>
#include <unistd.h>

#include "errlib.h"
#include "sockwrap.h"

//#define SRVPORT 1500

#define LISTENQ 15
#define MAXBUFL 255
#define TIMEOUT 20

#define MSG_ERR "-ERR\r\n"
#define MSG_SUC "+OK\r\n"

#define MAX_UINT16T 0xffff
//#define STATE_OK 0x00
//#define STATE_V  0x01

#ifdef TRACE
#define trace(x) x
#else
#define trace(x)
#endif

char *prog_name;

int checkRequest(int connfd,char filename[MAXBUFL]);
int sendSizeTime(int connfd, char filename[MAXBUFL]);
int sendContent(int connfd, char filename[MAXBUFL], int size);

int main (int argc, char *argv[]) {

	int listenfd, connfd, err=0, n;
	short port;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddrlen = sizeof(cliaddr);
	char filename[MAXBUFL];

	/* for errlib to know the program name */
	prog_name = argv[0];
	
	/* check arguments */
	if (argc!=2)
		err_quit("Usage: % <port>");
	port=atoi(argv[1]);

	/* create socket */
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	/* specify address to bind to */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	Bind(listenfd, (SA*) &servaddr, sizeof(servaddr));

	trace ( err_msg("(%s) socket created",prog_name) );
	trace ( err_msg("(%s) listening on %s:%u", prog_name, inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port)) );

	Listen(listenfd, LISTENQ);

	while (1) {
		trace( err_msg ("(%s) waiting for connections ...", prog_name) );

		
		connfd = Accept (listenfd, (SA*) &cliaddr, &cliaddrlen);
		
		trace ( err_msg("(%s) - new connection from client %s:%u", prog_name, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port)) );
		
		while(1) {
		trace ( err_msg("(%s) - waiting for commands ...", prog_name) );
		memset(filename, 0, MAXBUFL);
		fd_set socket_set;
		struct timeval timeout;
		FD_ZERO(&socket_set);
		FD_SET(connfd, &socket_set);
		timeout.tv_sec = TIMEOUT;
		timeout.tv_usec = 0;
		n = select(FD_SETSIZE, &socket_set, NULL, NULL, &timeout);
		if (n==0) {
			trace ( err_msg("(%s) Timeout waiting for data from client: connection with client will be closed", prog_name) );
			break;
		}
		else {
			if (n<0) {
				trace ( err_msg("(%s) Select failed, please send again!", prog_name) );
			}
			else {
				err = checkRequest(connfd,filename);
				if (err <0) {
					Sendn (connfd, MSG_ERR, strlen(MSG_ERR), 0);			
					break;
				} else {
					if (err > 0)											 break;				
					else {
						err = sendSizeTime(connfd, filename);
						if (err <0) {
						//printf("erere");
						Sendn (connfd, MSG_ERR, strlen(MSG_ERR), 0);			
						break;
						} else {
						//printf("kokoko");
						sendContent(connfd, filename, err);
						}
				}
			}
		}
		}
	}	

		//Close (connfd);
		trace( err_msg ("(%s) - connection closed by %s", prog_name, (err>0)?"client":"server") );
		close(connfd);
	}
	return 0;
}

int checkRequest(int connfd,char filename[MAXBUFL]) {
	char buf[MAXBUFL];
	int i;
	memset(buf, 0, MAXBUFL);
	int n = readline_unbuffered (connfd, buf, MAXBUFL);
	if (n<0) {
		trace( err_msg("(%s) Receive name failed.", prog_name) );
		return -1;
	} else {
		char get[5];
		strncpy(get, buf, 4);
		get[4] = '\0';
		if (strcmp(get, "GET ")==0) {
			//get the name of the requested file
			for (i=4; buf[i]!='\r'; i++) {
				filename[i-4] = buf[i];
				//printf("%d ", buf[i]);
			}
			filename[i-4] = '\0';
			trace( err_msg("(%s) received string '%s'", prog_name, buf) );
			trace( err_msg("(%s) client asked to send file '%s'", prog_name, filename) );
			return 0;
		} else {
			if ( strcmp(buf, "QUIT\r\n")==0 ) {
				trace( err_msg("(%s) client asked to terminate connection") );
				return 1;
			}
			else
				return -1;
		}
	}
		
}

int sendSizeTime(int connfd, char filename[MAXBUFL]) {
	struct stat st;
	uint32_t size,time;
	int r = stat(filename, &st);
	if (r<0) {
		trace( err_msg("(%s) --- cannot stat() file: No such file or directory", prog_name) );
		return -1;
	} else {
		//Writen (connfd, MSG_SUC, strlen(MSG_SUC));
		if (writen(connfd, MSG_SUC, strlen(MSG_SUC))<strlen(MSG_SUC)) {
		  trace (err_msg("(%s) error - writen() failed", prog_name));
		  return -1;	  
	}
		else	
		  trace( err_msg("(%s) --- sent '+OK\r\n' to client", prog_name) );
		size = htonl(st.st_size);
		time = htonl(st.st_mtime);
		Sendn (connfd, &size, sizeof(size), 0);
		trace( err_msg("(%s) --- sent file size '%lu' - converted in network byte order to client", prog_name, st.st_size) );
		Sendn (connfd, &time, sizeof(time), 0);
		trace( err_msg("(%s) --- sent file timestamp '%lu' - converted in network byte order to client", prog_name, st.st_mtime) );
		return st.st_size;
	}
}

int sendContent(int connfd, char filename[MAXBUFL], int size) {
	FILE *fp = fopen(filename, "rb"); 
	char content[MAXBUFL];
	int i=0;
	memset(content, 0, MAXBUFL);
	if (size<=MAXBUFL) {
		while(i<size) {
			content[i++] = fgetc(fp); 		
		}
		Writen(connfd, content, i);
	} else {
		while (size>MAXBUFL) {
			i = 0;
			while(i<MAXBUFL) {
				content[i++] = fgetc(fp);
			}
			if (writen(connfd, content, i) != i)
			 trace( err_msg("(%s) error - writen() failed", prog_name));
			size = size - MAXBUFL;
		}
		memset(content, 0, MAXBUFL);
		i = 0;
		while(i<size) {
				content[i++] = fgetc(fp);
			}
		Writen(connfd, content, i);
	}
	trace( err_msg("(%s) --- sent file '%s' to client", prog_name, filename) );	
	fclose(fp);
	return 0;
}
