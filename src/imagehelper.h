#ifndef IMAGEHELPER_H_
#define IMAGEHELPER_H_

#include <CImg.h>

#include "util.h"
#include "colour.h"
#include "fonts/due_font.h"

namespace DueUtil::Images {
  using namespace cimg_library;

  void draw_text(
    CImg<uint8_t> & canvas,
    int x, int y,
    std::u32string text,
    DueFont& font,
    int size,
    Colour const & colour = WHITE,
    int max_len = -1,
    int stroke = 0,
    Colour const & stroke_colour = WHITE);

  inline void draw_text(
    CImg<uint8_t> & canvas,
    int x, int y,
    std::string text,
    DueFont& font,
    int size,
    Colour const & colour = WHITE,
    int max_len = -1,
    int stroke = 0,
    Colour const & stroke_colour = WHITE
  ) {
    draw_text(canvas,
      x, y, Util::to_utf32(text), font, size, colour, max_len, stroke, stroke_colour);
  }

  int text_width(
    std::u32string text,
    DueFont& font,
    int size);

  int text_width(
    std::string text,
    DueFont& font,
    int size);
}

#endif
