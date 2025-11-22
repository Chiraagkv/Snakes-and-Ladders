// gui_snakes_and_ladders.c gcc -o s_and_l gui_snakes_and_ladders.c -lraylib -lm -ldl -lpthread -lrt -lX11
// Made by Chiraag, Utkarsh and Naksh
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
int animPath[200];
int animPathLen = 0;
float animProgress = 0.0f;
float animDuration = 0.18f; // seconds per step
Player *animPlayer = NULL;

int roll(){
    return (rand()%6) + 1;
}
Color palette[] = {RED, GREEN, BLUE, ORANGE, PURPLE, GOLD};

/* Helper: smoothstep easing */
static float smoothstep(float x) {
    if (x < 0) return 0;
    if (x > 1) return 1;
    return x*x*(3 - 2*x);
}

/* Snakes & Ladders placement with validation  */
void snakes_and_ladders(Snake snakes[], Ladder ladders[]){
    int total = SIZE * SIZE;
    int used[1010] = {0}; // assume board <= 1000 cells; increase if needed

    // ensure N_LADDERS and N_SNAKES not too many
    if (N_LADDERS > total/6) N_LADDERS = total/6;
    if (N_SNAKES > total/6) N_SNAKES = total/6;

    // Place ladders
    for(int i=0;i<N_LADDERS;i++){
        int bottom, top;
        int tries = 0;
        do {
            bottom = (rand() % (total - 2)) + 2; // avoid cell 1
            int climb = (rand()% (SIZE-1)) + 1; // at least 1 row but less than SIZE
            top = bottom + climb * SIZE + (rand() % SIZE); // bias upward
            if (top > total) top = bottom + (rand()% (SIZE)) + SIZE;
            tries++;
            if (tries > 500) break;
        } while(
            top > total ||
            bottom >= top ||
            ( (bottom-1)/SIZE == (top-1)/SIZE ) || // same row
            used[bottom] || used[top] ); // already used

        // mark and assign
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
            mouth = (rand() % (total - 1)) + 2; // not cell 1
            int drop = (rand()% (SIZE-1)) + 1; // drop at least one row
            tail = mouth - (drop * SIZE) - (rand() % SIZE);
            tries++;
            if (tries > 500) break;
        } while(
            tail < 1 ||
            mouth <= tail ||
            ( (mouth-1)/SIZE == (tail-1)/SIZE ) || // same row
            used[mouth] || used[tail] ); // collision with existing

        snakes[i].mouth = mouth;
        snakes[i].tail = tail;
        if (mouth >= 1) used[mouth] = 1;
        if (tail >= 1) used[tail] = 1;
    }

    // Extra safety: avoid snake mouth on ladder bottom and ladder top on snake mouth etc.
    for(int i=0;i<N_SNAKES;i++){
        for(int j=0;j<N_LADDERS;j++){
            if (snakes[i].mouth == ladders[j].bottom) {
                // move snake mouth slightly up if possible
                if (snakes[i].mouth + SIZE <= total) snakes[i].mouth += SIZE;
                else if (snakes[i].mouth - SIZE >= 1) snakes[i].mouth -= SIZE;
            }
            if (snakes[i].tail == ladders[j].top) {
                if (snakes[i].tail - SIZE >= 1) snakes[i].tail -= SIZE;
                else if (snakes[i].tail + SIZE <= total) snakes[i].tail += SIZE;
            }
        }
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

    int screenCol;
    // serpentine: even-numbered rows from bottom go left-to-right
    if (r % 2 == 0)
        screenCol = c;
    else
        screenCol = (SIZE - 1 - c);

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

            int r = SIZE - 1 - row; // board row index (0 at top)
            // compute board cell number properly (serpentine)
            int cell;
            if (r % 2 == 0)
                cell = r * SIZE + col + 1;
            else
                cell = r * SIZE + (SIZE - col);

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

        // Draw simple rungs with perpendicular
        Vector2 dir = NormalizeVec((Vector2){t.x-b.x, t.y-b.y});
        Vector2 perp = (Vector2){-dir.y, dir.x};

        for(int k=1;k<=3;k++){
            float f = k/4.0f;
            Vector2 mid = { b.x + (t.x-b.x)*f, b.y + (t.y-b.y)*f };
            DrawLine(
                (int)(mid.x - perp.x*8), (int)(mid.y - perp.y*8),
                (int)(mid.x + perp.x*8), (int)(mid.y + perp.y*8),
                DARKGREEN);
        }
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
    // ensure we include last cell
    for(int pos = oldPos; pos != newPos + step; pos += step){
        animPath[animPathLen++] = pos;
        if (animPathLen >= 199) break;
    }
    // set duration per step (shorter for single-step)
    animDuration = 0.14f; // base speed per step
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

    // Rolling/dice animation variables
    int diceDisplay = 1;
    int rolling = 0;
    float rollingTimer = 0.0f;
    float rollingDuration = 0.5f; // seconds

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
                int allFilled = 1;
                for(int i=0;i<np;i++) if (strlen(inputBuffers[i]) == 0) allFilled = 0;
                if (allFilled) {
                    for(int i=0;i<np;i++) strcpy(players[i].name, inputBuffers[i]);
                    enteringNames = 0;
                } else {
                    // brief visual feedback
                    inputBuffers[currentTyping][strlen(inputBuffers[currentTyping])] = '\0';
                }
            }
            continue;
        }

        // Input: click roll button or space
        // Handle rolling animation triggered by mouse click or space
        if (!rolling && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            if (CheckCollisionPointRec(m, rollBtn)) {
                rolling = 1;
                rollingTimer = 0.0f;
            }
        }
        if (!rolling && IsKeyPressed(KEY_SPACE)) {
            rolling = 1;
            rollingTimer = 0.0f;
        }

        // Save/Load/Restart keys
        if (IsKeyPressed(KEY_S)) {
            // Save game state to file
            FILE *f = fopen("s_and_l_save.txt","w");
            if (f) {
                fprintf(f, "%d\n", np);
                fprintf(f, "%d\n", currentTurn);
                for(int i=0;i<np;i++){
                    fprintf(f, "%s\n%d\n", players[i].name, players[i].position);
                }
                for(int i=0;i<N_SNAKES;i++) fprintf(f, "%d %d\n", snakes[i].mouth, snakes[i].tail);
                for(int i=0;i<N_LADDERS;i++) fprintf(f, "%d %d\n", ladders[i].bottom, ladders[i].top);
                fclose(f);
                sprintf(statusMsg, "Game saved to s_and_l_save.txt");
            } else {
                sprintf(statusMsg, "Error: cannot save file");
            }
        }
        if (IsKeyPressed(KEY_L)) {
            // Load game state from file
            FILE *f = fopen("s_and_l_save.txt","r");
            if (f) {
                int loadedNp = 0;
                if (fscanf(f, "%d\n", &loadedNp) == 1) {
                    if (loadedNp >=2 && loadedNp <= MAX_PLAYERS) {
                        np = loadedNp;
                        fscanf(f, "%d\n", &currentTurn);
                        for(int i=0;i<np;i++){
                            char namebuf[64];
                            fgets(namebuf, sizeof(namebuf), f);
                            // remove newline
                            if (namebuf[strlen(namebuf)-1] == '\n') namebuf[strlen(namebuf)-1] = '\0';
                            strcpy(players[i].name, namebuf);
                            fscanf(f, "%d\n", &players[i].position);
                        }
                        for(int i=0;i<N_SNAKES;i++) fscanf(f, "%d %d\n", &snakes[i].mouth, &snakes[i].tail);
                        for(int i=0;i<N_LADDERS;i++) fscanf(f, "%d %d\n", &ladders[i].bottom, &ladders[i].top);
                        sprintf(statusMsg, "Game loaded.");
                    } else {
                        sprintf(statusMsg, "Save file has invalid player count");
                    }
                } else {
                    sprintf(statusMsg, "Invalid save file");
                }
                fclose(f);
            } else {
                sprintf(statusMsg, "No save file found");
            }
        }
        if (IsKeyPressed(KEY_R)) {
            // Restart: reset positions, generate new board
            for(int i=0;i<np;i++){
                players[i].position = 0;
                strcpy(players[i].name, players[i].name); // keep names
            }
            currentTurn = 0;
            snakes_and_ladders(snakes, ladders);
            sprintf(statusMsg, "Game restarted");
        }

        // Rolling animation progress
        if (rolling) {
            rollingTimer += GetFrameTime();
            // update dice display rapidly
            if ((int)(rollingTimer * 20) % 2 == 0) diceDisplay = (rand()%6) + 1;
            if (rollingTimer >= rollingDuration) {
                rolling = 0;
                // final dice result:
                diceDisplay = (rand()%6) + 1;

                // Perform actual roll and animate movement
                Player *p = &players[currentTurn % np];
                int oldPos = p->position;
                // move according to diceDisplay but use roll_and_move to keep snakes/ladders logic
                // temporarily set roll() deterministic by using diceDisplay: we'll simulate
                int x = diceDisplay;
                if (x + p->position <= (SIZE*SIZE)) p->position += x;
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
                int newPos = p->position;
                start_animation(p, oldPos, newPos);
                if (newPos == SIZE*SIZE) {
                    sprintf(statusMsg, "%s wins!!", p->name);
                } else {
                    sprintf(statusMsg, "%s rolled %d. pos=%d", p->name, x, newPos);
                    currentTurn++;
                }
            }
        }

        // Drawing
        BeginDrawing();
            ClearBackground(RAYWHITE);
            // --- UPDATE ANIMATION FRAME ---
            if (animating) {
                float dt = GetFrameTime();
                animProgress += dt / animDuration;
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
                // Highlight current player
                if (i == (currentTurn % np)) {
                    DrawRectangle(px+5, 55 + i*30 - 2, 170, 26, Fade(YELLOW, 0.15f));
                }

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

                    float t = smoothstep(animProgress);
                    float x = a.x * (1 - t) + b.x * t;
                    float y = a.y * (1 - t) + b.y * t;

                    // Slight offset grid inside cell to avoid overlap
                    int tokenSpacing = cellSize / 6;
                    int col = i % 3;
                    int row = (i / 3);
                    int dx = (col - 1) * tokenSpacing;
                    int dy = (row - 1) * tokenSpacing/1.2;

                    DrawCircle(x + dx, y + dy, 12, players[i].color);
                    continue;
                }

                // Normal (non animated)
                if (pos == 0) {
                    // off-board area for tokens: arrange horizontally
                    int baseX = BOARD_MARGIN/2 + 10;
                    int baseY = SCREEN_H - BOARD_MARGIN/2 - 10;
                    int spacing = 22;
                    DrawCircle(baseX + i*spacing, baseY, 14, players[i].color);
                } else {
                    Vec2i pxy = cell_to_pixel(pos, cellSize);
                    // Add small offsets for players on same cell (3x2 grid)
                    int tokenSpacing = cellSize / 6;
                    if (tokenSpacing < 10) tokenSpacing = 10;
                    int col = i % 3;
                    int row = i / 3;
                    int dx = (col - 1) * tokenSpacing;
                    int dy = (row - 1) * (tokenSpacing/1.2);

                    DrawCircle(pxy.x + dx, pxy.y + dy, 12, players[i].color);
                }
            }


            // Roll button
            DrawRectangleRec(rollBtn, LIGHTGRAY);
            DrawRectangleLinesEx(rollBtn, 2, DARKGRAY);
            DrawText("ROLL (Space)", rollBtn.x + 12, rollBtn.y + 14, 18, BLACK);

            // dice display above button
            DrawText(TextFormat("%d", diceDisplay), rollBtn.x + 60, rollBtn.y - 40, 30, BLACK);
            if (rolling) {
                DrawText("Rolling...", rollBtn.x + 95, rollBtn.y - 40, 12, DARKGRAY);
            }

            // status
            DrawText(statusMsg, SCREEN_W - 500, SCREEN_H - 40, 16, DARKGRAY);

            // hint keys
            DrawText("S=Save  L=Load  R=Restart", 40, SCREEN_H - 40, 14, GRAY);

        EndDrawing();

        // if someone wins, wait a moment then break
        for (int i=0;i<np;i++){
            if (players[i].position == SIZE*SIZE) {
                // show for a few frames
                for (int f=0; f<120 && !WindowShouldClose(); f++){
                    BeginDrawing(); EndDrawing();
                }

                // AUTO SAVE before closing on win   <-- ADDED
                FILE *autoF = fopen("s_and_l_save.txt", "w");
                if (autoF) {
                    fprintf(autoF, "%d\n", np);
                    fprintf(autoF, "%d\n", currentTurn);
                    for (int k=0; k<np; k++) {
                        fprintf(autoF, "%s\n%d\n", players[k].name, players[k].position);
                    }
                    for(int k=0;k<N_SNAKES;k++) fprintf(autoF, "%d %d\n", snakes[k].mouth, snakes[k].tail);
                    for(int k=0;k<N_LADDERS;k++) fprintf(autoF, "%d %d\n", ladders[k].bottom, ladders[k].top);
                    fclose(autoF);
                }
                // END AUTO SAVE

                CloseWindow();
                return 0;
            }
        }
    }

    // AUTO SAVE on normal close  <-- ADDED
// AUTO-SAVE ON NORMAL CLOSE (write the REAL snakes/ladders)
    {
    FILE *autoF2 = fopen("s_and_l_save.txt", "w");
    if (autoF2) {
        fprintf(autoF2, "%d\n", np);
        fprintf(autoF2, "%d\n", currentTurn);

        for (int k = 0; k < np; k++)
            fprintf(autoF2, "%s\n%d\n", players[k].name, players[k].position);

        for (int k = 0; k < N_SNAKES; k++)
            fprintf(autoF2, "%d %d\n", snakes[k].mouth, snakes[k].tail);

        for (int k = 0; k < N_LADDERS; k++)
            fprintf(autoF2, "%d %d\n", ladders[k].bottom, ladders[k].top);

        fclose(autoF2);
        }
    }

    // END AUTO SAVE

    CloseWindow();
    return 0;
}
