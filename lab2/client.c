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

char *prog_name;

int sendRequest(int s, char filename[BUFLEN]);
int checkReply(int s); 
int receiveSizeTime(int s); 
int receiveContent(int s, int size, char filename[BUFLEN]);

int main(int argc, char *argv[]) {
	
	int			s;
	int 			result, size;
	struct sockaddr_in 	saddr;
	struct in_addr  	sIPaddr;
	short			port;
	char 			filename[BUFLEN];
	
	prog_name = argv[0];
	if (argc!=3) {
		err_quit ("Usage: % <ip address> <port>", prog_name);
	}
	port = atoi(argv[2]);
	Inet_aton(argv[1], &sIPaddr);
	
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
		result = sendRequest(s, filename);
		if (result<0) {
			break;
		} else {
		  result = checkReply(s);
		  if (result<0) {
			break;
		} else {
			size = receiveSizeTime(s);	
			receiveContent(s, size, filename);
		}
		}
 	}
	
	close(s);
	exit(0);
}

int sendRequest(int s, char filename[BUFLEN]) {
	char	buf[BUFLEN];
	int 	result;
	memset(buf,0,BUFLEN);
	trace( err_msg("(%s) - Insert file names (no GET), one per line (end with EOF - press CTRL+D on keyboard to enter EOF):",prog_name) );
	int i = 4;
	buf[0] = '\0';
	strcpy(buf, "GET ");
	while(1){
		if (scanf("%c",&buf[i])!=EOF) {
		filename[i-4] = buf[i];
		//printf("typing %d",buf[i]);
		if (buf[i] == '\n'){
			//printf("typing .. %c",buf[i]);
			filename[i-4] = '\0';
			break;
		}
		i++;} else {
		//printf("1111typing %d",buf[i]);
		Writen(s, "QUIT\r\n", 6);
		return -2;
		}
			
	}
	buf[i] = '\0';
	sprintf(buf, "%s\r\n",buf);
	
	result = write(s, buf, i+2);
	if ( result != i+2 ) {
		trace ( err_msg("(%s) error - write failed",prog_name) );
		return -1;
	} else {
		trace( err_msg("(%s) - request data has been sent", prog_name) );
		return 0;
	}
}

int checkReply(int s) {
	char	rbuf[BUFLEN];
	memset(rbuf,0,BUFLEN);
	int r = readline_unbuffered(s, rbuf, BUFLEN);
	if (r>0) { 
		trace( err_msg("(%s) --- received string '%s'",prog_name,rbuf) );
		if (strcmp(rbuf, "-ERR\r\n")==0)
		return -1;
		else
		return 0;
		} 	
	else {
		trace( err_msg("(%s) --- error in receiving string",prog_name) );
		return -1;
	}
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

int receiveContent(int s, int size, char filename[BUFLEN]) {
	char content[BUFLEN];
	int i = size;
	memset(content, 0, BUFLEN);
	if (size<=BUFLEN) {
		Readn(s, content, size);
	} else {
		while (size>BUFLEN) {
			Readn(s, content, BUFLEN);
			size = size - BUFLEN;
		}
		memset(content, 0, BUFLEN);
		Readn(s, content, size);
	}
	trace( err_msg("(%s) --- received '%d' bytes, file '%s' written", prog_name, i, filename) );	
	return 0;
}
