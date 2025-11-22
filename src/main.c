#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "raylib.h"
#include "../include/s_and_l.h"

// Screen dimensions
#define SCREEN_W 1000
#define SCREEN_H 800
#define BOARD_MARGIN 40

// Removed local extern declarations for rolling variables as they are in s_and_l.h

int main(void) {
    srand(time(NULL));
    InitWindow(SCREEN_W, SCREEN_H, "Snake & Ladders");
    SetTargetFPS(60);

    // --- Game state variables ---
    int np = 2; // Default starting minimum
    int currentTurn = 0;
    Player players[MAX_PLAYERS];
    Snake snakes[N_SNAKES];
    Ladder ladders[N_LADDERS];
    
    // --- UI variables ---
    int cellSize = (SCREEN_H - 2*BOARD_MARGIN) / SIZE;
    Rectangle rollBtn = { SCREEN_W - 180, SCREEN_H - 80, 150, 50 };
    Rectangle rematchBtn = { SCREEN_W/2 - 75, SCREEN_H/2 + 50, 150, 50 };
    Rectangle resumeBtn = { SCREEN_W/2 - 160, SCREEN_H/2 + 50, 150, 50 };
    Rectangle newGameBtn = { SCREEN_W/2 + 10, SCREEN_H/2 + 50, 150, 50 };

    int diceDisplay = 1;      // What the dice shows
    
    char statusMsg[200] = "Welcome to Snakes & Ladders!";
    GameState currentState = STATE_LOAD_OR_NEW;
    char winnerName[20] = "";


    // --- Main Game Loop ---
    while (!WindowShouldClose()) {

        // --- UPDATE LOGIC (STATE MACHINE) ---
        Vector2 mouse = GetMousePosition();
        
        // Rolling animation progress and turn handling
        if (currentState == STATE_GAMEPLAY) {
            
            // Player movement animation update
            if (animating) {
                animProgress += GetFrameTime() / animDuration;
                if (animProgress >= 1.0f) {
                    animProgress = 0.0f;
                    animStepIndex++;
                    if (animStepIndex >= animPathLen - 1) {
                        animating = 0; // animation finished
                    }
                }
            }
            
            // Dice rolling animation/trigger
            if (rolling) {
                rollingTimer += GetFrameTime();
                if ((int)(rollingTimer * 20) % 2 == 0) diceDisplay = roll(); // Animate dice
                
                if (rollingTimer >= rollingDuration) {
                    rolling = 0;
                    diceDisplay = roll(); // Final dice result

                    // Perform actual move and check for S/L
                    Player *p = &players[currentTurn % np];
                    int oldPos = p->position;
                    int diceResult = diceDisplay;
                    int newPos = oldPos;
                    
                    if (diceResult + oldPos <= (SIZE*SIZE)) newPos += diceResult;
                    
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
                    p->position = newPos; 
                    
                    if (newPos == SIZE*SIZE) {
                        strcpy(winnerName, p->name);
                        currentState = STATE_GAME_OVER;
                        sprintf(statusMsg, "%s reached 100!", p->name);
                    } else {
                        sprintf(statusMsg, "%s rolled %d. pos=%d", p->name, diceResult, newPos);
                        currentTurn++;
                    }
                }
            }
            
            // Input: click roll button or space
            if (!animating && !rolling) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, rollBtn)) {
                    rolling = 1;
                    rollingTimer = 0.0f;
                }
                if (IsKeyPressed(KEY_SPACE)) {
                    rolling = 1;
                    rollingTimer = 0.0f;
                }
            }
            
            // Save/Load/Restart keys
            if (IsKeyPressed(KEY_S)) SaveGame(players, np, currentTurn, snakes, ladders, statusMsg);
            if (IsKeyPressed(KEY_L)) { 
                if (LoadGame(players, &np, &currentTurn, snakes, ladders, statusMsg)) animating = 0;
            }
            if (IsKeyPressed(KEY_R)) {
                InitNewGame(players, np, snakes, ladders, &currentTurn);
                sprintf(statusMsg, "Game restarted. %s's turn.", players[currentTurn % np].name);
            }
        }
        
        // State-specific transitions and logic
        switch (currentState) {
            case STATE_LOAD_OR_NEW: {
                if (CheckSaveFileExists(SAVE_FILE_NAME)) {
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (CheckCollisionPointRec(mouse, resumeBtn)) {
                            if (LoadGame(players, &np, &currentTurn, snakes, ladders, statusMsg)) {
                                currentState = STATE_GAMEPLAY;
                            } else {
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
                int key = GetKeyPressed();
                if (key >= 50 && key <= 54) { // 2 to 6
                    np = key - 48;
                }
                else if (IsKeyPressed(KEY_ENTER) && np >= 2 && np <= 6) {
                    for(int i=0;i<np;i++){
                        players[i].position = 0;
                        players[i].id = i+1;
                        players[i].color = palette[i % PALETTE_SIZE]; 
                        strcpy(players[i].name, "");
                    }
                    currentTyping = 0;
                    currentState = STATE_ENTERING_NAMES;
                }
                break;
            }
            case STATE_ENTERING_NAMES: {
                int key = GetKeyPressed();
                if (key >= 32 && key <= 126) {
                    int len = (int)strlen(inputBuffers[currentTyping]);
                    if (len < 19) {
                        inputBuffers[currentTyping][len] = key;
                        inputBuffers[currentTyping][len+1] = '\0';
                    }
                }
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    int len = (int)strlen(inputBuffers[currentTyping]);
                    if (len > 0) inputBuffers[currentTyping][len-1] = '\0';
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    if (strlen(inputBuffers[currentTyping]) > 0) {
                        strcpy(players[currentTyping].name, inputBuffers[currentTyping]);
                        currentTyping = (currentTyping+1) % np;
                    }
                }
                if (IsKeyPressed(KEY_SPACE)) {
                    int allFilled = 1;
                    for(int i=0;i<np;i++) {
                         if (strlen(inputBuffers[i]) == 0) allFilled = 0;
                         else strcpy(players[i].name, inputBuffers[i]);
                    }
                    if (allFilled) {
                        InitNewGame(players, np, snakes, ladders, &currentTurn);
                        currentState = STATE_GAMEPLAY;
                    } else {
                        sprintf(statusMsg, "All player names must be entered before starting!");
                    }
                }
                break;
            }
            case STATE_GAME_OVER: {
                // Delete save file since the game is over
                DeleteSaveFile(SAVE_FILE_NAME);

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, rematchBtn)) {
                    InitNewGame(players, np, snakes, ladders, &currentTurn);
                    sprintf(statusMsg, "Rematch started! %s's turn.", players[currentTurn % np].name);
                    winnerName[0] = '\0'; 
                    currentState = STATE_GAMEPLAY;
                }
                break;
            }
             case STATE_GAMEPLAY:
                // Game logic handled before switch
                break;
        }


        // --- DRAWING ---
        BeginDrawing();
            ClearBackground(RAYWHITE);
            
            if (currentState == STATE_LOAD_OR_NEW) {
                // Initial Load or New Game Screen
                DrawText("Snake & Ladders (raylib)", 40, 50, 24, GRAY);
                DrawText("Welcome Back!", SCREEN_W/2 - MeasureText("Welcome Back!", 40)/2, SCREEN_H/2 - 100, 40, DARKGRAY);
                DrawText("A saved game was found. Do you want to resume?", SCREEN_W/2 - MeasureText("A saved game was found. Do you want to resume?", 20)/2, SCREEN_H/2 - 20, 20, GRAY);
                
                DrawRectangleRec(resumeBtn, GREEN); DrawRectangleLinesEx(resumeBtn, 2, DARKGREEN);
                DrawText("Resume Game", resumeBtn.x + 18, resumeBtn.y + 14, 20, WHITE);
                
                DrawRectangleRec(newGameBtn, ORANGE); DrawRectangleLinesEx(newGameBtn, 2, GOLD);
                DrawText("New Game", newGameBtn.x + 35, newGameBtn.y + 14, 20, WHITE);
                
            } else if (currentState == STATE_INPUT_PLAYERS) {
                // Input Player Count Screen
                DrawText("Enter Number of Players (2-6)", 40, 20, 30, DARKGRAY);
                DrawRectangle(40, 80, 250, 35, LIGHTGRAY);
                char text[2]; text[0] = (char)(np+48); text[1] = '\0';
                DrawText(text, 45, 88, 20, BLACK);
                
                DrawText("Press ENTER to confirm", 40, 80 + 2*50 + 20, 18, GRAY);
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "Current: %d players", np);
                DrawText(buffer, 40, 150, 20, DARKBLUE);
                if (!(np >= 2 && np <= 6)) DrawText("Number of players should be between 2 and 6", 40, 200, 18, RED);
                
            } else if (currentState == STATE_ENTERING_NAMES) {
                // Input Player Names Screen
                DrawText("Enter Player Names", 40, 20, 30, DARKGRAY);

                for(int i=0;i<np;i++){
                    Color boxColor = (i == currentTyping) ? YELLOW : LIGHTGRAY;
                    DrawRectangle(40, 80 + i*50, 250, 35, boxColor);
                    DrawText(inputBuffers[i], 45, 88 + i*50, 20, BLACK);
                }

                DrawText("Press ENTER to confirm a name", 40, 80 + np*50 + 20, 18, GRAY);
                DrawText("Press SPACE to START game when all names done", 40, 80 + np*50 + 45, 18, DARKGREEN);
                
            } else { // STATE_GAMEPLAY or STATE_GAME_OVER
                
                DrawText("Snake & Ladders (raylib)", 10, 10, 20, GRAY);
                draw_ui_panel(players, np, currentTurn);

                draw_board(SIZE, cellSize);
                draw_snakes_and_ladders(snakes, ladders, cellSize);
                draw_players(players, np, cellSize);

                // Roll button
                if (currentState == STATE_GAMEPLAY) {
                    DrawRectangleRec(rollBtn, LIGHTGRAY); DrawRectangleLinesEx(rollBtn, 2, DARKGRAY);
                    DrawText("ROLL (Space)", rollBtn.x + 12, rollBtn.y + 14, 18, BLACK);

                    DrawText(TextFormat("%d", diceDisplay), rollBtn.x + 60, rollBtn.y - 40, 30, BLACK);
                    if (rolling) DrawText("Rolling...", rollBtn.x + 95, rollBtn.y - 40, 12, DARKGRAY);
                }
                
                // status
                DrawText(statusMsg, SCREEN_W - 500, SCREEN_H - 40, 16, DARKGRAY);
                if (currentState == STATE_GAMEPLAY) DrawText("S=Save  L=Load  R=Restart", 40, SCREEN_H - 40, 14, GRAY);

                // Game Over Overlay
                if (currentState == STATE_GAME_OVER) {
                    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.7f)); 
                    
                    const char *winMsg = TextFormat("%s WINS!", winnerName);
                    int fontSize = 60;
                    int textWidth = MeasureText(winMsg, fontSize);
                    DrawText(winMsg, SCREEN_W/2 - textWidth/2, SCREEN_H/2 - 50, fontSize, GOLD);

                    DrawRectangleRec(rematchBtn, GREEN); DrawRectangleLinesEx(rematchBtn, 2, DARKGREEN);
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