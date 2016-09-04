/*generate header file:rpcgen -h -o types.h types.xdr
rpcgen -c -o types.c types.xdr*/
#include<stdlib.h>
#include<inttypes.h>
	#include<stdio.h>								
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include "errlib.h"
#include<unistd.h>
#include<rpc/xdr.h>
#include "types.h"
#include<unistd.h>
#ifdef TRACE
#define trace(x) x
#else
#define trace(x) 
#endif

#define TIMEOUT 15
#define BUFLEN 512
char *prog_name;

int main(int argc, char *argv[]) {

	XDR			xdrs_in, xdrs_out;
	char			buf[BUFLEN];
	char			rbuf[BUFLEN];
	
	int			s;
	int			input1, input2;
	int 			len,n;
	xdrhypersequence        req, outseq[2];
	xdrhyper		res;
	FILE			*fstreamw, *fstreamr;
	
	prog_name = argv[0];
	if (argc!=3) {
		err_quit ("Usage: % <port> <ip address>", prog_name);
	}
		prog_name=argv[0];
	struct addrinfo hints;
	struct addrinfo *result,*rp;
	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family=AF_UNSPEC;/*allow ipv4 or ipv6*/
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_protocol=0;//Any p
	int g=getaddrinfo(argv[1],argv[2],&hints,&result);
	if(g!=0){
	printf("getaddrinfo wrongly\n");
	exit(1);
	}
	
	
	for(rp=result;rp!=NULL;rp=rp->ai_next){
	s=socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);
	if(s==-1)
	continue;
	if(connect(s,rp->ai_addr,rp->ai_addrlen)!=-1)
	break;  //success
	close(s);
	}
	if(rp==NULL)
	{//no address succeed
	trace(err_msg("(%s)---could not connect",prog_name));
	
	}
	freeaddrinfo(result); 
	
	
	while(1) {
		memset(buf,0,BUFLEN);
		memset(rbuf,0,BUFLEN);
		trace( err_msg("(%s) - input 2 integers divided by space",prog_name) );
		
		fgets(buf, BUFLEN, stdin);
		sscanf(buf, "%u %u", &input1, &input2);
		printf("input 1=%u, input2=%u\n", input1, input2);
		
		fstreamw = fdopen(s, "w");
		req.xdrhypersequence_len = 2;
        	req.xdrhypersequence_val = outseq;
		req.xdrhypersequence_val[0] = input1;
		req.xdrhypersequence_val[1] = input2;
	
		xdrstdio_create(&xdrs_out, fstreamw, XDR_ENCODE);
		if (!xdr_xdrhypersequence(&xdrs_out, &req)) {
			xdr_destroy(&xdrs_out);
			return -1;
		}
		fflush(fstreamw);
		xdr_destroy(&xdrs_out);
		
		fstreamr = fdopen(s, "r"); 
		xdrstdio_create(&xdrs_in, fstreamr, XDR_DECODE);
		if (!xdr_xdrhyper(&xdrs_in, &res)) {
			err_sys ("(%s) error - in decoding response", prog_name);
		} else {
			trace( err_msg("(%s) - Response:%u", prog_name, res) );
		}
		fflush(fstreamr);
		xdr_destroy(&xdrs_in);
		
		fclose(fstreamr);
		fclose(fstreamw);

	}
	
	close(s);
	exit(0);
}
