#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "raylib.h"
#include <math.h>
#include "../include/s_and_l.h"

// --- Global Variables DEFINITIONS (Game state) ---
char inputBuffers[MAX_PLAYERS][20];
int currentTyping = 0;

// Rolling State DEFINITIONS
int rolling = 0;
float rollingTimer = 0.0f;
float rollingDuration = 0.5f; // Duration in seconds

// Animation Variable DEFINITIONS
int animating = 0;
Player *animPlayer = NULL;
int animPath[200];
int animPathLen = 0;
int animStepIndex = 0;
float animProgress = 0.0f;
float animDuration = 0.14f; 
// Define and initialize the palette array here
Color palette[PALETTE_SIZE] = {RED, GREEN, BLUE, ORANGE, PURPLE, GOLD};


// Helper: generate a random dice roll (1-6)
int roll(void){
    return (rand()%6) + 1;
}

/* Snakes & Ladders placement with validation */
void snakes_and_ladders(Snake snakes[], Ladder ladders[]){
    int total = SIZE * SIZE;
    int used[1010] = {0};

    // Place ladders
    for(int i=0;i<N_LADDERS;i++){
        int bottom, top;
        int tries = 0;
        do {
            bottom = (rand() % (total - 2)) + 2; 
            int climb = (rand()% (SIZE-1)) + 1; 
            top = bottom + climb * SIZE + (rand() % SIZE); 
            if (top > total) top = bottom + (rand()% (SIZE)) + SIZE;
            tries++;
            if (tries > 500) break;
        } while(
            top > total || bottom >= top ||
            ( (bottom-1)/SIZE == (top-1)/SIZE ) || used[bottom] || used[top] );

        ladders[i].bottom = bottom;
        ladders[i].top = top;
        if (bottom <= total) used[bottom] = 1;
        if (top <= total) used[top] = 1;
    }

    // Place snakes
    for(int i=0;i<N_SNAKES;i++){
        int mouth, tail;
        int tries = 0;
        do {
            mouth = (rand() % (total - 1)) + 2; 
            int drop = (rand()% (SIZE-1)) + 1; 
            tail = mouth - (drop * SIZE) - (rand() % SIZE);
            tries++;
            if (tries > 500) break;
        } while(
            tail < 1 || mouth <= tail ||
            ( (mouth-1)/SIZE == (tail-1)/SIZE ) || used[mouth] || used[tail] );

        snakes[i].mouth = mouth;
        snakes[i].tail = tail;
        if (mouth >= 1) used[mouth] = 1;
        if (tail >= 1) used[tail] = 1;
    }
}

// Function to check if a file exists
int CheckSaveFileExists(const char *fileName) {
    FILE *file = fopen(fileName, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// Function to delete the save file
void DeleteSaveFile(const char *fileName) {
    if (remove(fileName) == 0) {
        // Success
    } else {
        // Failure (file might not exist, which is fine)
    }
}

// Function to reset and start a new game (keeping player count/names)
void InitNewGame(Player players[], int np, Snake snakes[], Ladder ladders[], int *currentTurn) {
    for(int i=0;i<np;i++){
        players[i].position = 0;
        players[i].id = i+1;
        // Use PALETTE_SIZE from header
        players[i].color = palette[i % PALETTE_SIZE];
    }
    *currentTurn = 0;
    snakes_and_ladders(snakes, ladders);
}

// Function to load game state
int LoadGame(Player players[], int *np, int *currentTurn, Snake snakes[], Ladder ladders[], char *statusMsg) {
    FILE *f = fopen(SAVE_FILE_NAME,"r");
    if (f) {
        int loadedNp = 0;
        if (fscanf(f, "%d\n", &loadedNp) == 1) {
            if (loadedNp >=2 && loadedNp <= MAX_PLAYERS) {
                *np = loadedNp;
                fscanf(f, "%d\n", currentTurn);
                for(int i=0;i<*np;i++){
                    players[i].id = i+1;
                    // Use PALETTE_SIZE from header
                    players[i].color = palette[i % PALETTE_SIZE];
                    
                    char namebuf[64];
                    fgets(namebuf, sizeof(namebuf), f);
                    if (namebuf[strlen(namebuf)-1] == '\n') namebuf[strlen(namebuf)-1] = '\0';
                    strcpy(players[i].name, namebuf);
                    strcpy(inputBuffers[i], namebuf); 
                    fscanf(f, "%d\n", &players[i].position);
                }
                for(int i=0;i<N_SNAKES;i++) fscanf(f, "%d %d\n", &snakes[i].mouth, &snakes[i].tail);
                for(int i=0;i<N_LADDERS;i++) fscanf(f, "%d %d\n", &ladders[i].bottom, &ladders[i].top);
                fclose(f);
                if(statusMsg) sprintf(statusMsg, "Game loaded from save file.");
                return 1;
            } else {
                fclose(f);
                if(statusMsg) sprintf(statusMsg, "Save file has invalid player count. Starting new game.");
                return 0;
            }
        } else {
            fclose(f);
            if(statusMsg) sprintf(statusMsg, "Invalid save file format. Starting new game.");
            return 0;
        }
    } else {
        if(statusMsg) sprintf(statusMsg, "No save file found.");
        return 0;
    }
}

// Function to save game state
void SaveGame(Player players[], int np, int currentTurn, Snake snakes[], Ladder ladders[], char *statusMsg) {
    FILE *f = fopen(SAVE_FILE_NAME,"w");
    if (f) {
        fprintf(f, "%d\n", np);
        fprintf(f, "%d\n", currentTurn);
        for(int i=0;i<np;i++){
            fprintf(f, "%s\n%d\n", players[i].name, players[i].position);
        }
        for(int i=0;i<N_SNAKES;i++) fprintf(f, "%d %d\n", snakes[i].mouth, snakes[i].tail);
        for(int i=0;i<N_LADDERS;i++) fprintf(f, "%d %d\n", ladders[i].bottom, ladders[i].top);
        fclose(f);
        if(statusMsg) sprintf(statusMsg, "Game saved to %s", SAVE_FILE_NAME);
    } else {
        if(statusMsg) sprintf(statusMsg, "Error: cannot save file");
    }
}

// Initializes the animation path
void start_animation(Player *p, int oldPos, int newPos) {
    animating = 1;
    animStepIndex = 0;
    animPlayer = p;
    animProgress = 0.0f;
    animPathLen = 0;

    int step = (newPos > oldPos) ? 1 : -1;
    // create path for animation
    for(int pos = oldPos; pos != newPos + step; pos += step){
        animPath[animPathLen++] = pos;
        if (animPathLen >= 199) break;
    }
    animDuration = 0.14f; 
}