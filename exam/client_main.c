#include <stdio.h>
#include <stdlib.h>
#include    <string.h>
#include    <inttypes.h>
 #include <unistd.h>

#include "../utils.h"
#include "../errlib.h"
#include "../sockwrap.h"
#define MSG_ERR "-ERR\r\n"
#define MSG_SUC "+OK\r\n"
#define SA struct sockaddr
#define BUFLEN		1024
char *prog_name;

int sendRequest(int s, char *filename);
int checkReply(int s); 
int sendContent(int connfd, char *filename);

int
main(int argc, char* argv[])
{
	int			sockfd, n;
	socklen_t		len;
	struct sockaddr_storage	ss;
	
	if (argc != 4)
		err_quit("usage:  <hostname/IPaddress> <service/port#> <local file>");

	prog_name = argv[0];
	char filename[BUFLEN];
	filename[0] = '\0';
	
	sockfd = Tcp_connect(argv[1], argv[2]);

	len = sizeof(ss);
	Getpeername(sockfd, (SA *)&ss, &len);
	printf("connected to %s\n", Sock_ntop_host((SA *)&ss, len));
	
	strcpy(filename, argv[3]);
	
	sendRequest(sockfd, filename);
	if (checkReply(sockfd)<0)
	printf("Error in format\n");
	else {
	sendContent(sockfd, filename);
	}
	
	return 0;
}

int sendRequest(int s, char *filename) {
	char	buf[BUFLEN];
	memset(buf,0,BUFLEN);
	buf[0] = '\0';
	
	sprintf(buf, "PUT%s\r\n",filename);
	
	Writen(s, buf, strlen(buf));
		err_msg("(%s) - request data has been sent", prog_name);
		return 0;
	
}

int checkReply(int s) {
	char	rbuf[BUFLEN];
	memset(rbuf,0,BUFLEN);
	int r = readline_unbuffered(s, rbuf, BUFLEN);
	if (r>0) { 
		err_msg("(%s) --- received string '%s'",prog_name,rbuf);
		if (strcmp(rbuf, "-ERR\r\n")==0)
		return -1;
		else
		if (strcmp(rbuf, "+OK\r\n")==0)
		return 0;
		else
		return -1;
		} 	
	else {
		err_msg("(%s) - connection closed by server",prog_name);
		return -1;
	}
	//}
}

int sendContent(int connfd, char *filename) {
	FILE *fp = fopen(filename, "rb"); 
	if(fp == NULL) {
		printf("failed to open the file\n");
		perror("");
		return -1;
	}
	char content[BUFLEN];
	int i=0;
	memset(content, 0, BUFLEN);
	//while(content[i]!=EOF) {
	//while(!feof(fp)) {
	while((content[i++] = fgetc(fp))!=EOF) {
		//content[i++] = fgetc(fp); 		
		if (i>BUFLEN) {
		Writen(connfd, content, i-1);
		i=0;} 
	}
	Writen(connfd, content, i-1);
	 
	err_msg("(%s) --- sent file '%s' size '%d' to client", prog_name, filename, i-1);	
	fclose(fp);
	return 0;
}
