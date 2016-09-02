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
#define CNUM   10
#define CIPLENGTH 100
#define TIMEOUT 15
#define TIME 3

char *prog_name;

int main(int argc, char *argv[]) {

	char			buf[BUFLEN], ip[CIPLENGTH];
	char                    clients[CNUM+1][CIPLENGTH];
	uint16_t		port;
	int			s;
	int			n, i, mark, length;
	int 			time[CNUM];
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
	trace( err_msg("(%s) listening for UDP packets on %s:%u", prog_name,inet_ntoa(saddr.sin_addr),atoi(argv[1])) );
	
	memset(clients, 0, sizeof(clients[0][0]) * CNUM * CIPLENGTH); 
	//sprintf(clients, "%s", clients);
	for (i=0; i<CNUM; i++) {
		clients[i][0] = '\0';
	}
	memset(time, 0, CNUM);
	
	for (;;){
		memset(buf, 0, BUFLEN);
		memset(ip, 0, CIPLENGTH);
		trace( err_msg("(%s) waiting for a packet ...",prog_name) );
		addrlen = sizeof(struct sockaddr_in);
		Recvfrom(s, buf, BUFLEN-1, 0, (struct sockaddr *)&from, &addrlen);
		 strcpy ( ip, inet_ntoa(from.sin_addr) );
		 length = sizeof(clients)/sizeof(clients[0]);
		for (i=0 ;i<length; i++) {
			if (strcmp(clients[i], ip)==0) {
				if (time[i]>=3)
					mark = 1;
				else
					mark = 2;
			break;
			}
			else 
				mark = 0;
		}
		
		if (mark == 0) {
		strcpy(clients[length%CNUM], ip);
		time[length%CNUM] = 1;
		trace( err_msg("(%s) --- received string '%s' from %s  %d times(payloads larger than %d bytes are truncated)",prog_name,buf,inet_ntoa(from.sin_addr),time[length%CNUM],BUFLEN) );	
		n = strlen(buf);
		Sendto(s, buf, n, 0, (struct sockaddr *)&from,addrlen);
		}
		else {
		if (mark == 1)
		trace( err_msg("(%s) --- Not possible to serve a client more than 3 time!", prog_name) );
		else {
			time[i] ++;
			trace( err_msg("(%s) --- received string '%s' from %s  %d times(payloads larger than %d bytes are truncated)",prog_name,buf,inet_ntoa(from.sin_addr),time[i],BUFLEN) );	
		n = strlen(buf);
		Sendto(s, buf, n, 0, (struct sockaddr *)&from,addrlen);
		}
		}
	}
}
	
