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
#define TIMEOUT 15

char *prog_name;

int main(int argc, char *argv[]) {

	char			rbuf[BUFLEN];
	
	int			s;
	int 			len;
	struct sockaddr_in 	saddr, caddr;
	struct in_addr  	sIPaddr;
	short			port;
	
	struct sockaddr_in 	from;
	unsigned int		fromlen, namelen;
	fd_set			cset;
	struct	timeval		tval;
	
	prog_name = argv[0];
	if (argc!=4) {
		err_quit ("Usage: % <ip address> <port> <message>", prog_name);
	}
	
	port = atoi(argv[2]);
	Inet_aton(argv[1], &sIPaddr);
	
	s = Socket(AF_INET,SOCK_DGRAM, IPPROTO_UDP);
	
	trace( err_msg("(%s) socket created",prog_name) );
	
	trace( err_msg("(%s) destination %s:%s",prog_name,argv[2],argv[1]) );
	/* prepare client address structure */
	caddr.sin_family = AF_INET;
	caddr.sin_port   = htons(0);
	caddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
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
	len = strlen(argv[3]);
	memset(rbuf,0,BUFLEN);
	Sendto(s, argv[3], len, 0, (struct sockaddr *) &saddr, sizeof(saddr));
	trace( err_msg("(%s) - data has been sent",prog_name) );
	
	//waiting for response
	FD_ZERO(&cset);
	FD_SET(s, &cset);
	tval.tv_sec = TIMEOUT;
	tval.tv_usec = 0;
	Select(FD_SETSIZE, &cset, NULL, NULL, &tval);
	
	Recvfrom(s,rbuf,BUFLEN-1,0,(struct sockaddr *)&from,&fromlen);
	trace( err_msg("(%s) --- received string '%s'",prog_name,rbuf) );
	printf("%s",rbuf);
	
	close(s);
	exit(0);
	
}
