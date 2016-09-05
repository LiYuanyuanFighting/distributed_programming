#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <rpc/xdr.h>

#include <string.h>
#include <time.h>
#include <unistd.h>

#include "errlib.h"
#include "sockwrap.h"

#ifdef TRACE
#define trace(x) x
#else
#define trace(x)
#endif

#define BUFLEN 128
#define MAXBUFL 6526
#define TIMEOUT 10
#define stdin_fd 0
#define REPLY 1
#define SIZE_TIME 2
#define CONTENT 3
#define COUNT 10
#define MSG_ABORT "ABORT\r\n"
#define MSG_QUIT "QUIT\r\n"

char *prog_name;

int sendRequest(int s, int fileNum, char filename[COUNT][BUFLEN]);
int checkReply(int s); 
int receiveSizeTime(int s); 
int receiveContent(int s, int size, char filename[BUFLEN]);

int main(int argc, char *argv[]) {
	
	int			n, i, f, count=0, fw;
	int			s;
	int 			result, size, tag = 0, fileNum = 0, status = REPLY, current = 0;
	struct sockaddr_in 	saddr;
	struct in_addr  	sIPaddr;
	short			port;
	char 			filename[COUNT][BUFLEN];
	char 			content[MAXBUFL];
	
	prog_name = argv[0];
	if (argc!=3) {
		err_quit ("Usage: % <ip address> <port>", prog_name);
	}
	port = atoi(argv[2]);
	Inet_aton(argv[1], &sIPaddr);
	
	for (n=0;n<COUNT;n++) {
		memset(filename[n], 0, BUFLEN);
	}
	
	s = Socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
	
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	result = connect(s, (struct sockaddr *) &saddr, sizeof(saddr));
	if(result == -1)
		err_quit("connect() failed!", prog_name);
	trace ( err_msg("(%s) Connected!", prog_name) );
	
	while(1) {
		trace( err_msg("(%s) - Insert file names (no GET), one per line (end with EOF - press CTRL+D on keyboard to enter EOF):",prog_name) );
		/*-------------select------------*/
		fd_set socket_set;
		FD_ZERO(&socket_set);
		FD_SET(s, &socket_set);
		FD_SET(stdin_fd, &socket_set);
		struct timeval timeout;
		timeout.tv_sec = TIMEOUT;
		timeout.tv_usec = 0;
		//n = select(FD_SETSIZE, &socket_set, NULL, NULL, &timeout);
		n = select(FD_SETSIZE, &socket_set, NULL, NULL, NULL);
		printf("**************n=%d***********\n",n);
		if (n<0) {
		trace( err_msg("(%s) - select falied", prog_name) );
		return -1;
		} else { 
		if (n>0) {
		/*--------------check from stdin--------------*/
		int sd = FD_ISSET(stdin_fd, &socket_set);
		if (sd>0) {
		result = sendRequest(s, fileNum, filename);
		printf("--------filename[%d]=%s\n",fileNum, filename[fileNum]);
		if (result == -2) {
			break;
		}
		else if (result == -1) {
			printf("I want to quit after current file finishes---fileNum=%d-------\n", fileNum);
			if (count == 0) {
				break;
			}
			tag = 1;
		} else if (result == 0) {
			fileNum++;
			count++;
		}
		}  
		
		/*--------------check from server-------------*/
		f = FD_ISSET(s, &socket_set);
		printf("f = %d----------------\n",f);
		if (f>0) {
		printf("************status is:%d\n", status);
		switch (status) {
		case REPLY:
		  result = checkReply(s);
		  if (result<0) {
		  	close(s);
		  	exit(0);
		  	//fileNum--;
		  } else {
		  	status = SIZE_TIME;
		  }
		  break;
		case SIZE_TIME:
		  size = receiveSizeTime(s);
		  status = CONTENT;
		  printf("++++++filename[%d]=%s",current, filename[current]);
		  FILE *fp=fopen(filename[current],"wb");
		  if (NULL==fp) {
      		  printf("Error opening file");
      		  exit(-1);
    		  }
    		  i = size;
		  break;
		case CONTENT:
		  fw = 0;
		  memset(content, 0, MAXBUFL);
		  printf("Begin to read............\n");
		  if (size<=MAXBUFL) {
		  Readn(s, content, size);
		  printf("Read1............\n");
			/*if (n!=size) {
			  printf("read:%s\n",content);
				err_sys ("(%s) error - only read %d bytes, read() failed", prog_name, n);
			}*/
		  fw = fwrite(content,1,size,fp);
		  if (fw<size) 
		  printf("error\n");
		  size = 0;
		  } else {
			Readn(s, content, MAXBUFL);
			printf("receive %d byte data", MAXBUFL);
    			fw=fwrite(content,1,MAXBUFL,fp);
                        if(fw<MAXBUFL)
                        printf("error\n"); 
			size = size - MAXBUFL;
	     }
	if (size == 0) {
	trace( err_msg("(%s) --- received '%d' bytes, file '%s' written", prog_name, i, filename[current]) );	
	fclose(fp); 
	current++;
	count--;
	if (count==0 && tag == 1)  {
		trace(err_msg("(%s) --- finish all file transmission, terminate now."));
		Send(s, MSG_QUIT, strlen(MSG_QUIT), 0);
		close(s);
		exit(0);
	}
	status = REPLY;
	}
	break;
	
	default:
		trace(err_msg("(%s) -- Error in status!\n"));
		break; 
		 }
		}
		} 
		
		
	else {
		trace ( err_msg("(%s) Timeout waiting for data: connection with server will be closed", prog_name) );
			break;
		}
 	}
	}
	close(s);
	exit(0);
}

