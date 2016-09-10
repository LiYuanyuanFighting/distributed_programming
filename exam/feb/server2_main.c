/*
 *  TEMPLATE
 */
#include <stdio.h>
#include <stdlib.h>
#include    <string.h>
#include    <inttypes.h>
#include <unistd.h>

#include  <sys/wait.h>
#include <signal.h>

#include "../utils.h"
#include "../errlib.h"
#include "../sockwrap.h"
#define RBUFLEN	128
#define LISTENQ 15
char *prog_name;
int childNum;
int pidArray[10];

void sig_handlerP(int signo)
{
	
	int i, deadNum ,n;
	if (signo == SIGCHLD) {
		printf("parent received SIGCHLD\n");
	deadNum = 0;
		/*-------check alive child Number--------*/
		for (i = 0; i < childNum; i++) {
			n = waitpid(-1, NULL, WNOHANG);
			if (n>0)
			   deadNum ++;
		}
		childNum -= deadNum;
		
		}
}

int main (int argc, char *argv[])
{
	struct sockaddr_in servaddr,cliaddr;
        socklen_t cliaddrlen=sizeof(cliaddr);	
	int i, tag=0, port, listenfd, connfd, n;
	uint32_t hc,result;
	char buf[RBUFLEN+1];
	//check arguments
	if(argc!=3)
	{
	 printf("should be <port> <integer>");
	 exit(1);
	}
	prog_name = argv[0];
	port = atoi(argv[1]);
	int number = atoi(argv[2]);
	number = number * 1048576;//pow(2, number);
	//create the socket
	listenfd=socket(AF_INET,SOCK_STREAM,0);
    
	if(listenfd<0){
	printf("error in creating socket!");
	exit(1);	
	}
	//specify address to bind to
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(port);
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);

	int b=bind(listenfd,(SA*) &servaddr,sizeof(servaddr));
	if(b<0){
	printf("error in binding!\n");
	exit(1);
	}
	err_msg("(%s) socket created",prog_name);
	err_msg("(%s) listending on %s:%u",prog_name,inet_ntoa(servaddr.sin_addr),ntohs(servaddr.sin_port));
        int c=listen(listenfd,LISTENQ);
	if(c<0){
	printf("error in listening!\n");
	exit(1);
	}
	
	if (signal(SIGCHLD, sig_handlerP) == SIG_ERR) {
		err_msg ("(%s) - Error in catching SIGINT", prog_name);
		}
	
	for (;;)
	{
		tag = 0;
		err_msg ("(%s) waiting for connections ...", prog_name);
		if (childNum<3) {
		/* accept next connection */
		connfd = Accept (listenfd, (SA*) &cliaddr, &cliaddrlen);
		if ( !(Fork()>0) ) {	
		close (listenfd);
		err_msg("(%s) - %d new connection from client %s:%u", prog_name, getpid(), inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
		childNum ++;
		err_msg("(%s)new connection from client %s:%u",prog_name,inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));

		/* serve the client on socket s */
		for (i=0; i<number;)
		{
		memset(buf, 0 ,RBUFLEN+1);
		tag++;
		if((number-i)<=RBUFLEN) 
		    n=recv(connfd, buf, number-i, 0);   
		else
		    n=recv(connfd, buf, RBUFLEN, 0);
	            if (n < 0)
		    {
		       printf("Read error\n");
		       close(connfd);
		       printf("Socket %d closed\n", connfd);
		       break;
		    }
		    else if (n==0)
		    {
		       printf("Connection closed by party on socket %d\n",connfd);
		       close(connfd);
		       break;
		    }
	            else
		    {
		       i += n;
		       if (tag==1)
		       	hc = hashCode(buf, n, 1);
		       else
		       	hc = hashCode(buf, n, hc);
		        
		    }	
		}
		result = htonl(hc);
		printf("result is %u\n", hc);
		if(writen(connfd, &result, sizeof(uint32_t)) != sizeof(uint32_t))
	  	   printf("Write error while replying\n");
		else
		   printf("Reply sent\n");
		   close(connfd);
		   exit(0);
	  }
	  else 
	  	childNum ++;   
		   
	     } else {
		err_msg ("(%s) - the system is busy, prog_name!!!!!!!!!!!!!!!!");
		/*disable the SIGCHLD signal receipt before entering blocking wait()*/
		sigset_t signal_set;
		sigemptyset(&signal_set);
		sigaddset(&signal_set, SIGCHLD);//not SIGCHILD
		sigprocmask(SIG_BLOCK, &signal_set, NULL);
		wait(NULL);
		childNum--;
		sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
	}
	}
	
	return 0;
}
