#ifndef S_AND_L_H
#define S_AND_L_H

#include "raylib.h"

// --- Constants and Global Settings ---
#define SIZE 10
#define MAX_PLAYERS 6
#define N_SNAKES 5
#define N_LADDERS 5
#define SAVE_FILE_NAME "s_and_l_save.txt"
#define PALETTE_SIZE 6 // Define the explicit size of the color array

// --- Game State Enums and Structs ---

typedef enum {
    STATE_LOAD_OR_NEW = 0,
    STATE_INPUT_PLAYERS,
    STATE_ENTERING_NAMES,
    STATE_GAMEPLAY,
    STATE_GAME_OVER
} GameState;

typedef struct {
    char name[20];
    int id;
    int position;
    Color color;
} Player;

typedef struct {
    int mouth;
    int tail;
} Snake;

typedef struct {
    int top;
    int bottom;
} Ladder;

// For GUI coordinates
typedef struct { int x,y; } Vec2i;


// --- External Variables (defined in game.c) ---

extern char inputBuffers[MAX_PLAYERS][20];
extern int currentTyping;

// Rolling State Variables (Defined in game.c)
extern int rolling;
extern float rollingTimer;
extern float rollingDuration; 

// Animation Variables (Defined in game.c)
extern int animating;
extern Player *animPlayer;
extern int animPath[200];
extern int animPathLen;
extern int animStepIndex;
extern float animProgress;
extern float animDuration;
extern Color palette[PALETTE_SIZE];

// --- Function Prototypes (Defined in game.c) ---

void snakes_and_ladders(Snake snakes[], Ladder ladders[]);
int roll(void);
int CheckSaveFileExists(const char *fileName);
void DeleteSaveFile(const char *fileName);
void InitNewGame(Player players[], int np, Snake snakes[], Ladder ladders[], int *currentTurn);
int LoadGame(Player players[], int *np, int *currentTurn, Snake snakes[], Ladder ladders[], char *statusMsg);
void SaveGame(Player players[], int np, int currentTurn, Snake snakes[], Ladder ladders[], char *statusMsg);
void start_animation(Player *p, int oldPos, int newPos);

// --- Function Prototypes (Defined in drawing.c) ---

Vec2i cell_to_pixel(int cell, int cellSize);
void draw_board(int size, int cellSize);
void draw_snakes_and_ladders(Snake snakes[], Ladder lads[], int cellSize);
void draw_players(Player players[], int np, int cellSize);
void draw_ui_panel(Player players[], int np, int currentTurn);

#endif // S_AND_L_H