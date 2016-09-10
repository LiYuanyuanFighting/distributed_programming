/*
 *  TEMPLATE
 */
#include <stdio.h>
#include <stdlib.h>
#include    <string.h>
#include    <inttypes.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../utils.h"
#include "../errlib.h"
#include "../sockwrap.h"
#define RBUFLEN	1048576
#define BUFLEN 128

char *prog_name;

int main (int argc, char *argv[])
{

	if (argc!=5) {
		err_quit ("Usage: % ./prog_name <server_addr> <port_no> <how_many_megaBytes> <file_name>", prog_name);
	}
	prog_name=argv[0];
	char *filename = argv[4];
	int ntime = atoi(argv[3]);
	int s,sfd;
	//ssize_t nread;
	struct addrinfo hints;
	struct addrinfo *result,*rp;
	char *ptr,*rbuf;
	ptr=malloc(63*sizeof(ptr));
	rbuf=malloc(BUFLEN*sizeof(rbuf));
	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_protocol=IPPROTO_TCP;
	hints.ai_flags=0;
	s=getaddrinfo(argv[1],argv[2],&hints,&result);
	if(s!=0)
	{
    		printf("getaddrinfo wrongly\n");
    		exit(1);
	}
	for(rp=result;rp!=NULL;rp=rp->ai_next){
    		sfd=socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);

   		 if(sfd==-1)
    			continue;

    		if(connect(sfd,rp->ai_addr,rp->ai_addrlen)!=-1)
    			break;

   	 	close(sfd);}

    	if(rp==NULL){
    		err_msg("(%s)---could not connect",prog_name);
    		exit(1);
	}
    	freeaddrinfo(result);

	//fill buffer
	char buf[RBUFLEN];
	memset(buf, 0, RBUFLEN);
	int fp, code, nread, k;
	uint32_t hc;
	fp = open(filename, O_RDONLY);
	if (fp<0) {
		err_msg("Cannot open file %s\n", filename);
		code = 1;
	} else {
		nread = read(fp, buf, RBUFLEN);
		if (nread<RBUFLEN) {
			printf("Reading failed\n");
			code = 1;
		} else {
			code = 0;
		}
		close(fp);
	}
	for (k=0; k<ntime; k++) {
	  	Writen(sfd, buf, RBUFLEN);
	}	
	//receive result
	Readn(sfd, &hc, sizeof(uint32_t));
	hc = ntohl(hc);
	printf("I received %u\n", hc);
	Close(sfd);
	return code;
}
