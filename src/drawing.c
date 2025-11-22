#include <stdio.h>
#include <math.h>
#include "raylib.h"
#include "../include/s_and_l.h"

// Screen dimensions (defined in main.c scope, but hardcoded here for drawing geometry)
#define SCREEN_W 1000
#define SCREEN_H 800
#define BOARD_MARGIN 40

// Helper: smoothstep easing (USED in draw_players for animation interpolation)
static float smoothstep(float x) {
    if (x < 0) return 0;
    if (x > 1) return 1;
    return x*x*(3 - 2*x);
}

// Helper: Vector normalization
static Vector2 NormalizeVec(Vector2 v) {
    float len = sqrtf(v.x*v.x + v.y*v.y);
    if (len == 0) return (Vector2){0,0};
    return (Vector2){v.x/len, v.y/len};
}

// Converts a board cell number (1 to SIZE*SIZE) to pixel coordinates
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

// Draws the main grid and cell numbers
void draw_board(int size, int cellSize) {
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

// Draws the snakes and ladders connections
void draw_snakes_and_ladders(Snake snakes[], Ladder lads[], int cellSize){
    for(int i=0;i<N_LADDERS;i++){
        Vec2i b = cell_to_pixel(lads[i].bottom, cellSize);
        Vec2i t = cell_to_pixel(lads[i].top, cellSize);

        DrawLineEx((Vector2){b.x,b.y},(Vector2){t.x,t.y},6.0f, DARKGREEN);

        // Draw simple rungs 
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

// Draws the player tokens on the board
void draw_players(Player players[], int np, int cellSize) {
    for (int i = 0; i < np; i++) {
        int pos = players[i].position;
        float x, y;

        // 1. Handle Animation (Interpolated position)
        if (animating && animPlayer == &players[i] && animStepIndex < animPathLen - 1) {
            Vec2i currentStepStart = cell_to_pixel(animPath[animStepIndex], cellSize);
            Vec2i currentStepEnd = cell_to_pixel(animPath[animStepIndex + 1], cellSize);
            
            // Use smoothstep for the interpolation factor
            float t = smoothstep(animProgress); 
            
            // Recalculate interpolation between current steps
            x = currentStepStart.x * (1 - t) + currentStepEnd.x * t;
            y = currentStepStart.y * (1 - t) + currentStepEnd.y * t;
        } 
        // 2. Handle Static Position (Non-animated or off-board)
        else { 
            if (pos == 0) {
                // Off-board area for tokens: arrange horizontally at the bottom left
                int baseX = BOARD_MARGIN/2 + 10;
                int baseY = SCREEN_H - BOARD_MARGIN/2 - 10;
                int spacing = 22;
                x = baseX + i*spacing;
                y = baseY;
                DrawCircle(x, y, 14, players[i].color);
                continue; // Skip offset for off-board
            } else {
                Vec2i pxy = cell_to_pixel(pos, cellSize);
                x = pxy.x;
                y = pxy.y;
            }
        }
        
        // Add small offsets for players on the same cell (3x2 grid)
        int tokenSpacing = cellSize / 6;
        if (tokenSpacing < 10) tokenSpacing = 10;
        int col = i % 3;
        int row = i / 3;
        int dx = (col - 1) * tokenSpacing;
        int dy = (row - 1) * (tokenSpacing/1.2);

        DrawCircle(x + dx, y + dy, 12, players[i].color);
    }
}

// Draws the player list and turn indicator
void draw_ui_panel(Player players[], int np, int currentTurn) {
    int px = SCREEN_W - 200;
    DrawRectangle(px, 20, 180, np*30 + 50, Fade(DARKGRAY, 0.2f));
    DrawText("Players:", px+10, 30, 20, BLACK);

    for(int i=0;i<np;i++){
        // Highlight current player
        if (i == (currentTurn % np) && !animating) {
            DrawRectangle(px+5, 55 + i*30 - 2, 170, 26, Fade(YELLOW, 0.15f));
        }

        DrawRectangle(px+10, 60 + i*30, 18, 18, players[i].color);
        DrawText(players[i].name, px+35, 60 + i*30, 18, BLACK);
    }
}