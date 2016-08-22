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

#define BUFLEN 65536
#define TIMEOUT 15

char *prog_name;

int main(int argc, char *argv[]) {

	char			buf[BUFLEN];
	uint16_t		port;
	int			s;
	int			n;
	unsigned int		addrlen;
	struct sockaddr_in	saddr, from;
	
	prog_name = argv[0];
	if (argc!=2) {
		err_quit("Usage: % <port>");
	}
	
	port = htons(atoi(argv[1]));
	s = Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	trace( err_msg("(%s) socket created", prog_name) );
	
	saddr.sin_family	= AF_INET;
	saddr.sin_port		= port;
	saddr.sin_addr.s_addr	= INADDR_ANY;
	Bind(s, (struct sockaddr *) &saddr, sizeof(saddr));
	trace( err_msg("(%s) listening for UDP packets on %s:%u", prog_name,saddr.sin_addr.s_addr,atoi(argv[1])) );
	
	for (;;){
		memset(buf, 0, BUFLEN);
		trace( err_msg("(%s) waiting for a packet ...",prog_name) );
		addrlen = sizeof(struct sockaddr_in);
		Recvfrom(s, buf, BUFLEN-1, 0, (struct sockaddr *)&from, &addrlen);
		n = strlen(buf);
		trace( err_msg("(%s) --- received string '%s' (payloads larger than %d bytes are truncated)",prog_name,buf,BUFLEN) );	
		Sendto(s, buf, n, 0, (struct sockaddr *)&from,addrlen);
	}
}
	
