#include <stdio.h>
#include <rand.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <printf.h>
#include <event.h>
#include <graphics.h>
#include <sage.h>
#include <stdint.h>

#define GRID_ROWS 20
#define GRID_COLS 10
#define CELL_WIDTH 30
#define CELL_HEIGHT 30
#define MAX_SHAPE_SIZE 4
#define TRUE 1
#define FALSE 0

void drawGameGrid(Pixel *buf, uint32_t screen_width, uint32_t screen_height) {
    Pixel gridColor = {255, 255, 255, 0}; // White color for grid lines
    for (int i = 0; i < GRID_ROWS; ++i) {
        for (int j = 0; j < GRID_COLS; ++j) {
            Rectangle cell = {
                .x = j * CELL_WIDTH,
                .y = i * CELL_HEIGHT,
                .width = CELL_WIDTH,
                .height = CELL_HEIGHT
            };
            draw_rect(buf, &cell, &gridColor, screen_width, screen_height);
        }
    }
    // Flush the buffer to update the screen
    Rectangle fullScreen = {0, 0, screen_width, screen_height};
    screen_flush(&fullScreen);
}

typedef struct {
    char array[MAX_SHAPE_SIZE][MAX_SHAPE_SIZE];
    int width, row, col;
} Shape;

char GameGrid[GRID_ROWS][GRID_COLS] = {0};
int gameScore = 0;
char GameActive = TRUE;
Shape current;

typedef int64_t suseconds_t;
suseconds_t timer = 400000;
int decrease = 1000;

