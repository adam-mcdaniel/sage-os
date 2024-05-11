#include "graphics.h"
#include <stdint.h>

// Draw a rect->width * rect->height rectangle
void draw_rect(Pixel *buf,
               Rectangle *rect,
               Pixel *fill_color,
               uint32_t screen_width,
               uint32_t screen_height) {
  uint32_t left = rect->x;
  uint32_t right = rect->x + rect->width;
  uint32_t top = rect->y;
  uint32_t bottom = rect->y + rect->height;
  
  if (bottom > screen_height) {
    bottom = screen_height;
  }

  if (right > screen_width) {
    right = screen_width;
  }
  
  for (uint32_t row = top; row < bottom; ++row) {
    for (uint32_t col = left; col < right; ++col) {
      uint32_t offset = row * screen_width + col;
      buf[offset] = *fill_color;
    }
  }
}
