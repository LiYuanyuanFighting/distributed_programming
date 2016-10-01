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
int s,result,recv_size, K;
uint32_t id, op1, op2;
uint32_t res;
socklen_t		len;

struct sockaddr_storage	cliaddr;

prog_name=argv[0];
K = atoi(argv[2]);
s=Udp_server(NULL, argv[1], NULL);
/*---------------------create process pool------------------------*/
int i = 0, pid;
while(i<5) {
	pid=fork();
	if(pid) {
		i++;
	} else {
		break;
		}
	}
if (pid==0) 
{
for(;;) 
{
     err_msg("(%s)-waiting for client data..\n",prog_name);
     len = sizeof(cliaddr);
     memset(buf, 0, BUFLEN);
     recv_size = Recvfrom(s, buf, BUFLEN, 0, (SA *)&cliaddr, &len); 
     printf("datagram from %s\n", Sock_ntop((SA *)&cliaddr, len));
     //to check the family of peer_addr,then convert into ipv4 and ipv6 depends on real addresses are ipv6 or ipv4
     err_msg("(%s)-Received message size is:%lu ",prog_name,recv_size); 
   
     if(recv_size>0)
     {
     /* get the operands and send MSG_ERR in case of error */
		if (sscanf(buf,"%u %u %u",&id, &op1,&op2) != 3) {
			err_msg("(%s) --- wrong or missing operands",prog_name);
			int len = strlen(MSG_ERR);
			int ret_send = sendto(s, MSG_ERR, len, MSG_NOSIGNAL,(SA *)&cliaddr, len);
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
		int ret_send = sendto(s, buf, len, 0,(SA *)&cliaddr, len);
		if (ret_send!=len) {
			err_msg("(%s) - cannot send RESULT, errror code is %d",prog_name, errno);
				
		}
		continue;
     }

}
 }//parent waits for children 
if(pid>0) {
for (i=0; i<5; i++) {
 wait(NULL);
}
}	
return 0;
}
