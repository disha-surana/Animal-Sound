#include<stdio.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<ctype.h>   
#include<iostream>
#include<algorithm> 

#define SERV_TCP_PORT 5035
#define MAXLINE 4096

void str_cli(FILE *fp, int sockfd)
{
    int         maxfdp1, stdineof;
    fd_set      rset;
    char        buf[MAXLINE],recv[MAXLINE];
    int     	n;

    stdineof = 0;
    FD_ZERO(&rset);

    for ( ; ; ) {

    	bzero(recv,sizeof(recv));
    	bzero(buf,sizeof(buf));
    	
        if (stdineof == 0)
            FD_SET(fileno(fp), &rset);
        
        FD_SET(sockfd, &rset);
        maxfdp1=std::max(sockfd,0)+1;
        select(maxfdp1, &rset, NULL, NULL, NULL);

        //Socket readable
        if (FD_ISSET(sockfd, &rset)) {  
            if ( (n = read(sockfd, recv, MAXLINE)) == 0) {
                if (stdineof == 1)
                    return;     
                else{
                    printf("\nstr_cli: server terminated prematurely\n");
					return;
				}
            }
			recv[n]='\0';
			write(1,recv,strlen(recv));
        }

        //Input readable
        if (FD_ISSET(fileno(fp), &rset)) {  
            if ( (n = read(fileno(fp), buf, MAXLINE)) == 0) {
                stdineof = 1;
                shutdown(sockfd, SHUT_WR);  //cannot write anymore to socket
                FD_CLR(fileno(fp), &rset);
            }
			else if(strcasecmp(buf,"end\n")==0 || strcasecmp(buf,"bye\n")==0){
                stdineof = 1;
                write(sockfd, buf, strlen(buf)); //last write 'bye' needed for server's bye ok reply
                shutdown(sockfd, SHUT_WR);  //cannot write anymore to socket
                FD_CLR(fileno(fp), &rset);
            }
			else
                write(sockfd, buf, strlen(buf));
        }
    }
}

int main(int argc, char **argv){

    int sockfd;
	struct sockaddr_in serv_addr;
	
	if (argc != 2)
	       perror("usage: a.out <IPaddress>");
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	       perror("socket error");
	
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_TCP_PORT);
   	if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0)
             perror("inet_pton error for %s");
             
    write(1,"\nEnter username:",16);
             
	if(connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1){
		perror("Error");
		return 1;
	}

	str_cli(stdin,sockfd);
	
	return 0;
}
