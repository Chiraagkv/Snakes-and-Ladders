// gui_snakes_and_ladders.c gcc -o s_and_l gui_snakes_and_ladders.c -lraylib -lm -ldl -lpthread -lrt -lX11

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "raylib.h"
#include <math.h>

int SIZE = 10;
int N_SNAKES = 5;
int N_LADDERS = 5;
#define MAX_PLAYERS 6
int enteringNames = 1;
char inputBuffers[MAX_PLAYERS][20];
int currentTyping = 0;


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


int animating = 0;
int animStepIndex = 0;
int animPath[20];
int animPathLen = 0;
float animProgress = 0.0f;
Player *animPlayer = NULL;

int roll(){
    return (rand()%6) + 1;
}
Color palette[] = {RED, GREEN, BLUE, ORANGE, PURPLE, GOLD};

void snakes_and_ladders(Snake snakes[], Ladder ladders[]){
    for(int i=0;i<N_LADDERS;i++){
        int bottom, top;
        do {
            bottom = (rand()%(SIZE*SIZE-1))+1;
            top = bottom + (rand()%SIZE) + SIZE; // climb at least 1 full row
        } while(top > SIZE*SIZE || (bottom/SIZE)==(top/SIZE)); // ensure not same row

        ladders[i].bottom = bottom;
        ladders[i].top = top;
    }

    for(int i=0;i<N_SNAKES;i++){
        int mouth, tail;
        do {
            mouth = (rand()%(SIZE*SIZE-1))+2;
            tail = mouth - (rand()%SIZE) - SIZE; // drop at least 1 full row
        } while(tail < 1 || (mouth/SIZE)==(tail/SIZE)); // ensure not same row

        snakes[i].mouth = mouth;
        snakes[i].tail = tail;
    }
}



// ----- GUI helpers -----
const int SCREEN_W = 1000;
const int SCREEN_H = 800;
const int BOARD_MARGIN = 40;

typedef struct { int x,y; } Vec2i;

Vec2i cell_to_pixel(int cell, int cellSize) {
    Vec2i v = {0,0};
    if (cell < 1 || cell > SIZE*SIZE) return v;

    int r = (cell - 1) / SIZE;      // 0 at bottom, SIZE-1 at top
    int c = (cell - 1) % SIZE;

    int screenRow = SIZE - 1 - r;   // invert vertically

    int screenCol = (r % 2 == 0) ? c : (SIZE - 1 - c);

    v.x = BOARD_MARGIN + screenCol * cellSize + cellSize/2;
    v.y = BOARD_MARGIN + screenRow * cellSize + cellSize/2;
    return v;
}


void draw_board(int size, int cellSize) {
    int total = size * cellSize;
    for (int row=0; row < size; row++){
        for (int col=0; col < size; col++){
            int x = BOARD_MARGIN + col * cellSize;
            int y = BOARD_MARGIN + row * cellSize;
            DrawRectangleLines(x,y,cellSize,cellSize, DARKGRAY);
            int r = SIZE - 1 - row; // board row from top visually
            int base = r * SIZE;
            int cell = base + (r % 2 == 0 ? col + 1 : (SIZE - col));
            DrawText(TextFormat("%d", cell), x + 6, y + 6, 12, DARKGRAY);

        }
    }
}
Vector2 NormalizeVec(Vector2 v) {
    float len = sqrtf(v.x*v.x + v.y*v.y);
    if (len == 0) return (Vector2){0,0};
    return (Vector2){v.x/len, v.y/len};
}

void draw_snakes_and_ladders(Snake snakes[], Ladder lads[], int cellSize){
    for(int i=0;i<N_LADDERS;i++){
        Vec2i b = cell_to_pixel(lads[i].bottom, cellSize);
        Vec2i t = cell_to_pixel(lads[i].top, cellSize);

        DrawLineEx((Vector2){b.x,b.y},(Vector2){t.x,t.y},6.0f, DARKGREEN);
        
        // Draw rungs (doesn't work)
        // Vector2 dir = NormalizeVec((Vector2){t.x-b.x, t.y-b.y});
        // Vector2 perp = (Vector2){-dir.y, dir.x};

        // for(int k=1;k<=3;k++){
        //     float f = k/4.0f;
        //     Vector2 mid = {b.x + (t.x-b.x)*f, b.y + (t.y-b.y)*f};
        //     DrawLineEx(
        //         (Vector2){mid.x - perp.x*6, mid.y - perp.y*6},
        //         (Vector2){mid.x + perp.x*6, mid.y + perp.y*6},
        //         3, DARKGREEN);
        // }
    }
    for(int i=0;i<N_SNAKES;i++){
        Vec2i m = cell_to_pixel(snakes[i].mouth, cellSize);
        Vec2i t = cell_to_pixel(snakes[i].tail, cellSize);

        DrawLineBezier((Vector2){m.x,m.y}, (Vector2){t.x,t.y}, 4.0f, MAROON);
    }
}



