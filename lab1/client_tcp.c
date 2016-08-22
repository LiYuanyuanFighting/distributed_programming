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

int main(int argc, char *argv[]) {

	char			buf[BUFLEN];
	char			rbuf[BUFLEN];
	
	int			s;
	int 			result,i;
	struct sockaddr_in 	saddr;
	struct in_addr  	sIPaddr;
	short			port;
	
	prog_name = argv[0];
	if (argc!=3) {
		err_quit ("Usage: % <port> <ip address>", prog_name);
	}
	port = atoi(argv[1]);
	Inet_aton(argv[2], &sIPaddr);
	
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
		memset(buf,0,BUFLEN);
		memset(rbuf,0,BUFLEN);
		trace( err_msg("(%s) - type 2 operands divided by space",prog_name) );
		i = 0;
		while(1){
			scanf("%c",&buf[i]);
			printf("typing %d",buf[i]);
			if (buf[i] == '\n'){
				printf("typing .. %c",buf[i]);
				break;
			}
			i++;
		}
		buf[i] = '\0';
		sprintf(buf, "%s\r\n",buf);
		
		result = write(s, buf, i+2);
		if ( result != i+2 ) {
			err_sys ("(%s) error - write failed",prog_name);
		}
		printf("hah");
		Readline(s, rbuf, BUFLEN);
		trace( err_msg("(%s) - result is %s",prog_name,rbuf) ); 
	}
	
	close(s);
	exit(0);
}
