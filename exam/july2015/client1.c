#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../errlib.h"
#include "../sockwrap.h"
#include "../utils.h"
#define TIMEOUT 3
#define BUFLEN 1024
char *prog_name;

int
main(int argc, char* argv[])
{	
	char			buf[BUFLEN], rbuf[BUFLEN], content[BUFLEN];
	
	int			s, fd;
	int 			len;
	int 			exit_code;
	struct sockaddr_in 	saddr;//, caddr;
	struct in_addr  	sIPaddr;
	short			port;
	
	struct sockaddr_in 	from;
	unsigned int		fromlen;//, namelen;
	fd_set			cset;
	struct	timeval		tval;
	uint32_t		id, op1, op2, res, res_id;
	
	prog_name = argv[0];
	if (argc!=6) {
		err_quit ("Usage: % <ip address> <port> <id> <op1> <op2>", prog_name);
	}
	
	port = atoi(argv[2]);
	Inet_aton(argv[1], &sIPaddr);
	id = htonl(atoi(argv[3]));
	op1 = htonl(atoi(argv[4]));
	op2 = htonl(atoi(argv[5]));
	
	s = Socket(AF_INET,SOCK_DGRAM, IPPROTO_UDP);
	
	err_msg("(%s) socket created",prog_name);
	
	err_msg("(%s) destination %s:%s",prog_name,argv[2],argv[1]);
	/* prepare client address structure 
	caddr.sin_family = AF_INET;
	caddr.sin_port   = htons(0);
	caddr.sin_addr.s_addr = htonl(INADDR_ANY);*/
	
	/* bind */
	/*Bind(s, (struct sockaddr *)&caddr, sizeof(caddr));
	namelen = sizeof(caddr);
	getsockname(s, (struct sockaddr *) &caddr, &namelen);
	trace( err_msg("(%s) -done. Bound to addr: ", &caddr) );*/
	
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//send msg
	memset(buf, 0 , BUFLEN);
	buf[0] = '\0';
	sprintf(buf, "%u %u %u", id, op1, op2);
	len = strlen(buf);
	memset(rbuf,0,BUFLEN);
	Sendto(s, buf, len, 0, (struct sockaddr *) &saddr, sizeof(saddr));
	err_msg("(%s) - data has been sent",prog_name);
	
	//waiting for response
	FD_ZERO(&cset);
	FD_SET(s, &cset);
	tval.tv_sec = TIMEOUT;
	tval.tv_usec = 0;
	//Select(FD_SETSIZE, &cset, NULL, NULL, &tval);
	int n;
	n = select(FD_SETSIZE, &cset, NULL, NULL, &tval);
            
	if (n == -1)
	err_quit("select() failed");
	if (n>0)
        {
            time_t t;
            printf("%d\n",(unsigned)time(&t));
	
	fromlen = sizeof(struct sockaddr_in);
	Recvfrom(s,rbuf,BUFLEN-1,0,(struct sockaddr *)&from,&fromlen);
	err_msg("(%s) --- received string '%s'",prog_name,rbuf);
	printf("%s",rbuf);
	memset(content, 0, BUFLEN);
	content[0]='\0';
	if (sscanf(rbuf, "%u %u", &res_id,&res) != 2) {
		exit_code = 1;
		sprintf(content, "%d", exit_code);
	} else {
	  if (id != res_id) {
		exit_code = 1;
		sprintf(content, "%d", exit_code);
		}
	  else {
	  	exit_code = 0;
	  	res = ntohl(res);
	  	sprintf(content, "%d %u", exit_code, res);
	  	}
		}
	}
	else {printf("No response received after %d seconds\n",TIMEOUT);
	exit_code = 2;
	sprintf(content, "%d %u", exit_code, res);
	}	
	printf("content is %s", content);
	//write to the file
	/*open the file*/
    	fd = open("output.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
    	if (fd == -1) {
    		printf("Error while opening the file!\n");
    		}
    	else {
    	n = write(fd, content, strlen(content));
    	if (n < strlen(content)) {
    		err_msg("(%s) - Error while writing file", prog_name);
    		close(fd);
    		}
    	}
    	close(fd);
	close(s);
	exit(0);
	
	return 0;
}
