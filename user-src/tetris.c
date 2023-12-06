#include <stdio.h>
#include <malloc.h>
#include <rand.h>
#include <ctype.h>
#include <string.h>
#include <stat.h>
#include <unistd.h>
#include <printf.h>
#include <event.h>
#include <sage.h>
#include <stdint.h>


typedef int64_t suseconds_t;

#define GRID_ROWS 20
#define GRID_COLS 15
#define TRUE 1
#define FALSE 0

char GameGrid[GRID_ROWS][GRID_COLS] = {0};
int gameScore = 0;
char GameActive = TRUE;
suseconds_t timer = 400000;
int decrease = 1000;

typedef struct {
    char **array;
    int width, row, col;
} Shape;
Shape current;

const Shape ShapesArray[7]= {
	{(char *[]){(char []){0,1,1},(char []){1,1,0}, (char []){0,0,0}}, 3, 0, 0},    
	{(char *[]){(char []){1,1,0},(char []){0,1,1}, (char []){0,0,0}}, 3, 0, 0},  
	{(char *[]){(char []){0,1,0},(char []){1,1,1}, (char []){0,0,0}}, 3, 0, 0},    
	{(char *[]){(char []){0,0,1},(char []){1,1,1}, (char []){0,0,0}}, 3, 0, 0},    
	{(char *[]){(char []){1,0,0},(char []){1,1,1}, (char []){0,0,0}}, 3, 0, 0},  
	{(char *[]){(char []){1,1},(char []){1,1}}, 2, 0, 0},
	{(char *[]){(char []){0,0,0,0}, (char []){1,1,1,1}, (char []){0,0,0,0}, (char []){0,0,0,0}}, 4, 0, 0}
};

Shape CopyShape(Shape shape){
	Shape new_shape = shape;
	char **copyshape = shape.array;
	new_shape.array = (char**)malloc(new_shape.width*sizeof(char*));
    int i, j;
    for(i = 0; i < new_shape.width; i++){
		new_shape.array[i] = (char*)malloc(new_shape.width*sizeof(char));
		for(j=0; j < new_shape.width; j++) {
			new_shape.array[i][j] = copyshape[i][j];
		}
    }
    return new_shape;
}

void DelShape(Shape shape){
    int i;
    for(i = 0; i < shape.width; i++){
		free(shape.array[i]);
    }
    free(shape.array);
}

int CheckPos(Shape shape){
	char **array = shape.array;
	int i, j;
	for(i = 0; i < shape.width;i++) {
		for(j = 0; j < shape.width ;j++){
			if((shape.col+j < 0 || shape.col+j >= GRID_COLS || shape.row+i >= GRID_ROWS)){
				if(array[i][j])
					return FALSE;
				
			}
			else if(GameGrid[shape.row+i][shape.col+j] && array[i][j])
				return FALSE;
		}
	}
	return TRUE;
}

void SetRandShape(){
	Shape new_shape = CopyShape(ShapesArray[rand()%7]);

    new_shape.col = rand()%(GRID_COLS-new_shape.width+1);
    new_shape.row = 0;
    DelShape(current);
	current = new_shape;
	if(!CheckPos(current)){
		GameActive = FALSE;
	}
}

void RotShape(Shape shape){
	Shape temp = CopyShape(shape);
	int i, j, k, width;
	width = shape.width;
	for(i = 0; i < width ; i++){
		for(j = 0, k = width-1; j < width ; j++, k--){
				shape.array[i][j] = temp.array[k][i];
		}
	}
	DelShape(temp);
}

void WriteToGameGrid(){
	int i, j;
	for(i = 0; i < current.width ;i++){
		for(j = 0; j < current.width ; j++){
			if(current.array[i][j])
				GameGrid[current.row+i][current.col+j] = current.array[i][j];
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
	Shape temp = CopyShape(current);
	switch(action){
		case 's':
			temp.row++;
			if(CheckPos(temp))
				current.row++;
			else {
				WriteToGameGrid();
				UpdateGameScore();
                SetRandShape();
			}
			break;
		case 'd':
			temp.col++;
			if(CheckPos(temp))
				current.col++;
			break;
		case 'a':
			temp.col--;
			if(CheckPos(temp))
				current.col--;
			break;
		case 'w':
			RotShape(temp); 
			if(CheckPos(temp))
				RotShape(current);
			break;
	}
	DelShape(temp);
	PrintGameGrid();
}

int main() {
    srand(0); // Initialize random number generator
    gameScore = 0; // Reset gameScore
    SetRandShape(); // Set initial random shape
    PrintGameGrid(); // Initial print of the GameGrid

    timer = 0;

    while(GameActive){
        ProcessInput(); // Process keyboard events

        if (timer++ % 100 == 0) { // Update every 100 iterations, adjust as needed
            UpdateCurrent('s'); // Automatically move the shape down
        }
    }

    DelShape(current); // Clean up the current shape

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