/*
	Documentation of the mkdir() function:

	NAME
	mkdir - make a directory

	SYNOPSIS
	#include <sys/stat.h>

	int mkdir(const char *path, mode_t mode);

	DESCRIPTION
	The mkdir() function shall create a new directory with name path. The file permission bits of the new directory shall be initialized from mode. These file permission bits of the mode argument shall be modified by the process' file creation mask.
	When bits in mode other than the file permission bits are set, the meaning of these additional bits is implementation-defined.
	The directory's user ID shall be set to the process' effective user ID. The directory's group ID shall be set to the group ID of the parent directory or to the effective group ID of the process. Implementations shall provide a way to initialize the directory's group ID to the group ID of the parent directory. Implementations may, but need not, provide an implementation-defined way to initialize the directory's group ID to the effective group ID of the calling process.
	The newly created directory shall be an empty directory.
	If path names a symbolic link, mkdir() shall fail and set errno to [EEXIST].
	Upon successful completion, mkdir() shall mark for update the st_atime, st_ctime, and st_mtime fields of the directory. Also, the st_ctime and st_mtime fields of the directory that contains the new entry shall be marked for update.

	RETURN VALUE
	Upon successful completion, mkdir() shall return 0. Otherwise, -1 shall be returned, no directory shall be created, and errno shall be set to indicate the error.

	EXAMPLE
	mkdir ("/home/test/myFolder", 0777);
 */
/*--------------------tcp simple-----------------------*/
#include <stdio.h>
#include <stdlib.h>
#include    <string.h>
#include    <inttypes.h>
 #include <unistd.h>

#include "../utils.h"
#include "../errlib.h"
#include "../sockwrap.h"
#define MAXBUFL	128
#define TIMEOUT 20
#define MSG_ERR "-ERR\r\n"
#define MSG_SUC "+OK\r\n"
#define SA struct sockaddr
#define	LISTENQ		1024	/* 2nd argument to listen() */
#define PATH "/home/stud/s217403/Documents/dp/for_exam/exam/other_samples/exam_dp_sep2014"
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
int checkRequest(int connfd,char filename[MAXBUFL]);
int uploadContent(int connfd, char filename[MAXBUFL], struct sockaddr_storage	cliaddr;);

int main (int argc, char *argv[])
{
	socklen_t		len, addrlen;
	int i, port, listenfd, connfd, n, err;
	uint32_t hc,result;
	struct sockaddr_storage	cliaddr;
	
	//check arguments
	if(argc!=2)
	{
	 printf("should be <port>");
	 exit(1);
	}
	prog_name = argv[0];
	char filename[MAXBUFL];
	//create the socket
	listenfd = Tcp_listen(NULL, argv[1], &addrlen);

	err_msg("(%s) socket created",prog_name);
	if (signal(SIGCHLD, sig_handlerP) == SIG_ERR) {
		err_msg ("(%s) - Error in catching SIGINT", prog_name);
		}
	for (;;)
	{
		err_msg ("(%s) waiting for connections ...", prog_name);
		len = sizeof(cliaddr);
		/* accept next connection */
		if (childNum<3) {
		connfd =Accept(listenfd, (SA *)&cliaddr, &len);
		if ( !(Fork()>0) ) {	
		close (listenfd);
		childNum ++;
		printf("connection from %s\n", Sock_ntop((SA *)&cliaddr, len));
 
 		while(1) {
		err_msg("(%s) - waiting for commands ...", prog_name);
		memset(filename, 0, MAXBUFL);
		fd_set socket_set;
		struct timeval timeout;
		FD_ZERO(&socket_set);
		FD_SET(connfd, &socket_set);
		timeout.tv_sec = TIMEOUT;
		timeout.tv_usec = 0;
		n = select(FD_SETSIZE, &socket_set, NULL, NULL, &timeout);
		if (n==0) {
			err_msg("(%s) Timeout waiting for data from client: connection with client will be closed", prog_name);
			break;
		}
		else {
			if (n<0) {
				err_msg("(%s) Select failed, please send again!", prog_name);
			}
			else {
				err = checkRequest(connfd,filename);
				if (err <0) {
					if (writen(connfd, MSG_ERR, strlen(MSG_ERR))<strlen(MSG_ERR)) {
		  err_msg("(%s) error - writen() failed", prog_name);
		  err = -1;	  
	}
					break;
				} else {
					if (writen(connfd, MSG_SUC, strlen(MSG_SUC))<strlen(MSG_SUC)) {
		  err_msg("(%s) error - writen() failed", prog_name);
		  err = -1;
		  break;	  
	}								
					else {
						
						uploadContent(connfd, filename, cliaddr);
						
				}
			}
		}
		}
	}	

		//Close (connfd);
		err_msg ("(%s) - connection closed by %s", prog_name, (err>0)?"client":"server");
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
 		

int checkRequest(int connfd,char filename[MAXBUFL]) {
	char buf[MAXBUFL];
	int i;
	memset(buf, 0, MAXBUFL);
	int n = readline_unbuffered (connfd, buf, MAXBUFL);
	if (n<0) {
		err_msg("(%s) Receive name failed.", prog_name);
		return -1;
	} else {
		char get[4];
		strncpy(get, buf, 3);
		get[3] = '\0';
		if (strcmp(get, "PUT")==0) {
			//get the name of the requested file
			for (i=3; buf[i]!='\r'; i++) {
				filename[i-3] = buf[i];
				//printf("%d ", buf[i]);
			}
			filename[i-3] = '\0';
			err_msg("(%s) received string '%s'", prog_name, buf);
			err_msg("(%s) client asked to send file '%s'", prog_name, filename);
			return 0;
		} else {
			if ( strcmp(buf, "QUIT\r\n")==0 ) {
				err_msg("(%s) client asked to terminate connection");
				return 1;
			}
			else
				return -1;
		}
	}
		
}



int uploadContent(int connfd, char filename[MAXBUFL], struct sockaddr_storage	cliaddr) {
	char content[MAXBUFL];
	int i=0 , n;
	int f=0;
	memset(content, 0, MAXBUFL);
	char path[MAXBUFL];
	char *port, *ip;
	char *delim = ":";
	
	printf("%s\n", Sock_ntop((SA *)&cliaddr, sizeof(cliaddr)));
	ip = strtok(Sock_ntop((SA *)&cliaddr, sizeof(cliaddr)), delim);
	
	
	port = strtok(NULL, delim);
        printf("%s    %s\n", ip, port);
        printf("kokoko1\n");
	sprintf(path, "%s/%s_%s", PATH, ip, port);
	printf("%s\n", path);
	mkdir (path, 0777);
	sprintf(path, "%s/%s", path, filename);
	FILE *fp=fopen(path,"wb");
   	if(NULL==fp){
      	printf("Error opening file\n");
      	exit(-1);
    	}
	
	i=0;
	while(1) {
	memset(content, 0, MAXBUFL);
	//n = recv(connfd, content, MAXBUFL, MSG_DONTWAIT);
	n = recv(connfd, content, MAXBUFL, 0);
	if (n==0) {
		break;
	}
	f = fwrite(content,1,n,fp);
	if (f<n) {
	printf("error\n");
	break;}
	i = i+f;
	}
	err_msg("(%s) --- received '%d' bytes, file '%s' written", prog_name, i, filename);	
	fclose(fp);
	return 0;
}
