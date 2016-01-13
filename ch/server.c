#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "game2.h"
#define PORT "9034" // port we're listening on
#define GAMES 100 //size of game array
#define MAXDATASIZE 1024

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int main(void){
    fd_set master; 		// master file descriptor list
    fd_set read_fds; 		// temp file descriptor list for select()
    int fdmax; 			// maximum file descriptor number
    int listener; 		// listening socket descriptor
    int newfd; 			// newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    char buf[MAXDATASIZE]; 	// buffer for client data
    int nbytes;
    char remoteIP[INET6_ADDRSTRLEN];
    int yes=1; 			// for setsockopt() SO_REUSEADDR, below
    int i, j, rv ,x;
    struct addrinfo hints, *ai, *p;
    FD_ZERO(&master); 		// clear the master and temp sets
    FD_ZERO(&read_fds);
    Game* game;
    init();  			//init games
    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        	continue;
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;
    }
    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
    freeaddrinfo(ai); // all done with this
    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    // add the listener to the master set
    FD_SET(listener, &master);
    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one
    // main loop
    printf("server is running\n");
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &read_fds)){ // we got one!!
                if (i == listener){
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,(struct sockaddr *)&remoteaddr,&addrlen);
                    if (newfd == -1){
                        perror("accept");
                    }
                    else{
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) { // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on ""socket %d\n",inet_ntop(remoteaddr.ss_family,
                        get_in_addr((struct sockaddr*)&remoteaddr),remoteIP, INET6_ADDRSTRLEN),newfd);
			buf[0]=UNBORN;
                        if (send(newfd,buf,sizeof(buf), 0) == -1)
                        	perror("send");
                    }
                }
                else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof(char)*MAXDATASIZE, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closedBeej's Guide to Network Programming 40
                            printf("selectserver: socket %d hung up\n", i);
                            game = findGame(i);
                            if(game==NULL)
                            	perror("findGame");
                            } else {
                            perror("recv");
                        }
                        close(game->player1); 		// bye!
                        close(game->player2); 		// bye!
                        FD_CLR(game->player1, &master); // remove from master set
                        FD_CLR(game->player2, &master); // remove from master set
			close_game(game);
                        }else {
                        // we got some data from a client
                        switch (buf[0]){
                            case '1':          	  	// players decision for game type
                            if(buf[1] == '1'){ 		//player chose to play online
                                create_regular_game(i); 
                            }
                            else{   			//player chose to play vs cpu
                                gnugame(i);
                            }
                            break;
                            case '2':            	//player sent a move
                            handle_move_from_player(i,buf+1);    //"+1" : skip the protocol parameter
                            
                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    return 0;
}
