#ifndef GAME_H
#define GAME_H
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define HISTORY_SIZE 100 
#define BUF_SIZE 1024
#define GAMES 100
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif
typedef enum Stat{WHITE_WIN,BLACK_WIN,DRAW,WHITE_TURN,BLACK_TURN,WAITING_FOR_PLAYER2,GAME_STARTED,UNBORN}Status; 
typedef struct game{
	int player1;
	int player2;
	short int is_gnuGame;
	int gnu_stdin;
	int gnu_stdout;
	Status status;
	char history[HISTORY_SIZE][BUF_SIZE];
	int historyLine;
	pid_t pid; 
}Game;

int check_if_game_ends(Game* game);
void init();
Game* find_Game_With_One_Player();
Game* findAvailableGame();
Game* findGame(int playerNumber);
void write_move(int player,char* buf);
Game* create_gnu_game(int Stdin,int Stdout,int player,pid_t* p_id);
void create_regular_game(int fd);
void close_game(Game* game);
void handle_move_from_player(int i, char* buf);



#endif
