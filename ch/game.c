#include "game2.h"
/*gobal*/ 
Game gameArray[GAMES];
Game* gamePtr;
int playerNumber;	//keep track on the connections


int check_if_game_ends(Game* game){
	if(game->historyLine > 5){
		char buf[1];
		buf[0] = (game->status = (rand()%3));		//random select how the game ends (0-2 are the ending status)
		if (send(game->player1,buf,sizeof(buf), 0) == -1)
                        	perror("send");
		if (send(game->player2,buf,sizeof(buf), 0) == -1)
                        	perror("send");
		return TRUE;
	}
	return FALSE;
}
void init_game(Game* game){
	game->player1=0;
	game->player2=0;	
	game->status = UNBORN;
	game->historyLine=0;
	memset(game->history, 0, sizeof(char)*HISTORY_SIZE*BUF_SIZE);
}
void init(){	
	playerNumber=0;
	int i;		
	for(i=0;i<GAMES;++i){
		init_game(&(gameArray[i]));
	}	
}

int isAvailable(Game* game){
	if(game->status != UNBORN)
		return FALSE;
	return TRUE;
}
Game* find_Game_With_One_Player(){
	int i;		
	for(i=0;i<GAMES;++i){
		if(gameArray[i].status == WAITING_FOR_PLAYER2)
			return &gameArray[i];
	}
	return NULL;
}

Game* findAvailableGame(){
	int i;
	for(i=0;i<GAMES;++i){
	if(isAvailable(&gameArray[i]))
		return &(gameArray[i]);
	}
	return NULL;
}
Game* findGame(int playerNumber){	
	int i;
	for(i=0;i<GAMES;++i){
		if(isAvailable(&gameArray[i]) == FALSE){
			if(gameArray[i].player1 == playerNumber || gameArray[i].player2 == playerNumber)
				return &(gameArray[i]);
		}
	}
	return NULL;
}
/*write moves to history*/
void write_move(int player,char* buf){
	gamePtr = findGame(player);
	strcpy(gamePtr->history[(gamePtr->historyLine++)],buf);
}
/*builds the foundations for a gnu game */
Game* create_gnu_game(int Stdin,int Stdout,int player,pid_t* p_id){
	gamePtr = findAvailableGame();
	gamePtr->pid=*p_id;
	gamePtr->is_gnuGame=1;
	gamePtr->gnu_stdin=Stdin;
	gamePtr->gnu_stdout=Stdout;	
	gamePtr->status=WHITE_TURN;
	gamePtr->player1=player;
	return gamePtr;	
}
/*function that builds the foundations for a game between 2 players on the web*/
void create_regular_game(int fd){
	char buf[BUF_SIZE];
	playerNumber++;
	if(playerNumber%2 != 0){	//player 1 is joining
		gamePtr = findAvailableGame();
		if(gamePtr == NULL)
			perror("no free games");
		gamePtr->player1=fd;
		gamePtr->status = WAITING_FOR_PLAYER2;
	}
	else{			//player 2 is joining
		gamePtr = find_Game_With_One_Player();
		if(gamePtr == NULL)
			perror("findGame");
		gamePtr->player2=fd;
		gamePtr->status=GAME_STARTED;
		buf[0]=gamePtr->status;
		buf[1]=0;
                if (send(gamePtr->player2,buf,sizeof(Status), 0) == -1)
                	perror("send");
                gamePtr->status=WHITE_TURN;						
	}	
	buf[0]=gamePtr->status;
	buf[1]=0;
        if(send(gamePtr->player1,buf,sizeof(Status), 0) == -1)
        	perror("send");
}

void close_game(Game* game){
	if(game->is_gnuGame)
		 kill(game->pid, 1); 			//send signo signal to the child process	
	init_game(game);	
}
/*function that deals with the moves from players (check where to send the move) */
void handle_move_from_player(int i, char* buf){
	Game* game=findGame(i);
	if(check_if_game_ends(game) == FALSE){	
		int numbyte;
		char buffer[BUF_SIZE];
		write_move(i,buf);	
		if(game->is_gnuGame){
			strcat(buf,"\n");
			//write to stdin
			write(game->gnu_stdin,buf,strlen(buf));
			//read from stdout
			sleep(15);
 	  		numbyte=read(game->gnu_stdout,buffer, sizeof(buffer));
			buf[numbyte]='\0'; 
			write_move(i,buffer);
			buf[0]=game->status;
			strcpy(buf+1,buffer);
			if(send(game->player1,buf, sizeof(char)*BUF_SIZE, 0) == -1)
                        	perror("send");		
		}
		else{
			buffer[0]=game->status;
			strcpy(buffer+1,buf);
                        if(game->player1 == i){
                        	game->status=BLACK_TURN;
                                if (send(game->player2,buffer, sizeof(char)*BUF_SIZE, 0) == -1)
                                	perror("send");
                        }
                        else{
                                game->status=WHITE_TURN;
                                if (send(game->player1,buffer,sizeof(char)*BUF_SIZE, 0) == -1)
                                	perror("send");
                        }
		}
	}
}
/*function that creates a child proccess and gets rud of the first two messages from gnu chess */
void gnugame(int player){
    char buffer[1024];
    Game* game;
    pid_t pid = 0;
    int rpipes[2];
    int wpipes[2];
    char buf[256];
    int numbyte;
    pipe(rpipes);
    pipe(wpipes);
    pid = fork();
    if (pid == 0)
    {
        // Child
        close(wpipes[1]);
        close(rpipes[0]);
        dup2(wpipes[0], STDIN_FILENO);
        dup2(rpipes[1], STDOUT_FILENO);
        close(wpipes[0]);
        close(rpipes[1]);
        execl("/usr/games/gnuchess", "gnuchess", (char*) NULL);
        exit(1);
    }
    //parent
    close(wpipes[0]);
    close(rpipes[1]);
    game=create_gnu_game(wpipes[1],rpipes[0],player,&pid);
    numbyte= read(rpipes[0],buffer, sizeof(buffer));
    buffer[numbyte]='\0';
    memset(&buffer, 0, sizeof buffer);
    read(rpipes[0],buffer, sizeof(buffer));
    buf[0]=game->status;
    if (send(game->player1,buf,sizeof(Status), 0) == -1)
   	 perror("send");
    
}

