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

// --- New State Management ---
typedef enum {
    STATE_LOAD_OR_NEW = 0, // Check for save file, ask to resume
    STATE_INPUT_PLAYERS,   // State to input player count (2-6)
    STATE_ENTERING_NAMES,  // State to input player names
    STATE_GAMEPLAY,        // Main game loop
    STATE_GAME_OVER        // Game finished (win screen)
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


void snakes_and_ladders(Snake snakes[], Ladder ladders[]){
    int total = SIZE * SIZE;
    int used[1010] = {0}; 


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

// --- New Utility Functions ---

// Function to check if a file exists (Renamed from FileExists to avoid raylib conflict)
int CheckSaveFileExists(const char *fileName) {
    FILE *file = fopen(fileName, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// Function to reset and start a new game (keeping player count/names)
// Removed statusMsg argument as it was incorrectly passed from main
void InitNewGame(Player players[], int np, Snake snakes[], Ladder ladders[], int *currentTurn) {
    for(int i=0;i<np;i++){
        players[i].position = 0;
        players[i].id = i+1;
        players[i].color = palette[i % (sizeof(palette)/sizeof(Color))];
        // Names are preserved from the previous setup
    }
    *currentTurn = 0;
    snakes_and_ladders(snakes, ladders);
}

// Function to load game state
int LoadGame(Player players[], int *np, int *currentTurn, Snake snakes[], Ladder ladders[], char *statusMsg) {
    FILE *f = fopen("s_and_l_save.txt","r");
    if (f) {
        int loadedNp = 0;
        if (fscanf(f, "%d\n", &loadedNp) == 1) {
            if (loadedNp >=2 && loadedNp <= MAX_PLAYERS) {
                *np = loadedNp;
                fscanf(f, "%d\n", currentTurn);
                for(int i=0;i<*np;i++){
                    // Re-initialize player colors and ID
                    players[i].id = i+1;
                    players[i].color = palette[i % (sizeof(palette)/sizeof(Color))];
                    
                    char namebuf[64];
                    fgets(namebuf, sizeof(namebuf), f);
                    if (namebuf[strlen(namebuf)-1] == '\n') namebuf[strlen(namebuf)-1] = '\0';
                    strcpy(players[i].name, namebuf);
                    // Also update input buffer for visual consistency if they go back to name input
                    strcpy(inputBuffers[i], namebuf); 
                    fscanf(f, "%d\n", &players[i].position);
                }
                for(int i=0;i<N_SNAKES;i++) fscanf(f, "%d %d\n", &snakes[i].mouth, &snakes[i].tail);
                for(int i=0;i<N_LADDERS;i++) fscanf(f, "%d %d\n", &ladders[i].bottom, &ladders[i].top);
                fclose(f);
                if(statusMsg) sprintf(statusMsg, "Game loaded from save file.");
                return 1; // Success
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
        return 0; // Failure
    }
}

// Function to save game state
void SaveGame(Player players[], int np, int currentTurn, Snake snakes[], Ladder ladders[], char *statusMsg) {
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
        if(statusMsg) sprintf(statusMsg, "Game saved to s_and_l_save.txt");
    } else {
        if(statusMsg) sprintf(statusMsg, "Error: cannot save file");
    }
}


// ----- main GUI loop -----
int main(void) {
    srand(time(NULL));
    InitWindow(SCREEN_W, SCREEN_H, "Snake & Ladders");
    SetTargetFPS(60);

    // Game state variables
    int np = 2; // Default starting minimum
    int currentTurn = 0;
    Player players[MAX_PLAYERS];
    Snake snakes[10];
    Ladder ladders[10];
    
    // UI variables
    int cellSize = (SCREEN_H - 2*BOARD_MARGIN) / SIZE;
    Rectangle rollBtn = { SCREEN_W - 180, SCREEN_H - 80, 150, 50 };
    Rectangle rematchBtn = { SCREEN_W/2 - 75, SCREEN_H/2 + 50, 150, 50 };
    Rectangle resumeBtn = { SCREEN_W/2 - 160, SCREEN_H/2 + 50, 150, 50 };
    Rectangle newGameBtn = { SCREEN_W/2 + 10, SCREEN_H/2 + 50, 150, 50 };

    // --- Dice/Rolling Variables (FIXED MISSING DECLARATIONS) ---
    int diceDisplay = 1;      // What the dice shows
    int rolling = 0;          // Is the dice currently rolling
    float rollingTimer = 0.0f; // Timer for rolling animation
    float rollingDuration = 0.5f; // How long the dice rolls (seconds)
    // -----------------------------------------------------------
    
    char statusMsg[200] = "Welcome to Snakes & Ladders!";
    GameState currentState = STATE_LOAD_OR_NEW;
    char winnerName[20] = "";
    int initialGameLoaded = 0;


    // --- Main Game Loop ---
    while (!WindowShouldClose()) {

        // --- UPDATE LOGIC (STATE MACHINE) ---
        Vector2 mouse = GetMousePosition();
        
        // Rolling animation progress only runs during gameplay
        if (currentState == STATE_GAMEPLAY && animating) {
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
        
        // Handle Game Logic based on State
        switch (currentState) {
            case STATE_LOAD_OR_NEW: {
                // Use renamed function
                if (CheckSaveFileExists("s_and_l_save.txt")) {
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (CheckCollisionPointRec(mouse, resumeBtn)) {
                            // Load game and transition to gameplay
                            if (LoadGame(players, &np, &currentTurn, snakes, ladders, statusMsg)) {
                                currentState = STATE_GAMEPLAY;
                                initialGameLoaded = 1; // Mark that player data is setup
                            } else {
                                // If load fails (e.g., bad file format), proceed to new game setup
                                currentState = STATE_INPUT_PLAYERS;
                            }
                        } else if (CheckCollisionPointRec(mouse, newGameBtn)) {
                            currentState = STATE_INPUT_PLAYERS;
                        }
                    }
                } else {
                    currentState = STATE_INPUT_PLAYERS;
                }
                break;
            }
            case STATE_INPUT_PLAYERS: {
                // Input logic for number of players (2-6)
                int key = GetKeyPressed();
                if (key >= 50 && key <= 54) { // 2 to 6
                    np = key - 48;
                }
                else if (IsKeyPressed(KEY_ENTER) && np >= 2 && np <= 6) {
                    // Initialize players and transition to name input
                    for(int i=0;i<np;i++){
                        players[i].position = 0;
                        players[i].id = i+1;
                        players[i].color = palette[i % (sizeof(palette)/sizeof(Color))];
                        strcpy(players[i].name, "");
                        // inputBuffers already hold names if loaded and rejected, or are empty
                    }
                    currentTyping = 0;
                    currentState = STATE_ENTERING_NAMES;
                }
                break;
            }
            case STATE_ENTERING_NAMES: {
                // Input logic for player names
                int key = GetKeyPressed();
                if (key >= 32 && key <= 126) {
                    int len = strlen(inputBuffers[currentTyping]);
                    if (len < 19) {
                        inputBuffers[currentTyping][len] = key;
                        inputBuffers[currentTyping][len+1] = '\0';
                    }
                }
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    int len = strlen(inputBuffers[currentTyping]);
                    if (len > 0) inputBuffers[currentTyping][len-1] = '\0';
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    // Only confirm if name is not empty
                    if (strlen(inputBuffers[currentTyping]) > 0) {
                        strcpy(players[currentTyping].name, inputBuffers[currentTyping]);
                        currentTyping = (currentTyping+1) % np;
                    }
                }
                if (IsKeyPressed(KEY_SPACE)) {
                    int allFilled = 1;
                    // Copy any unconfirmed names from buffer to player struct
                    for(int i=0;i<np;i++) {
                         if (strlen(inputBuffers[i]) == 0) allFilled = 0;
                         else strcpy(players[i].name, inputBuffers[i]);
                    }
                    if (allFilled) {
                        // All names set, start game
                        InitNewGame(players, np, snakes, ladders, &currentTurn);
                        currentState = STATE_GAMEPLAY;
                    } else {
                        sprintf(statusMsg, "All player names must be entered before starting!");
                    }
                }
                break;
            }
            case STATE_GAMEPLAY: {
                // --- Input: click roll button or space ---
                if (!animating) {
                    // Roll button click
                    if (!rolling && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, rollBtn)) {
                        rolling = 1;
                        rollingTimer = 0.0f;
                    }
                    // Space key press
                    if (!rolling && IsKeyPressed(KEY_SPACE)) {
                        rolling = 1;
                        rollingTimer = 0.0f;
                    }
                }

                // --- Save/Load/Restart keys (optional during gameplay) ---
                if (IsKeyPressed(KEY_S)) SaveGame(players, np, currentTurn, snakes, ladders, statusMsg);
                if (IsKeyPressed(KEY_L)) { 
                    if (LoadGame(players, &np, &currentTurn, snakes, ladders, statusMsg)) animating = 0;
                }
                // Corrected call to InitNewGame
                if (IsKeyPressed(KEY_R)) {
                    InitNewGame(players, np, snakes, ladders, &currentTurn);
                    sprintf(statusMsg, "Game restarted. %s's turn.", players[currentTurn % np].name);
                }


                // --- Rolling animation progress ---
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
                        // Move based on dice roll and check for S/L hits
                        int x = diceDisplay;
                        if (x + p->position <= (SIZE*SIZE)) p->position += x;
                        
                        int newPos = p->position;

                        // Check for S/L hits after the dice movement
                        for (int i=0;i<N_SNAKES;i++){
                            if (newPos == snakes[i].mouth){
                                newPos = snakes[i].tail;
                                break;
                            }
                        }
                        for (int i=0;i<N_LADDERS;i++){
                            if (newPos == ladders[i].bottom){
                                newPos = ladders[i].top;
                                break;
                            }
                        }
                        
                        start_animation(p, oldPos, newPos);
                        p->position = newPos; // Update final position after animation path is set
                        
                        if (newPos == SIZE*SIZE) {
                            strcpy(winnerName, p->name);
                            currentState = STATE_GAME_OVER;
                            // Win! Set message but don't increment turn
                            sprintf(statusMsg, "%s reached 100!", p->name);
                        } else {
                            // No win, move to next player
                            sprintf(statusMsg, "%s rolled %d. pos=%d", p->name, x, newPos);
                            currentTurn++;
                        }
                    }
                }

                // If a player wins during the animation update
                if (!animating && winnerName[0] != '\0') {
                    // Transition to GAME_OVER after any movement animation completes
                    currentState = STATE_GAME_OVER;
                }
                break;
            }
            case STATE_GAME_OVER: {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, rematchBtn)) {
                    // Rematch: keep players/names, reset positions and board
                    InitNewGame(players, np, snakes, ladders, &currentTurn);
                    sprintf(statusMsg, "Rematch started! %s's turn.", players[currentTurn % np].name);
                    // Clear the winner name for the new game
                    winnerName[0] = '\0'; 
                    currentState = STATE_GAMEPLAY;
                }
                break;
            }
        }


        // --- DRAWING ---
        BeginDrawing();
            ClearBackground(RAYWHITE);
            
            if (currentState == STATE_LOAD_OR_NEW) {
                // --- Initial Load or New Game Screen ---
                DrawText("Snake & Ladders (raylib)", 40, 50, 24, GRAY);
                DrawText("Welcome Back!", SCREEN_W/2 - MeasureText("Welcome Back!", 40)/2, SCREEN_H/2 - 100, 40, DARKGRAY);
                DrawText("A saved game was found. Do you want to resume?", SCREEN_W/2 - MeasureText("A saved game was found. Do you want to resume?", 20)/2, SCREEN_H/2 - 20, 20, GRAY);
                
                // Draw Resume Button
                DrawRectangleRec(resumeBtn, GREEN);
                DrawRectangleLinesEx(resumeBtn, 2, DARKGREEN);
                DrawText("Resume Game", resumeBtn.x + 18, resumeBtn.y + 14, 20, WHITE);
                
                // Draw New Game Button
                DrawRectangleRec(newGameBtn, ORANGE);
                DrawRectangleLinesEx(newGameBtn, 2, GOLD);
                DrawText("New Game", newGameBtn.x + 35, newGameBtn.y + 14, 20, WHITE);
                
            } else if (currentState == STATE_INPUT_PLAYERS) {
                // --- Input Player Count Screen ---
                DrawText("Enter Number of Players (2-6)", 40, 20, 30, DARKGRAY);
                
                // Input box
                DrawRectangle(40, 80, 250, 35, LIGHTGRAY);
                char text[2];
                text[0] = (char)(np+48); // the digit itself
                text[1] = '\0';
                DrawText(text, 45, 88, 20, BLACK);
                
                DrawText("Press ENTER to confirm", 40, 80 + 2*50 + 20, 18, GRAY);
                char nplayers[64];   // big enough for the text

                snprintf(nplayers, sizeof(nplayers), "Current: %d players", np);
                DrawText(nplayers, 40, 150, 20, DARKBLUE);
                if (!(np >= 2 && np <= 6)) {
                    DrawText("Number of players should be between 2 and 6", 40, 200, 18, RED);
                }
                
            } else if (currentState == STATE_ENTERING_NAMES) {
                // --- Input Player Names Screen ---
                DrawText("Enter Player Names", 40, 20, 30, DARKGRAY);

                for(int i=0;i<np;i++){
                    Color boxColor = (i == currentTyping) ? YELLOW : LIGHTGRAY;
                    DrawRectangle(40, 80 + i*50, 250, 35, boxColor);
                    DrawText(inputBuffers[i], 45, 88 + i*50, 20, BLACK);
                }

                DrawText("Press ENTER to confirm a name", 40, 80 + np*50 + 20, 18, GRAY);
                DrawText("Press SPACE to START game when all names done", 40, 80 + np*50 + 45, 18, DARKGREEN);
                
            } else { // STATE_GAMEPLAY or STATE_GAME_OVER
                
                // Draw Board and Game elements first
                DrawText("Snake & Ladders (raylib)", 10, 10, 20, GRAY);
                
                int px = SCREEN_W - 200;
                DrawRectangle(px, 20, 180, np*30 + 50, Fade(DARKGRAY, 0.2f));
                DrawText("Players:", px+10, 30, 20, BLACK);

                for(int i=0;i<np;i++){
                    // Highlight current player
                    if (i == (currentTurn % np) && currentState == STATE_GAMEPLAY) {
                        DrawRectangle(px+5, 55 + i*30 - 2, 170, 26, Fade(YELLOW, 0.15f));
                    }

                    DrawRectangle(px+10, 60 + i*30, 18, 18, players[i].color);
                    DrawText(players[i].name, px+35, 60 + i*30, 18, BLACK);
                }

                draw_board(SIZE, cellSize);
                draw_snakes_and_ladders(snakes, ladders, cellSize);

                // draw players (token drawing logic remains the same)
                for (int i = 0; i < np; i++) {
                    int pos = players[i].position;

                    // If this is the animating player, draw interpolated position
                    if (animating && animPlayer == &players[i] && animStepIndex < animPathLen - 1) {
                        Vec2i a = cell_to_pixel(animPath[animStepIndex], cellSize);
                        Vec2i b = cell_to_pixel(animPath[animPathLen - 1], cellSize); // Draw towards final position if animPathLen > 1
                        
                        // Recalculate interpolation if the path has multiple steps
                        Vec2i currentStepStart = cell_to_pixel(animPath[animStepIndex], cellSize);
                        Vec2i currentStepEnd = cell_to_pixel(animPath[animStepIndex + 1], cellSize);

                        float t = smoothstep(animProgress);
                        float x = currentStepStart.x * (1 - t) + currentStepEnd.x * t;
                        float y = currentStepStart.y * (1 - t) + currentStepEnd.y * t;

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
                if (currentState == STATE_GAMEPLAY) {
                    DrawRectangleRec(rollBtn, LIGHTGRAY);
                    DrawRectangleLinesEx(rollBtn, 2, DARKGRAY);
                    DrawText("ROLL (Space)", rollBtn.x + 12, rollBtn.y + 14, 18, BLACK);

                    // dice display above button
                    DrawText(TextFormat("%d", diceDisplay), rollBtn.x + 60, rollBtn.y - 40, 30, BLACK);
                    if (rolling) {
                        DrawText("Rolling...", rollBtn.x + 95, rollBtn.y - 40, 12, DARKGRAY);
                    }
                }
                
                // status
                DrawText(statusMsg, SCREEN_W - 500, SCREEN_H - 40, 16, DARKGRAY);

                // hint keys
                if (currentState == STATE_GAMEPLAY) {
                    DrawText("S=Save  L=Load  R=Restart", 40, SCREEN_H - 40, 14, GRAY);
                }

                // --- Game Over Overlay ---
                if (currentState == STATE_GAME_OVER) {
                    // Semi-transparent overlay
                    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.7f)); 
                    
                    // Winner Text (BIG FONT)
                    const char *winMsg = TextFormat("%s WINS!", winnerName);
                    int fontSize = 60;
                    int textWidth = MeasureText(winMsg, fontSize);
                    DrawText(winMsg, SCREEN_W/2 - textWidth/2, SCREEN_H/2 - 50, fontSize, GOLD);

                    // Rematch Button
                    DrawRectangleRec(rematchBtn, GREEN);
                    DrawRectangleLinesEx(rematchBtn, 2, DARKGREEN);
                    DrawText("Rematch", rematchBtn.x + 35, rematchBtn.y + 14, 20, WHITE);
                }
            }
            
        EndDrawing();
    }

    // AUTO-SAVE on normal close 
    if (currentState == STATE_GAMEPLAY) {
        SaveGame(players, np, currentTurn, snakes, ladders, NULL);
    }
    
    CloseWindow();
    return 0;
}