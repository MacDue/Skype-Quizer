
#include "util.h"
#include "imagehelper.h"
#include "fonts/cimg_freetype.h"

namespace DueUtil::Images {
  int Colour::luminance() const {
    return rgb[0] * 0.299 + rgb[1] * 0.587 + rgb[2] * 0.114;
  }

  int32_t Colour::to_int() {
    int red   = (this->rgb[0] << 16) & 0x00FF0000;
    int green = (this->rgb[1] << 8) & 0x0000FF00;
    int blue  = this->rgb[2] & 0x000000FF;
    return red | green | blue;
  }

  uint8_t * Colour::as_bytes(uint8_t bytes[]) const {
    bytes[0] = static_cast<uint8_t>(this->rgb[0]);
    bytes[1] = static_cast<uint8_t>(this->rgb[1]);
    bytes[2] = static_cast<uint8_t>(this->rgb[2]);
    bytes[3] = 255;
    return bytes;
  }

  int get_text_limit_length(
    std::u32string& str, FT_Face fontface, int size, int max_width
  ) {
    bool removed_chars = false;
    int ellipsis_width = textWidth(fontface, size, U"…");

    int width = 0;
    size_t i;
    max_width = std::max<size_t>(0, max_width - ellipsis_width);
    for (i = 0; i < str.length(); i++) {
      int new_width = width + charWidth(fontface, size, str[i]);
      if (new_width > max_width) {
        removed_chars = true;
        break;
      }
      width = new_width;
    }
    if (removed_chars) {
      str.resize(i);
      str += U"…";
      width += ellipsis_width;
    }
    return width;
  }

  void draw_text(
    CImg<uint8_t> & canvas,
    int x, int y,
    std::u32string text,
    DueFont& font,
    int size,
    Colour const & colour,
    int max_len,
    int stroke,
    Colour const & stroke_colour
  ) {
    uint8_t colour_bytes[4];
    uint8_t stroke_colour_bytes[4];
    stroke_colour.as_bytes(stroke_colour_bytes);
    if (max_len > 0) {
      (void) get_text_limit_length(text, font.face(), size, max_len);
    }
    drawText(
      font.face(),
      canvas,
      size,
      text,
      x, y + size - 1,
      colour.as_bytes(colour_bytes),
      stroke > 0 ? font.stroker(stroke) : nullptr,
      stroke_colour_bytes, 0);
  }

  int text_width(
    std::u32string text,
    DueFont& font,
    int size
  ) {
    return textWidth(font.face(), size, text);
  }

  int text_width(
    std::string text,
    DueFont& font,
    int size
  ) {
    return text_width(Util::to_utf32(text), font, size);
  }
}
