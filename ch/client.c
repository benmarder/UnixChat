#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "game2.h"
#define PORT "9034" 		// the port client will be connecting to
#define MAXDATASIZE 1024 	// max number of bytes we can get at once
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
int main(int argc, char *argv[]){
    int sockfd, numbytes;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    char buf[MAXDATASIZE];
    char choose_game[MAXDATASIZE];
    char send_moves[MAXDATASIZE];
    int myTurn;
    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo); 		// all done with this structure
    
    while(1){				// main while
        if ((numbytes = recv(sockfd, buf, sizeof(char)*MAXDATASIZE, 0)) == -1) {
            perror("recv");
            exit(1);
        }
        
        choose_game[0]='1';            //these are the protocols for a msg between client-server.
        choose_game[1]='\0';           //a number is attached to the begining of the msg  
        send_moves[0]='2';             //'1' is to indicate that the player is choosing a game type
        send_moves[1]='\0';            //'2' is to indicate that the player sends a move
        switch (buf[0]) {
            case UNBORN:
		printf("wellcome to the game, please select the type of game:\n(1)play online\n(2)play vs computer\n");
            	while(1){
                	scanf("%s",buf);
                	if((buf[0] != '1') && (buf[0] != '2'))
                		printf("wrong input! try again\n");
                	else break;
            	}
            	strcat(choose_game,buf);
            	if(send(sockfd,choose_game,sizeof(choose_game),0)==-1)
            		perror("send");
            	break;
            
            case GAME_STARTED:
		printf("wellcome to the game, you are mached with a player! you are black! please wait for your turn\n");
            	myTurn=BLACK_TURN;
            	break;
            case WAITING_FOR_PLAYER2:
            	printf("waiting for another player, you are white!\n");
            	myTurn=WHITE_TURN;
            	break;
            
            case WHITE_TURN:
            case BLACK_TURN:
		printf("opponent move:%s\n",buf+1);
            	printf("your turn! please enter a move\n");
            	scanf("%s",buf);
            	printf("your move :%s was sent! please wait for your turn\n",buf);
            	strcat(send_moves,buf);
            	if(send(sockfd,send_moves,sizeof(send_moves),0)==-1)
            		perror("send");
            	break;
            case WHITE_WIN:
		if(myTurn==WHITE_TURN)
            		printf("you win!!\n");
            	else
            		printf("you lose!!\n");
            	close(sockfd);
            	exit(1);
            case BLACK_WIN:
	        if(myTurn==BLACK_TURN)
            		printf("you win!!\n");
            	else
            		printf("you lose!!\n");
            	close(sockfd);
            	exit(1);
            case DRAW:
		printf("the game ends with a draw\n");
            	break;
            default:
		printf("connection was lost\n");
		close(sockfd);
		return;

            
        }
    }
    close(sockfd);
    return 0;
}