int sendRequest(int s, int fileNum, char filename[COUNT][BUFLEN]) {
	char	buf[BUFLEN];
	int 	result, i;
	memset(buf,0,BUFLEN);
	fgets(buf, BUFLEN, stdin);
	cleanString(buf);
	if (strcmp(buf, "Q")==0) {
		//Send(s, MSG_QUIT, strlen(MSG_QUIT), 0);//put in position after sending last file
		return -1;
	}
	else {
		if (strcmp(buf, "A")==0) {
			Send(s, MSG_ABORT, strlen(MSG_ABORT), 0);
			return -2;
		}
		else if (strncmp(buf, "GET ", 4)==0) {
			for (i=4; i<strlen(buf); i++)
				filename[fileNum][i-4]=buf[i];
			filename[fileNum][i-4]='\0';
			sprintf(buf, "%s\r\n", buf);
			Send(s, buf, strlen(buf), 0);
			trace( err_msg("(%s) - request data has been sent", prog_name) );
	return 0;
		} else {
			trace(err_msg("(%s) - Input error, redo it!"););
			return -3;
		}
	}
	
	
	
}

int checkReply(int s) {
	char	rbuf[BUFLEN];
	memset(rbuf,0,BUFLEN);
	/*fd_set socket_set;
	struct timeval timeout;
	
	 set connection timeout */
	/*FD_ZERO(&socket_set); 
	FD_SET(s, &socket_set);
	timeout.tv_sec = TIMEOUT;
	timeout.tv_usec = 0;
        int n;
	
	if( (n = Select(FD_SETSIZE, &socket_set, NULL, NULL, &timeout)) == -1)
	{
		printf("select() failed\n");
		return -1;
	}
	if(n > 0)
	{*/
	int r = getCommand(s, rbuf, 6);
	printf("get command %d",r);
	//int r = readline_unbuffered(s, rbuf, BUFLEN);
	if (r>0) { 
		trace( err_msg("(%s) --- received string '%s'",prog_name,rbuf) );
		if (strcmp(rbuf, "-ERR")==0)
		return -1;
		else
		return 0;
		} 	
	else {
		trace( err_msg("(%s) - connection closed by server",prog_name) );
		return -1;
	}
	//}
}

int receiveSizeTime(int s) {
	uint32_t time, size;
	Readn(s, &size, sizeof(uint32_t));
	Readn(s, &time, sizeof(uint32_t));
	time = ntohl(time);
	size = ntohl(size);
	trace( err_msg("(%s) --- received file size '%d'",prog_name,size) );
	trace( err_msg("(%s) --- received file timestamp '%d'",prog_name,time) );
	return size;
}

int receiveContent(int s, int size, char filename[MAXBUFL]) {
	char content[MAXBUFL];
	int i = size, n;
	int f=0;
	memset(content, 0, MAXBUFL);
	FILE *fp=fopen(filename,"wb");
   	if(NULL==fp){
      	printf("Error opening file");
      	exit(-1);
    	}
	if (size<=MAXBUFL) {
		Readn(s, content, size);
		f = fwrite(content,1,size,fp);
		if (f<size) 
		printf("error\n");
	} else {
		while (size>MAXBUFL) {
			n= readn(s, content, MAXBUFL);
			if (n!=MAXBUFL) {
				err_sys ("(%s) error - readline() failed", prog_name);
			}
			printf("receive %d byte data", n);
    			f=fwrite(content,1,n,fp);
                        if(f<n)
                        printf("error\n"); 
			size = size - MAXBUFL;
		}
		memset(content, 0, MAXBUFL);
		Readn(s, content, size);
		f=fwrite(content,1,size,fp);
		if (f<size) 
		printf("error\n");
	}
	trace( err_msg("(%s) --- received '%d' bytes, file '%s' written", prog_name, i, filename) );	
	fclose(fp);
	return 0;
}


