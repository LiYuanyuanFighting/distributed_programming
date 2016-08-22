/* Utility function to display a string str
   followed by an internet address a, written
   in decimal notation
*/
void showAddr(char *str, struct sockaddr_in *a)
{
    char *p;
    
    p = inet_ntoa(a->sin_addr);
	printf("%s %s",str,p);
	printf("!%u\n", ntohs(a->sin_port));
}
