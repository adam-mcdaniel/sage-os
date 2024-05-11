#include <event.h>
#include <graphics.h>
#include <malloc.h>
#include <sage.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ABS_INT 32767

#define SCREEN_WIDTH (1280)
#define SCREEN_HEIGHT (800)
// #define WIDTH (600)
// #define HEIGHT (600)
#define WIDTH SCREEN_WIDTH
#define HEIGHT SCREEN_HEIGHT
#define BORDER_THICKNESS (10)

#define XO_WIDTH (SCREEN_WIDTH / 6)
#define XO_HEIGHT (SCREEN_HEIGHT / 6)
// #define XO_HEIGHT (100)

#define COL_LIMIT0 (WIDTH / 3)
#define COL_LIMIT1 (2 * WIDTH / 3)
#define COL_LIMIT2 (WIDTH)

#define ROW_LIMIT0 (HEIGHT / 3)
#define ROW_LIMIT1 (2 * HEIGHT / 3)
#define ROW_LIMIT2 (HEIGHT)

#define CELL_X0 (XO_WIDTH / 2) // TODO: Need to account for change in origin
#define CELL_X1 (CELL_X0 + COL_LIMIT0)
#define CELL_X2 (CELL_X0 + COL_LIMIT1)
#define CELL_Y0 (XO_HEIGHT / 2)
#define CELL_Y1 (CELL_Y0 + ROW_LIMIT0)
#define CELL_Y2 (CELL_Y0 + ROW_LIMIT1)

static int cell_x[3];
static int cell_y[3];

static char turn;
static bool filled[9];
static bool which_player[9];
static bool whos_turn;
static bool who_won;

void draw_board(Pixel *buf, Pixel *background_pix, Pixel *border_pix, uint32_t screen_width, uint32_t screen_height) {
  // Background
  Rectangle background_rect = {0, 0, WIDTH, HEIGHT};
  draw_rect(buf, &background_rect, background_pix, screen_width, screen_height);

  // Upper horizontal border
  Rectangle top_border_rect = {0, 0, WIDTH, BORDER_THICKNESS};
  draw_rect(buf, &top_border_rect, border_pix, screen_width, screen_height);

  // Lower horizontal border
  Rectangle bot_border_rect = {0, HEIGHT - BORDER_THICKNESS, WIDTH, BORDER_THICKNESS};
  draw_rect(buf, &bot_border_rect, border_pix, screen_width, screen_height);

  // Left border
  Rectangle left_border_rect = {0, 0, BORDER_THICKNESS, HEIGHT};
  draw_rect(buf, &left_border_rect, border_pix, screen_width, screen_height);

  // Right border
  Rectangle right_border_rect = {WIDTH - BORDER_THICKNESS, 0, BORDER_THICKNESS, HEIGHT};
  draw_rect(buf, &right_border_rect, border_pix, screen_width, screen_height);

  Rectangle vert_mid_border_rect0 = {WIDTH / 3, 0, BORDER_THICKNESS, HEIGHT};
  draw_rect(buf, &vert_mid_border_rect0, border_pix, screen_width, screen_height);

  Rectangle vert_mid_border_rect1 = {2 * WIDTH / 3, 0, BORDER_THICKNESS, HEIGHT};
  draw_rect(buf, &vert_mid_border_rect1, border_pix, screen_width, screen_height);

  Rectangle hor_mid_border_rect0 = {0, HEIGHT / 3, WIDTH, BORDER_THICKNESS};
  draw_rect(buf, &hor_mid_border_rect0, border_pix, screen_width, screen_height);

  Rectangle hor_mid_border_rect1 = {0, 2 * HEIGHT / 3, WIDTH, BORDER_THICKNESS};
  draw_rect(buf, &hor_mid_border_rect1, border_pix, screen_width, screen_height);

  // FIXME
  Rectangle lol = {0, 0, WIDTH, HEIGHT};
  screen_draw_rect(buf, &lol, 1, 1);
  screen_flush(&lol);
}

// void draw_x(Pixel *buf,
//             Rectangle *rect,
//             Pixel *fill_color) {
//   uint32_t top = rect->y;
//   uint32_t left = rect->x;
//   uint32_t bottom = rect->y + rect->height;
//   printf("bottom: %d\n", bottom);
//   // uint32_t right = rect->width;

//   for (uint32_t row = top; row < bottom; ++row) {
//     uint32_t offset0 = (row * XO_WIDTH) + left;
//     // uint32_t offset1 = (row * XO_WIDTH) + right;
//     buf[offset0] = *fill_color;
//     // buf[offset1] = *fill_color;
//     ++left;
//     // --right;
//     // buf[offset0 + 1] = *fill_color;
//     // buf[offset1 - 1] = *fill_color;
//   }

