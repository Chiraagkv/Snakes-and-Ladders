#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


int SIZE=3;
int N_SNAKES=1;
int N_LADDERS=1;

// defining structs
typedef struct {
    char name[20];
    int id;
    int position;
}Player;

typedef struct {
    int mouth;
    int tail;
} Snake;

typedef struct {
    int top;
    int bottom;
} Ladder;

// defining functions
int roll(){
    return (rand()%6) + 1;
}

void init_players(Player *players, int n){
    static int ID=1;
    char name[20];
    for (int i=0;i<n;i++){
        printf("Enter Player %d's name: ", i+1);
        scanf("%s", name);
        strcpy(players[i].name, name);
        players[i].position = 0;
        players[i].id = ID++;
    }
}

void snakes_and_ladders(Snake snakes[], Ladder lads[]){
    int m, t;
    for (int i=0;i<N_SNAKES;i++){
        m = (rand()%(SIZE*SIZE-1)) + 1;
        t = (rand()%(m-1)) + 1;
        Snake s = {m, t};
        snakes[i] = s;
    }

    for (int i=0;i<N_LADDERS;i++){
        int bottom = (rand() % (SIZE*SIZE - 10)) + 1;
        int top = bottom + (rand() % (SIZE*SIZE - bottom)) + 1;
        Ladder l = {top, bottom};
        lads[i] = l;
    }
}

int roll_and_move(Player *p, Snake snakes[], Ladder ladders[]){
    int x = roll();
    if (x + p->position <= (SIZE*SIZE)){
        p->position += x;
    }
    printf("Player %s moved to position %d\n", p->name, p->position);
    for (int i=0;i<N_SNAKES;i++){
        if (p->position == snakes[i].mouth){
            p->position = snakes[i].tail;
            printf("Player %s got bit by a snake! Current position: %d\n", p->name, p->position);
            break;
        }
    }
    for (int i=0;i<N_LADDERS;i++){
        if (p->position == ladders[i].bottom){
            p->position = ladders[i].top;
            printf("Player %s climbed a ladder! Current position: %d\n", p->name, p->position);
            break;
        }
    }
    return p->position;
}

void save_game(Player players[], int np, Snake snakes[], Ladder ladders[], int currentTurn) {
    FILE *f = fopen("savegame.dat", "wb");
    if (!f) {
        printf("Error opening save file!\n");
        return;
    }

    fwrite(&np, sizeof(int), 1, f);
    fwrite(players, sizeof(Player), np, f);
    fwrite(snakes, sizeof(Snake), N_SNAKES, f);
    fwrite(ladders, sizeof(Ladder), N_LADDERS, f);
    fwrite(&currentTurn, sizeof(int), 1, f);

    fclose(f);
    printf("\nGame saved successfully.\n");
}

int load_game(Player players[], int *np, Snake snakes[], Ladder ladders[], int *currentTurn) { // currentTurn tells where to put the value of the current turn
    FILE *f = fopen("savegame.dat", "rb");
    if (!f) {
        printf("No saved game found.\n");
        return 1; // 1 means failure
    }

    fread(np, sizeof(int), 1, f);
    fread(players, sizeof(Player), *np, f);
    fread(snakes, sizeof(Snake), N_SNAKES, f);
    fread(ladders, sizeof(Ladder), N_LADDERS, f);
    fread(currentTurn, sizeof(int), 1, f);

    fclose(f);
    printf("\nGame loaded successfully.\n");
    return 0; // success
}

// Game loop
int main(){
    srand(time(NULL));
    int np;
    printf("Enter number of players: ");
    scanf("%d", &np);
    Player players[np];
    init_players(players, np);
    Snake snakes[N_SNAKES];
    Ladder ladders[N_LADDERS];
    snakes_and_ladders(snakes, ladders);
    int i = 0;
    while(1){
        Player *player_pointer = &players[i%np];
        while (getchar() != '\n');  // clear leftover input from previous scanf
        getchar();
        int pos = roll_and_move(player_pointer, snakes, ladders);
        if (pos == (SIZE*SIZE)){
            printf("Player %s wins!!\n", players[i%np].name);
            break;
        }
        i++;
    }
}
