#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>

#include <sys/types.h>
#include  <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>

#include <rpc/xdr.h>

#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "errlib.h"
#include "sockwrap.h"

//#define SRVPORT 1500

#define LISTENQ 15
#define MAXBUFL 255
#define TIMEOUT 120

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
int childNum, connfd;
int pidArray[10];

int checkRequest(int connfd,char filename[MAXBUFL]);
int sendSizeTime(int connfd, char filename[MAXBUFL]);
int sendContent(int connfd, char filename[MAXBUFL], int size);

void sig_handlerP(int signo)
{
	
	int i;
	if (signo == SIGINT) {
		printf("parent received SIGINT\n");
	for (i=0; i<childNum; i++) {
		kill(pidArray[i], SIGINT);
	}
	for (i=0; i<childNum; i++) {
		wait(NULL);
	}
		
		exit(0);
		}
}

/*void sig_handlerC(int signo)
{
	
	int i;
	if (signo == SIGINT) {
		printf("child %d received SIGINT\n", getpid());
		Close(connfd);
		exit(0);
		}
}*/

int main (int argc, char *argv[]) {

	int listenfd, err=0, n, i, pid=0;
	short port;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddrlen = sizeof(cliaddr);
	char filename[MAXBUFL];

	/* for errlib to know the program name */
	prog_name = argv[0];
	childNum = atoi(argv[2]);
	
	/* check arguments */
	if (argc!=3)
		err_quit("Usage: % <port> <child number>");
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

	i = 0;
	while(i<childNum) {
		pid=fork();
		pidArray[i] = pid;
		if(pid) {
			i++;
		} else {
			break;
		}
	}
	while (pid==0) {
		/*if (signal(SIGINT, sig_handlerC) == SIG_ERR) {
			trace( err_msg ("(%s) - %d error in catching SIGINT", prog_name, getpid()) );
		}*/
		trace( err_msg ("(%s) %d waiting for connections ...", prog_name, getpid()) );
		connfd = Accept (listenfd, (SA*) &cliaddr, &cliaddrlen);

		trace ( err_msg("(%s) - %d new connection from client %s:%u", prog_name, getpid(), inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port)) );
		
		while(1) {
		trace ( err_msg("(%s) - %d waiting for commands ...", prog_name, getpid()) );
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
				printf("DEBUG---%d\n",err);
				if (err <0) {
					Sendn (connfd, MSG_ERR, strlen(MSG_ERR), 0);			
					break;
				} else {
					if (err == 0)
					{
						err = sendSizeTime(connfd, filename);
						if (err <0) {
						//printf("erere");
						Sendn (connfd, MSG_ERR, strlen(MSG_ERR), 0);			
						break;
						} else {
						//printf("kokoko");
						sendContent(connfd, filename, err);
						}
				} else {
					break;
				}
			}
		}
		}
	}	

		Close (connfd);
		printf("err = %d---------------\n",err);
		trace( err_msg ("(%s) - %d connection closed by %s", prog_name, getpid(), (err>=0)?"client":"server") );
		
	}
	//parent waits for children 
	if(pid>0) {
		Close(listenfd);
		if (signal(SIGINT, sig_handlerP) == SIG_ERR) {
			trace( err_msg ("(%s) - Error in catching SIGINT", prog_name) );
		}
		   while(1) 
        sleep(1);
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
		return 1;
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
			if ( strcmp(buf, "QUIT\r\n")==0 || strcmp(buf, "ABORT\r\n")==0 ) {
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
		if(writen(connfd, content, size)!= size) {
		 trace( err_msg("(%s) error - writen() failed", prog_name));
			 fclose(fp);
			 return -1;
		}
	} else {
		while (size>MAXBUFL) {
			i = 0;
			while(i<MAXBUFL) {
				content[i++] = fgetc(fp);
			}
			if(writen(connfd, content, MAXBUFL)!= MAXBUFL) {
		 trace( err_msg("(%s) error - writen() failed", prog_name));
			 fclose(fp);
			 return -1;
		}
			size = size - MAXBUFL;
		}
		memset(content, 0, MAXBUFL);
		i = 0;
		while(i<size) {
				content[i++] = fgetc(fp);
			}
		if(writen(connfd, content, size)!= size) {
		 trace( err_msg("(%s) error - writen() failed", prog_name));
			 fclose(fp);
			 return -1;
		}
	}
	trace( err_msg("(%s) --- sent file '%s' to client", prog_name, filename) );	
	fclose(fp);
	
	
	return 0;
}