//   Rectangle lol = {0, 0, WIDTH, HEIGHT};
//   screen_draw_rect(buf, &lol, 1, 1);
//   screen_flush(&lol);
// }

void fill_cell(Pixel *buf, Rectangle *rect, bool whos_turn) {
  Pixel fill_color = {0, 255, 0, 0};
  if (whos_turn) {
    fill_color.r = 255;
    fill_color.g = 0;
  }
  draw_rect(buf, rect, &fill_color, WIDTH, HEIGHT);
  Rectangle lol = {0, 0, WIDTH, HEIGHT};
  screen_draw_rect(buf, &lol, 1, 1);
  screen_flush(&lol);
}

void update_cell(int cell, Pixel *buf) {
  printf("Player %d chose cell %d\n", whos_turn, cell);
  
  int x = cell % 3;
  int y = cell / 3;
  Rectangle rect = {cell_x[x], cell_y[y], XO_WIDTH, XO_HEIGHT};
  fill_cell(buf, &rect, whos_turn);

  filled[cell] = true;
  which_player[cell] = whos_turn;
  printf("Cell %d filled by player %d\n", cell, whos_turn);
  whos_turn = !whos_turn;
  printf("Player %d's turn...\n", whos_turn);
  ++turn;
}

// Calculate winning/draw condition, return true if someone won.
bool calc_win() {
  char p0_counter = 0;
  char p1_counter = 0;
  for (int y = 0; y < 9; y += 3) {
    for (int x = y; x < y + 3; ++x) {
      if (filled[x]) {
        p0_counter = which_player[x] == 0 ? p0_counter + 1 : p0_counter;
        p1_counter = which_player[x] == 1 ? p1_counter + 1 : p1_counter;
      }
    }

    if (p0_counter == 3) {
      who_won = 0;
      return true;
    }

    if (p1_counter == 3) {
      who_won = 1;
      return true;
    }

    p0_counter = 0;
    p1_counter = 0;
  }

  for (int i = 0; i < 3; ++i) {
    for (int j = i; j < i + 7; j += 3) {
      if (filled[j]) {
        p0_counter = which_player[j] == 0 ? p0_counter + 1 : p0_counter;
        p1_counter = which_player[j] == 1 ? p1_counter + 1 : p1_counter;
      }
    }

    if (p0_counter == 3) {
      who_won = 0;
      return true;
    }

    if (p1_counter == 3) {
      who_won = 1;
      return true;
    }

    p0_counter = 0;
    p1_counter = 0;
  }

  for (int i = 0; i < 9; i += 4) {
    if (filled[i]) {
      p0_counter = which_player[i] == 0 ? p0_counter + 1 : p0_counter;
      p1_counter = which_player[i] == 1 ? p1_counter + 1 : p1_counter;
    }
  }

  if (p0_counter == 3) {
    who_won = 0;
    return true;
  }

  if (p1_counter == 3) {
    who_won = 1;
    return true;
  }

  p0_counter = 0;
  p1_counter = 0;

  for (int i = 2; i < 7; i += 2) {
    if (filled[i]) {
      p0_counter = which_player[i] == 0 ? p0_counter + 1 : p0_counter;
      p1_counter = which_player[i] == 1 ? p1_counter + 1 : p1_counter;
    }
  }

  if (p0_counter == 3) {
    who_won = 0;
    return true;
  }

  if (p1_counter == 3) {
    who_won = 1;
    return true;
  }

  p0_counter = 0;
  p1_counter = 0;
  
  return false;
}

void annouce_winner() {
  printf("Player %d won!\n", who_won);
  // TODO: Draw a win screen
}

void annouce_draw() {
  printf("It's a draw!\n");
  // TODO: Draw a draw screen
}

