/*
 *  warning: this is just a sample server to permit testing the clients; it is not as optimized or robust as it should be
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>

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
#include "types.h"
//#define SRVPORT 1500

#define LISTENQ 15
#define MAXBUFL 255

#define MSG_ERR "wrong operands\r\n"
#define MSG_OVF "overflow\r\n"

#define MAX_UINT16T 0xffff
//#define STATE_OK 0x00
//#define STATE_V  0x01

#ifdef TRACE
#define trace(x) x
#else
#define trace(x)
#endif

char *prog_name;



int prot_x (int connfd)
{
	XDR xdrs_r, xdrs_w;
	//char buf[MAXBUFL];
	//int nread;
	xdrhypersequence req;
	xdrhyper op1;
	xdrhyper op2;
	xdrhyper res;

	//xdrmem_create(xdrs, buf, MAXBUFL, XDR_DECODE);
	FILE *stream_socket_r = fdopen(connfd, "r");
	if (stream_socket_r == NULL)
		err_sys ("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&xdrs_r, stream_socket_r, XDR_DECODE);

	FILE *stream_socket_w = fdopen(connfd, "w");
	if (stream_socket_w == NULL)
		err_sys ("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&xdrs_w, stream_socket_w, XDR_ENCODE);

	trace( err_msg("(%s) - waiting for operands ...", prog_name) );
	
	/* decode request */
	req.xdrhypersequence_len = 2;
	req.xdrhypersequence_val = (xdrhyper *)malloc(2*sizeof(xdrhyper));
	/* get the operands */
	if ( ! xdr_xdrhypersequence(&xdrs_r, &req) ) {
		trace( err_msg("(%s) - cannot read op1 with XDR", prog_name) );		
		xdr_destroy(&xdrs_r);
	} 
	
	fflush(stream_socket_r);

	op1 = req.xdrhypersequence_val[0];
	op2 = req.xdrhypersequence_val[1];
	/* do the operation */
	res = op1 + op2;

	xdr_destroy(&xdrs_r);

	trace( err_msg("(%s) --- result of the sum: %d", prog_name, res) );

	/* send the result */
	if( !xdr_xdrhyper(&xdrs_w, &res) ) {
		trace( err_msg("(%s) - cannot send with XDR", prog_name) );
	}
	fflush(stream_socket_w);

	xdr_destroy(&xdrs_w);
	fclose(stream_socket_w);

	/* NB: Close read streams only after writing operations have also been done */
	fclose(stream_socket_r);


	trace( err_msg("(%s) --- result just sent back", prog_name) );

	return 0;
}


int main (int argc, char *argv[]) {

	int listenfd, connfd, err=0;
	short port;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddrlen = sizeof(cliaddr);

	/* for errlib to know the program name */
	prog_name = argv[0];

	/* check arguments */
	if (argc!=2)
		err_quit ("usage: %s <port>", prog_name);
	port=atoi(argv[1]);

	/* create socket */
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	/* specify address to bind to */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	Bind(listenfd, (SA*) &servaddr, sizeof(servaddr));

	trace ( err_msg("(%s) socket created",prog_name) );
	trace ( err_msg("(%s) listening on %s:%u", prog_name, inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port)) );

	Listen(listenfd, LISTENQ);

	while (1) {
		trace( err_msg ("(%s) waiting for connections ...", prog_name) );

		int retry = 0;
		do {
			connfd = accept (listenfd, (SA*) &cliaddr, &cliaddrlen);
			if (connfd<0) {
				if (INTERRUPTED_BY_SIGNAL ||
				    errno == EPROTO || errno == ECONNABORTED ||
				    errno == EMFILE || errno == ENFILE ||
				    errno == ENOBUFS || errno == ENOMEM	) {
					retry = 1;
					err_ret ("(%s) error - accept() failed", prog_name);
				} else {
					err_ret ("(%s) error - accept() failed", prog_name);
					return 1;
				}
			} else {
				trace ( err_msg("(%s) - new connection from client %s:%u", prog_name, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port)) );
				retry = 0;
			}
		} while (retry);

		err = prot_x(connfd);
		
			
		//Close (connfd);
		trace( err_msg ("(%s) - connection closed by %s", prog_name, (err==0)?"client":"server") );
	}
	return 0;
}

