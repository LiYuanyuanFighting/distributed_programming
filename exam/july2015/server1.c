#include<unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "../errlib.h"
#include "../sockwrap.h"
#include "../utils.h"
#define BUFLEN 1024
#define MSG_ERR "wrong operands\r\n"
char *prog_name;

int
main(int argc, char* argv[])
{
if (argc!=3) {
	printf("Usage: port integer");
	exit(-1);
}
char buf[BUFLEN];
uint16_t port;
int s,result,recv_size, K;
uint32_t id, op1, op2;
uint32_t res;

struct sockaddr_in servaddr;

port=htons(atoi(argv[1]));
prog_name=argv[0];
K = atoi(argv[2]);
s=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
if(s<0){
  err_ret("(%s)-socket() failed",prog_name);
  //exit(1);
  }
servaddr.sin_family =PF_INET;
servaddr.sin_port   =port;
servaddr.sin_addr.s_addr =INADDR_ANY;
err_msg("(%s)-Binding to address %s\n",prog_name,inet_ntoa(servaddr.sin_addr));
result=bind(s,(struct sockaddr *) 
&servaddr,sizeof(servaddr));
if(result==-1)
     err_msg("(%s)-bind() failed",prog_name);

for(;;) 
{
 in_port_t port1;
     err_msg("(%s)-waiting for client data..\n",prog_name);
     struct sockaddr_storage peer_addr;
     socklen_t peer_addr_len;
     peer_addr_len = sizeof(struct sockaddr_storage); /* this value has to be re-initialized each time the Recvfrom is called, see man page */
     memset(buf, 0, BUFLEN);
     recv_size = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &peer_addr, &peer_addr_len); 
     //to check the family of peer_addr,then convert into ipv4 and ipv6 depends on real addresses are ipv6 or ipv4
     err_msg("(%s)-Received message size is:%lu ",prog_name,recv_size); 
     
     char text_addr[INET6_ADDRSTRLEN];
/*--------------------------------------ipv4---------------------------*/     
     if( ((struct sockaddr *)&peer_addr)->sa_family == AF_INET )
      {port1 = ((struct sockaddr_in *)&peer_addr)->sin_port;
//inet_ntop - convert IPv4 and IPv6 addresses from binary to text form    
     printf("zero%p\n",(struct sockaddr_in *)&peer_addr);
     Inet_ntop(AF_INET, &( ((struct sockaddr_in *)&peer_addr)->sin_addr), text_addr, sizeof(text_addr));    
	 printf("first%p\n",(struct sockaddr_in *)&peer_addr);}
/*---------------------------------------------------------------------*/

/*--------------------------------------ipv6---------------------------*/
     else
     {port1 = ((struct sockaddr_in6 *)&peer_addr)->sin6_port;
//inet_ntop - convert IPv4 and IPv6 addresses from binary to text form
     printf("zero%p\n",(struct sockaddr_in6 *)&peer_addr);
     Inet_ntop(AF_INET6, &( ((struct sockaddr_in6 *)&peer_addr)->sin6_addr), text_addr, sizeof(text_addr));    
     printf("first%p\n",(struct sockaddr_in6 *)&peer_addr);}
/*---------------------------------------------------------------------*/	 
	 
     if(recv_size>0)
     {
     /* get the operands and send MSG_ERR in case of error */
		if (sscanf(buf,"%u %u %u",&id, &op1,&op2) != 3) {
			err_msg("(%s) --- wrong or missing operands",prog_name);
			int len = strlen(MSG_ERR);
			int ret_send = sendto(s, MSG_ERR, len, MSG_NOSIGNAL,(struct sockaddr *)&peer_addr,peer_addr_len);
			if (ret_send!=len) {
				printf("cannot send MSG_ERR\n");
				err_msg("(%s) - cannot send MSG_ERR",prog_name);
				
			}
			continue;
		}
		op1 = ntohl(op1);
		op2 = ntohl(op2);
		err_msg("(%s) ---id %u operands %u %u",prog_name,id,op1,op2);

		/* do the operation */
		res = (op1 + op2)%K;
		memset(buf, 0, BUFLEN);
		buf[0] = '\0';
		res = htonl(res);
		sprintf(buf, "%u %u", id, res);
		int len = strlen(buf);
		int ret_send = sendto(s, buf, len, 0,(struct sockaddr *)&peer_addr,peer_addr_len);
		if (ret_send!=len) {
			err_msg("(%s) - cannot send RESULT, errror code is %d",prog_name, errno);
				
		}
		continue;
     }

}
 	
return 0;
}