int roll_and_move(Player *p, Snake snakes[], Ladder ladders[]){
    int x = roll();
    if (x + p->position <= (SIZE*SIZE)){
        p->position += x;
    }
    for (int i=0;i<N_SNAKES;i++){
        if (p->position == snakes[i].mouth){
            p->position = snakes[i].tail;
            break;
        }
    }
    for (int i=0;i<N_LADDERS;i++){
        if (p->position == ladders[i].bottom){
            p->position = ladders[i].top;
            break;
        }
    }
    return p->position;
}
void start_animation(Player *p, int oldPos, int newPos) {
    animating = 1;
    animStepIndex = 0;
    animPlayer = p;
    animProgress = 0.0f;
    animPathLen = 0;

    int step = (newPos > oldPos) ? 1 : -1;
    for(int pos = oldPos; pos != newPos+step; pos += step){
        animPath[animPathLen++] = pos;
    }
}

// ----- main GUI loop -----
int main(void) {
    srand(time(NULL));
    InitWindow(SCREEN_W, SCREEN_H, "Snake & Ladders");
    SetTargetFPS(60);

    // game state
    int np=6; // Gotta take input
    int done = 0;
    Player players[6]; // max 6
    
    while (!done){
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Enter Number of Players", 40, 20, 30, DARKGRAY);
        DrawRectangle(40, 80, 250, 35, LIGHTGRAY);
        DrawText("", 45, 88, 20, BLACK);
        DrawText("Press ENTER to confirm", 40, 80 + 2*50 + 20, 18, GRAY);
        int key = GetKeyPressed();
        if (key >= 50 && key <= 54) {
            np = key - 48;
            char text[2];
            text[0] = (char)key; // the digit itself
            text[1] = '\0'; 
            BeginDrawing();
            DrawText(text, 45, 88, 20, BLACK);
            EndDrawing();
        }
        else if (IsKeyPressed(KEY_ENTER) && np >= 2 && np <= 6) {
            done = 1;
        }
        else{
            char text[2];
            text[0] = (char)(np+48); // the digit itself
            text[1] = '\0'; 
            BeginDrawing();
            DrawText(text, 45, 88, 20, BLACK);
            DrawText("Number of players should be between 2 and 6", 40, 80 + 8*50 + 20, 18, RED);
            EndDrawing();
        }
    }
    for(int i=0;i<np;i++){
        players[i].position = 0;
        players[i].id = i+1;
        players[i].color = palette[i % (sizeof(palette)/sizeof(Color))];
        strcpy(players[i].name, "");
        strcpy(inputBuffers[i], "");
    }



    Snake snakes[10]; Ladder ladders[10];
    snakes_and_ladders(snakes, ladders);

    int currentTurn = 0;
    int cellSize = (SCREEN_H - 2*BOARD_MARGIN) / SIZE; // square cells

    // Roll button rectangle
    Rectangle rollBtn = { SCREEN_W - 180, SCREEN_H - 80, 150, 50 };

    char statusMsg[200] = "Click ROLL or press Space";

    while (!WindowShouldClose()) {
        if (enteringNames) {
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Enter Player Names", 40, 20, 30, DARKGRAY);

            for(int i=0;i<np;i++){
                DrawRectangle(40, 80 + i*50, 250, 35, LIGHTGRAY);
                DrawText(inputBuffers[i], 45, 88 + i*50, 20, BLACK);
            }

            DrawText("Press ENTER to confirm a name", 40, 80 + np*50 + 20, 18, GRAY);
            DrawText("Press SPACE to START game when all names done", 40, 80 + np*50 + 45, 18, DARKGREEN);

            EndDrawing();

            int key = GetKeyPressed();
            if (key >= 32 && key <= 126) {
                int len = strlen(inputBuffers[currentTyping]);
                if (len < 19) inputBuffers[currentTyping][len] = key, inputBuffers[currentTyping][len+1] = '\0';
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                int len = strlen(inputBuffers[currentTyping]);
                if (len > 0) inputBuffers[currentTyping][len-1] = '\0';
            }
            if (IsKeyPressed(KEY_ENTER)) {
                strcpy(players[currentTyping].name, inputBuffers[currentTyping]);
                currentTyping = (currentTyping+1) % np;
            }
            if (IsKeyPressed(KEY_SPACE)) {
                for(int i=0;i<np;i++) strcpy(players[i].name, inputBuffers[i]);
                enteringNames = 0;
            }
            continue;
        }

        // Input: click roll button or space
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            if (CheckCollisionPointRec(m, rollBtn)) {
                Player *p = &players[currentTurn % np];
                int oldPos = p->position;
                int pos = roll_and_move(p, snakes, ladders);
                start_animation(p, oldPos, p->position);
                sprintf(statusMsg, "%s rolled. pos=%d", p->name, pos);
                if (pos == SIZE*SIZE) {
                    sprintf(statusMsg, "%s wins!!", p->name);
                } else {
                    currentTurn++;
                }
            }
        }
        if (IsKeyPressed(KEY_SPACE)) {
            Player *p = &players[currentTurn % np];
            int pos = roll_and_move(p, snakes, ladders);
            sprintf(statusMsg, "%s rolled. pos=%d", p->name, pos);
            if (pos == SIZE*SIZE) {
                sprintf(statusMsg, "%s wins!!", p->name);
            } else {
                currentTurn++;
            }
        }

        // Drawing
        BeginDrawing();
            ClearBackground(RAYWHITE);
            // --- UPDATE ANIMATION FRAME ---
            if (animating) {
                animProgress += 0.02f; // adjust speed if needed
                if (animProgress >= 1.0f) {
                    animProgress = 0.0f;
                    animStepIndex++;
                    if (animStepIndex >= animPathLen - 1) {
                        animating = 0; // animation finished
                    }
                }
            }

            DrawText("Snake & Ladders (raylib)", 10, 10, 20, GRAY);
            int px = SCREEN_W - 200;
            DrawRectangle(px, 20, 180, np*30 + 20, Fade(DARKGRAY, 0.2f));
            DrawText("Players:", px+10, 30, 20, BLACK);

            for(int i=0;i<np;i++){
                DrawRectangle(px+10, 60 + i*30, 18, 18, players[i].color);
                DrawText(players[i].name, px+35, 60 + i*30, 18, BLACK);
}


            draw_board(SIZE, cellSize);
            draw_snakes_and_ladders(snakes, ladders, cellSize);

            // draw players
            for (int i = 0; i < np; i++) {
                int pos = players[i].position;

                // If this is the animating player, draw interpolated position
                if (animating && animPlayer == &players[i] && animStepIndex < animPathLen - 1) {
                    Vec2i a = cell_to_pixel(animPath[animStepIndex], cellSize);
                    Vec2i b = cell_to_pixel(animPath[animStepIndex + 1], cellSize);

                    float t = animProgress;
                    float x = a.x * (1 - t) + b.x * t;
                    float y = a.y * (1 - t) + b.y * t;

                    DrawCircle(x, y, 14, players[i].color);
                    continue;
                }

                // Normal (non animated)
                if (pos == 0) {
                    DrawCircle(BOARD_MARGIN/2 + 10 + i*18, SCREEN_H - BOARD_MARGIN/2 - 10, 14, players[i].color);
                } else {
                    Vec2i pxy = cell_to_pixel(pos, cellSize);
                    DrawCircle(pxy.x, pxy.y, 14, players[i].color);
                }
            }


            // Roll button
            DrawRectangleRec(rollBtn, LIGHTGRAY);
            DrawRectangleLinesEx(rollBtn, 2, DARKGRAY);
            DrawText("ROLL (Space)", rollBtn.x + 12, rollBtn.y + 14, 18, BLACK);

            // status
            DrawText(statusMsg, SCREEN_W - 380, SCREEN_H - 40, 16, DARKGRAY);

        EndDrawing();

        // if someone wins, wait a moment then break
        for (int i=0;i<np;i++){
            if (players[i].position == SIZE*SIZE) {
                // show for a few frames
                for (int f=0; f<120 && !WindowShouldClose(); f++){
                    BeginDrawing(); EndDrawing();
                }
                CloseWindow();
                return 0;
            }
        }
    }

    CloseWindow();
    return 0;
}