Shape ShapesArray[7] = {
    {{{0,1,1},{1,1,0},{0,0,0},{0,0,0}}, 3, 0, 0},
    {{{1,1,0},{0,1,1},{0,0,0},{0,0,0}}, 3, 0, 0},
    {{{0,1,0},{1,1,1},{0,0,0},{0,0,0}}, 3, 0, 0},
    {{{0,0,1},{1,1,1},{0,0,0},{0,0,0}}, 3, 0, 0},
    {{{1,0,0},{1,1,1},{0,0,0},{0,0,0}}, 3, 0, 0},
    {{{1,1},{1,1},{0,0},{0,0}}, 2, 0, 0},
    {{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, 4, 0, 0}
};

void drawTetrisShape(Pixel *buf, Shape *shape, Pixel *color, uint32_t screen_width, uint32_t screen_height) {
    for (int i = 0; i < shape->width; i++) {
        for (int j = 0; j < shape->width; j++) {
            if (shape->array[i][j]) {
                Rectangle cell = {
                    .x = (shape->col + j) * CELL_WIDTH,
                    .y = (shape->row + i) * CELL_HEIGHT,
                    .width = CELL_WIDTH,
                    .height = CELL_HEIGHT
                };
                draw_rect(buf, &cell, color, screen_width, screen_height);
            }
        }
    }
}

void clearScreen(Pixel *buf, Rectangle *screen_rect, uint32_t screen_width, uint32_t screen_height) {
    Pixel backgroundColor = {0, 0, 0, 0}; // Black color for background
    Rectangle fullScreenRect = {0, 0, screen_width, screen_height};
    draw_rect(buf, &fullScreenRect, &backgroundColor, screen_width, screen_height);
    screen_flush(screen_rect);
}

void CopyShape(Shape *dest, Shape *src) {
    dest->width = src->width;
    dest->row = src->row;
    dest->col = src->col;
    for (int i = 0; i < MAX_SHAPE_SIZE; i++) {
        for (int j = 0; j < MAX_SHAPE_SIZE; j++) {
            dest->array[i][j] = src->array[i][j];
        }
    }
}

int CheckPos(Shape *shape){
    for (int i = 0; i < shape->width; i++) {
        for (int j = 0; j < shape->width; j++) {
            if (shape->array[i][j]) {
                int x = shape->col + j;
                int y = shape->row + i;
                if (x < 0 || x >= GRID_COLS || y >= GRID_ROWS || GameGrid[y][x])
                    return FALSE;
            }
        }
    }
    return TRUE;
}

void SetRandShape(){
    int idx = rand() % 7;
    CopyShape(&current, &ShapesArray[idx]);
    current.col = rand() % (GRID_COLS - current.width + 1);
    current.row = 0;
    if (!CheckPos(&current)) {
        GameActive = FALSE;
    }
}

void RotShape(Shape *shape){
    Shape temp;
    CopyShape(&temp, shape);
    for (int i = 0; i < shape->width; i++) {
        for (int j = 0, k = shape->width - 1; j < shape->width; j++, k--) {
            shape->array[i][j] = temp.array[k][i];
        }
    }
}

void WriteToGameGrid(){
    for (int i = 0; i < current.width; i++) {
        for (int j = 0; j < current.width; j++) {
            if (current.array[i][j]) {
                GameGrid[current.row + i][current.col + j] = current.array[i][j];
            }
        }
    }
}

void UpdateGameScore(){
	int i, j, sum, count=0;
	for(i=0;i<GRID_ROWS;i++){
		sum = 0;
		for(j=0;j< GRID_COLS;j++) {
			sum+=GameGrid[i][j];
		}
		if(sum==GRID_COLS){
			count++;
			int l, k;
			for(k = i;k >=1;k--)
				for(l=0;l<GRID_COLS;l++)
					GameGrid[k][l]=GameGrid[k-1][l];
			for(l=0;l<GRID_COLS;l++)
				GameGrid[k][l]=0;
			timer-=decrease--;
		}
	}
	gameScore += 100*count;
}

void PrintGameGrid(){
    char Buffer[GRID_ROWS][GRID_COLS] = {0};
    int i, j;
    for(i = 0; i < current.width ;i++){
        for(j = 0; j < current.width ; j++){
            if(current.array[i][j])
                Buffer[current.row+i][current.col+j] = current.array[i][j];
        }
    }
    
    for(i = 0; i < 50; i++) printf("\n");

    for(i=0; i<GRID_COLS-9; i++)
        printf(" ");
    printf("Simple Tetris by Tokey\n");
    for(i = 0; i < GRID_ROWS ;i++){
        for(j = 0; j < GRID_COLS ; j++){
            printf("%c ", (GameGrid[i][j] + Buffer[i][j])? '#': '.');
        }
        printf("\n");
    }
    printf("\nYour Score: %d\n", gameScore);
}

void ProcessInput() {
    struct virtio_input_event event;

    while (get_events(&event, 1)) {
        // Check if the event is a key press
        if (event.type == EV_KEY) {
            // Handle different keys
            switch (event.code) {
                case KEY_UP:
                    // Handle up arrow key
                    break;
                case KEY_DOWN:
                    // Handle down arrow key
                    break;
            }
        }
    }
}

void UpdateCurrent(int action){
	Shape temp;
    CopyShape(&temp, &current);
	switch(action){
		case 's':
			temp.row++;
			if(CheckPos(&temp))
				current.row++;
			else {
				WriteToGameGrid();
				UpdateGameScore();
                SetRandShape();
			}
			break;
		case 'd':
			temp.col++;
			if(CheckPos(&temp))
				current.col++;
			break;
		case 'a':
			temp.col--;
			if(CheckPos(&temp))
				current.col--;
			break;
		case 'w':
			RotShape(&temp); 
			if(CheckPos(&temp))
				RotShape(&current);
			break;
	}
	PrintGameGrid();
}

int main() {
    srand(0); // Initialize random number generator
    gameScore = 0; // Reset gameScore
    Rectangle screen_rect;
    screen_get_dims(&screen_rect);

    Pixel buf[screen_rect.width * screen_rect.height];
    // Clear the screen
    clearScreen(buf, &screen_rect, screen_rect.width, screen_rect.height);

    drawGameGrid(buf, screen_rect.width, screen_rect.height);
    
    SetRandShape(); // Set initial random shape
    PrintGameGrid(); // Initial print of the GameGrid

    timer = 0;

    while(GameActive){
        ProcessInput(); // Process keyboard events

        if (timer++ % 100 == 0) { // Update every 100 iterations, adjust as needed
            UpdateCurrent('s'); // Automatically move the shape down
        }
    }

    // Draw game grid
    drawGameGrid(buf, screen_rect.width, screen_rect.height);

    // Draw current Tetris shape
    Pixel shapeColor = {255, 0, 0, 0}; // Red color for Tetris shape
    drawTetrisShape(buf, &current, &shapeColor, screen_rect.width, screen_rect.height);

    // Flush the buffer to update the screen
    screen_flush(&screen_rect);

    // Final print of the game state
    int i, j;
    for(i = 0; i < GRID_ROWS; i++){
        for(j = 0; j < GRID_COLS; j++){
            printf("%c ", GameGrid[i][j] ? '$' : '*');
        }
        printf("\n");
    }
    printf("\nGAME OVER!!!\n");
    printf("\nYour Score: %d\n", gameScore);

    return 0;
}