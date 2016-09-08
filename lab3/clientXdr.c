#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <rpc/xdr.h>

#include <string.h>
#include <time.h>
#include <unistd.h>

#include "errlib.h"
#include "sockwrap.h"
#include "types.h"

#ifdef TRACE
#define trace(x) x
#else
#define trace(x)
#endif

#define BUFLEN 128
#define MAXBUFL 6526
#define TIMEOUT 10

char *prog_name;

int sendRequest(int s, char filename[BUFLEN]);
int checkReply(int s); 
int receiveSizeTime(int s); 
int receiveContent(int s, int size, char filename[BUFLEN]);

int sendRequestXdr(int s, char filename[BUFLEN], FILE *stream_socket);
int checkReplyXdr(int s);

int main(int argc, char *argv[]) {
	
	int			s;
	int 			result, size, time, n;
	struct sockaddr_in 	saddr;
	struct in_addr  	sIPaddr;
	short			port;
	char 			filename[BUFLEN];
	FILE 			*stream_socket_r, *stream_socket;
	
	prog_name = argv[0];
	if (argc!=4 || argv[3][0]!='-') {
		err_quit ("Usage: % <ip address> <port> <protocol>", prog_name);
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

	switch (argv[3][1]) {
	
	case 'a':
	do{
		result = sendRequest(s, filename);
		if (result==0) {
		  result = checkReply(s);
		  if (result==0){
			size = receiveSizeTime(s);	
			receiveContent(s, size, filename);
		}
		}
 	}while(result==0);
 	break;
 	
 	case 'x':
 	do{
 		
 		result = sendRequestXdr(s, filename, stream_socket);
 		if (result==0) {
		 //result = checkReplyXdr(s);
		 /*-------------------check reply------------------*/
		 XDR xdrs_r;
        	 message m;
         	stream_socket_r = fdopen(s,"r");
		
	
	
		printf("waiting for control message");
    		xdrstdio_create(&xdrs_r,stream_socket_r,XDR_DECODE);
		printf("I have created control stream");
    		memset(&m, 0, sizeof(message));
    		if(!xdr_message(&xdrs_r,&m))

    		{
    			printf("Error in receiving xdr quit messages!\n");
    			xdr_destroy(&xdrs_r);
    			//fclose(stream_socket_r);
    			result= -1;
    		}
    		
    		printf("has received control message\n");
    		int return_value, fd;
    		switch (m.tag){
    		case OK:
    			result =0;
    			printf("received ok\n!");
    			size = m.message_u.fdata.contents.contents_len;
    			time = m.message_u.fdata.last_mod_time;
    			err_msg("(%s) --- received file timestamp '%d'",prog_name,time);
    			err_msg("(%s) --- received file size '%d'",prog_name,size);
    			/*open the file*/
    			fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
    			if (fd == -1) {
    				printf("Error while opening the file!\n");
    				break;
    			}
    			n = write(fd, m.message_u.fdata.contents.contents_val, size);
    			if (n < size) {
    				err_msg("(%s) - Error while writing file", prog_name);
    				close(fd);
    				result = -1;
    			}
    			err_msg("(%s) - Received file correcly!", prog_name);
    			close(fd);
    			break;
    		case ERR:
    			printf("received error\n");
    			result =-1;
    			break;
    		
    		} 
		 /*------------------------------------------------*/
		xdr_destroy(&xdrs_r);
		}
	}while(result==0);
	//fclose(stream_socket_r);
	//fclose(stream_socket);
 		break;
 	default:
 		err_quit ("(%s) protocol - input error", prog_name);
 		break;
	}
	
	//fclose(stream_socket_r);
	//fclose(stream_socket);
	close(s);
	free(prog_name);
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
	
	Writen(s, buf, i+2);
		trace( err_msg("(%s) - request data has been sent", prog_name) );
		return 0;
	
}

int sendRequestXdr(int s, char filename[BUFLEN], FILE *stream_socket) {
	int 	result;
	XDR 	xdrs;
	message m;
	
	memset(&m, 0, sizeof(message));
	stream_socket = fdopen(s, "w");
	if (stream_socket == NULL)
		err_sys ("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&xdrs, stream_socket, XDR_ENCODE);
	
	memset(filename,0,BUFLEN);
	trace( err_msg("(%s) - Insert file names (no GET), one per line (end with EOF - press CTRL+D on keyboard to enter EOF):",prog_name) );
	int i = 0;
	while(1){
		if (scanf("%c",&filename[i])!=EOF) {
		//printf("typing %d",buf[i]);
		if (filename[i] == '\n'){
			//printf("typing .. %c",buf[i]);
			filename[i] = '\0';
			break;
		}
		i++;} else {
		//printf("1111typing %d",buf[i]);
		//Writen(s, "QUIT\r\n", 6);
		printf("I'm going to quit\n");
		m.tag = 2;
		if ( ! xdr_message(&xdrs, &m) ) {
			err_msg("(%s) - cannot send quit with XDR", prog_name);
		}
		fflush(stream_socket);
		xdr_destroy(&xdrs);
		//fclose(stream_socket);
		return -2;
		}
			
	}

	//Writen(s, buf, i+2);
	//send name to the server
	m.tag = 0; //GET
	m.message_u.filename = strdup(filename);
	if ( ! xdr_message(&xdrs, &m) ) {
			err_msg("(%s) - cannot send quit with XDR", prog_name);
		}
	fflush(stream_socket);
	xdr_destroy(&xdrs);
	//fclose(stream_socket);////can't close nowwwwww!
	trace( err_msg("(%s) - request data has been sent", prog_name) );
	return 0;
	
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
	//int r = getCommand(s, rbuf, 6);
	int r = readline_unbuffered(s, rbuf, BUFLEN);
	if (r>0) { 
		trace( err_msg("(%s) --- received string '%s'",prog_name,rbuf) );
		if (strcmp(rbuf, "-ERR\r\n")==0)
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
int checkReplyXdr(int s) {
	 XDR xdrs_r;
         message m_out;
         FILE *stream_socket_r = fdopen(s,"r");
	
	printf("waiting for control message");
    	xdrstdio_create(&xdrs_r,stream_socket_r,XDR_DECODE);
	printf("I have created control stream");
    		memset(&m_out,0,sizeof(message));
    		if(!xdr_message(&xdrs_r,&m_out))

    		{
    			printf("Error in receiving xdr quit messages!\n");
    			xdr_destroy(&xdrs_r);
    			fclose(stream_socket_r);
    			return -1;
    		}
    		
    		printf("has received control message\n");
    		int return_value;
    		switch (m_out.tag){
    		case OK:
    			return_value =0;
    			printf("received ok\n!");
    			break;
    		case ERR:
    			printf("received error\n");
    			return_value =-1;
    			break;
    		
    		} return return_value;
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

