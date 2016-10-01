/*--------------------tcp simple-----------------------*/
#include <stdio.h>
#include <stdlib.h>
#include    <string.h>
#include    <inttypes.h>
 #include <unistd.h>

#include "../utils.h"
#include "../errlib.h"
#include "../sockwrap.h"
#define MAXBUFL	128
#define TIMEOUT 20
#define MSG_ERR "-ERR\r\n"
#define MSG_SUC "+OK\r\n"
#define SA struct sockaddr
#define	LISTENQ		1024	/* 2nd argument to listen() */
#define PATH "/export/home/stud/s217403/Documents/dp/for_exam/exam/other_samples/exam_dp_sep2014/source"
char *prog_name;

int checkRequest(int connfd,char filename[MAXBUFL]);
int uploadContent(int connfd, char filename[MAXBUFL]);

int main (int argc, char *argv[])
{
	socklen_t		len, addrlen;
	int i, port, listenfd, connfd, n, err;
	uint32_t hc,result;
	struct sockaddr_storage	cliaddr;
	
	//check arguments
	if(argc!=2)
	{
	 printf("should be <port>");
	 exit(1);
	}
	prog_name = argv[0];
	char filename[MAXBUFL];
	//create the socket
	listenfd = Tcp_listen(NULL, argv[1], &addrlen);

	err_msg("(%s) socket created",prog_name);
	
	for (;;)
	{
		err_msg ("(%s) waiting for connections ...", prog_name);
		len = sizeof(cliaddr);
		/* accept next connection */
		connfd =Accept(listenfd, (SA *)&cliaddr, &len);
		printf("connection from %s\n", Sock_ntop((SA *)&cliaddr, len));
 
 		while(1) {
		err_msg("(%s) - waiting for commands ...", prog_name);
		memset(filename, 0, MAXBUFL);
		fd_set socket_set;
		struct timeval timeout;
		FD_ZERO(&socket_set);
		FD_SET(connfd, &socket_set);
		timeout.tv_sec = TIMEOUT;
		timeout.tv_usec = 0;
		n = select(FD_SETSIZE, &socket_set, NULL, NULL, &timeout);
		if (n==0) {
			err_msg("(%s) Timeout waiting for data from client: connection with client will be closed", prog_name);
			break;
		}
		else {
			if (n<0) {
				err_msg("(%s) Select failed, please send again!", prog_name);
			}
			else {
				err = checkRequest(connfd,filename);
				if (err <0) {
					if (writen(connfd, MSG_ERR, strlen(MSG_ERR))<strlen(MSG_ERR)) {
		  err_msg("(%s) error - writen() failed", prog_name);
		  err = -1;	  
	}
					break;
				} else {
					if (writen(connfd, MSG_SUC, strlen(MSG_SUC))<strlen(MSG_SUC)) {
		  err_msg("(%s) error - writen() failed", prog_name);
		  err = -1;
		  break;	  
	}								
					else {
						//printf("kokoko");
						uploadContent(connfd, filename);
						
				}
			}
		}
		}
	}	

		//Close (connfd);
		err_msg ("(%s) - connection closed by %s", prog_name, (err>0)?"client":"server");
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
		err_msg("(%s) Receive name failed.", prog_name);
		return -1;
	} else {
		char get[4];
		strncpy(get, buf, 3);
		get[3] = '\0';
		if (strcmp(get, "PUT")==0) {
			//get the name of the requested file
			for (i=3; buf[i]!='\r'; i++) {
				filename[i-3] = buf[i];
				//printf("%d ", buf[i]);
			}
			filename[i-3] = '\0';
			err_msg("(%s) received string '%s'", prog_name, buf);
			err_msg("(%s) client asked to send file '%s'", prog_name, filename);
			return 0;
		} else {
			if ( strcmp(buf, "QUIT\r\n")==0 ) {
				err_msg("(%s) client asked to terminate connection");
				return 1;
			}
			else
				return -1;
		}
	}
		
}


int uploadContent(int connfd, char filename[MAXBUFL]) {
	char content[MAXBUFL];
	int i=0 , n;
	int f=0;
	char full[MAXBUFL], curr[MAXBUFL];
	full[0] = '\0';
	
	memset(content, 0, MAXBUFL);
	/*printf("file name is %s PATH is %s\n", filename, PATH);
	getcwd(curr, sizeof(curr));
	printf("current working directory is %s\n", curr);
	sprintf(curr, "%s/%s", curr, filename);
	printf("file name is %s\n", curr);*/
	FILE *fp=fopen(filename,"wb");
   	if(NULL==fp){
      	printf("Error opening file");
      	exit(-1);
    	}
	
	while(1) {
	memset(content, 0, MAXBUFL);
	//n = recv(connfd, content, MAXBUFL, MSG_DONTWAIT);
	n = recv(connfd, content, MAXBUFL, 0);
	if (n==0) {
		break;
	}
	f = fwrite(content,1,n,fp);
	if (f<n) {
	printf("error\n");
	break;}
	i = i+f;
	}
	err_msg("(%s) --- received '%d' bytes, file '%s' written", prog_name, i, filename);	
	fclose(fp);
	return 0;
}