int main() {
  // char buf[0x600000] = {0};
  // salloc_init((uint8_t *)buf, (uint8_t *)(buf + sizeof(buf)));
  // printf("~> %p", buf);
  // init_malloc(buf, sizeof(buf));
  // Pixel *pbuf = malloc(1 * 1 * sizeof(Pixel));
  // pbuf[0] = p;
  // free(pbuf);

  // Init cell positions
  cell_x[0] = CELL_X0;
  cell_x[1] = CELL_X1;
  cell_x[2] = CELL_X2;
  cell_y[0] = CELL_Y0;
  cell_y[1] = CELL_Y1;
  cell_y[2] = CELL_Y2;
  
  Rectangle screen_rect;
  screen_get_dims(&screen_rect);

  Pixel buf[WIDTH * HEIGHT] = {0};
  Pixel background_pix = {0, 0, 0, 0}; // Black
  Pixel border_pix = {0, 0, 255, 0}; // Blue
  printf("Printing board...\n");
  draw_board(buf, &background_pix, &border_pix, WIDTH, HEIGHT);
  printf("Printed board!\n");

  VirtioInputEvent tablet_event;
  VirtioInputEvent keyboard_event;
  uint32_t tablet_x = 0;
  uint32_t tablet_y = 0;

  // Main game loop
  while (1) {
    get_tablet_event(&tablet_event);
    if (tablet_event.type == EV_KEY && tablet_event.code == BTN_LEFT && tablet_event.value == 1) {
      printf("HELLO 0x%x\n", tablet_event.value);
      do {
        get_tablet_event(&tablet_event);
        if (tablet_event.type == EV_ABS) {
          if (tablet_event.code == ABS_X)
            tablet_x = screen_rect.width * tablet_event.value / ABS_INT; // Move calc
          if (tablet_event.code == ABS_Y)
            tablet_y = screen_rect.height * tablet_event.value / ABS_INT;
        }
        printf("T: 0x%x, C: 0x%x, V: 0x%x\n", tablet_event.type, tablet_event.code, tablet_event.value);
      } while (tablet_event.type != EV_SYN);
      printf("X: %d, Y: %d\n", tablet_x, tablet_y);

      if (filled[0] == false &&
          tablet_x < ROW_LIMIT0 &&
          tablet_y < COL_LIMIT0) {
        update_cell(0, buf);
      }

      if (filled[1] == false &&
          tablet_x > COL_LIMIT0 &&
          tablet_x < COL_LIMIT1 &&
          tablet_y < ROW_LIMIT0) {
        update_cell(1, buf);
      }

      if (filled[2] == false &&
          tablet_x > COL_LIMIT1 &&
          tablet_x < COL_LIMIT2 &&
          tablet_y < ROW_LIMIT0) {
        update_cell(2, buf);
      }

      if (filled[3] == false &&
          tablet_x < COL_LIMIT0 && 
          tablet_y < ROW_LIMIT1 &&
          tablet_y > ROW_LIMIT0) {
        update_cell(3, buf);
      }

      if (filled[4] == false &&
          tablet_x > COL_LIMIT0 && 
          tablet_x < COL_LIMIT1 &&
          tablet_y > ROW_LIMIT0 &&
          tablet_y < ROW_LIMIT1) {
        update_cell(4, buf);
      }

      if (filled[5] == false &&
          tablet_x > COL_LIMIT1 && 
          tablet_x < COL_LIMIT2 &&
          tablet_y > ROW_LIMIT0 &&
          tablet_y < ROW_LIMIT1) {
        update_cell(5, buf);
      }

      if (filled[6] == false &&
          tablet_x < COL_LIMIT0 && 
          tablet_y > ROW_LIMIT1 &&
          tablet_y < ROW_LIMIT2) {
        update_cell(6, buf);
      }

      if (filled[7] == false &&
          tablet_x > COL_LIMIT0 && 
          tablet_x < COL_LIMIT1 &&
          tablet_y > ROW_LIMIT1 &&
          tablet_y < ROW_LIMIT2) {
        update_cell(7, buf);
      }

      if (filled[8] == false &&
          tablet_x > COL_LIMIT1 && 
          tablet_x < COL_LIMIT2 &&
          tablet_y > ROW_LIMIT1 &&
          tablet_y < ROW_LIMIT2) {
        update_cell(8, buf);
      }
    }

    // Process exit command (ctrl + c)
    get_keyboard_event(&keyboard_event);
    if (keyboard_event.type == EV_KEY && keyboard_event.code == KEY_LEFTCTRL && keyboard_event.value == 1) {
      printf("YO\n");
      printf("T: 0x%x, C: 0x%x, V: 0x%x\n", keyboard_event.type, keyboard_event.code, keyboard_event.value);
      do {
        get_keyboard_event(&keyboard_event);
      } while (keyboard_event.type == EV_SYN);
      printf("T: 0x%x, C: 0x%x, V: 0x%x\n", keyboard_event.type, keyboard_event.code, keyboard_event.value);
      if (keyboard_event.type == EV_KEY && keyboard_event.code == KEY_C) {
        printf("Exiting...\n");
        exit();
      }
    }

    bool won = calc_win();

    if (won)
      annouce_winner();

    if (!won && turn == 9)
      annouce_draw();

    if (won || turn == 9) {
      // Reset game
      turn = 0;
      memset(&filled, 0, sizeof(filled));
      memset(&which_player, 0, sizeof(which_player));
      whos_turn = 0;
      who_won = 0;
  
      Pixel background_pix = {0, 0, 0, 0}; // Black
      Pixel border_pix = {0, 0, 255, 0}; // Blue
      printf("Clearing board...\n");
      draw_board(buf, &background_pix, &border_pix, WIDTH, HEIGHT);
    }

    // sleep(10);
  }
}
