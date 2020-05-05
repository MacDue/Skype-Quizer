#include <exception>
#include "fonts/cimg_freetype.h"

#define tfpos2int(pos) ((pos) >> 6)
#define FT_THROW(msg) throw std::runtime_error(msg);

/*
 libtruetype for cimg from https://github.com/tttzof351/cimg-and-freetype
 */
void initFreetype(
  FT_Library& ftlib,
  FT_Face& face,
  std::string const & fontFullName
) {
  FT_Error fterr;
  if ((fterr = FT_Init_FreeType(&ftlib))) {
    FT_THROW("error init freetype");
  }

  if ((fterr = FT_New_Face(ftlib, fontFullName.c_str(), 0, &face))) {
    if (fterr == FT_Err_Unknown_File_Format) {
      FT_THROW("unsupported font");
    } else {
      FT_THROW("can't create new face");;
    }
  }
}

void closeFreetype(
  FT_Library ftlib,
  FT_Face face
) {
  FT_Done_Face(face);
  FT_Done_FreeType(ftlib);
}

void drawBitmap(
  FT_Bitmap& bitmap,
  cimg_t& image,
  int shiftX,
  int shiftY,
  uint8_t fontColor[] = nullptr
) {
  uint8_t white[] = {255, 255, 255, 255};
  if (fontColor == nullptr){
    fontColor = white;
  }

  for (unsigned int y = 0; y < bitmap.rows; ++y) {
    for (unsigned int x = 0; x < bitmap.width; ++x) {
      uint8_t glyphValue = bitmap.buffer[y * bitmap.width + x];
      float alpha = ((255 - glyphValue) / 255.0f);
      cimg_forC(image, c){
        uint8_t image_val = image(x + shiftX, y + shiftY, c);
          image(x + shiftX, y + shiftY, c) = static_cast<uint8_t>(
            image_val * alpha + fontColor[c] * (1.0 - alpha));
      }
    }
  }
}

void drawGlyph(
  FT_GlyphSlot glyphSlot,
  cimg_t& image,
  int shiftX,
  int shiftY,
  uint8_t fontColor[] = nullptr,
  FT_Stroker stroker = nullptr,
  uint8_t strokeColor[] = nullptr
) {
  shiftY -= glyphSlot->bitmap_top;
  shiftX += glyphSlot->bitmap_left;
  if (stroker != nullptr) {
    FT_Glyph glyph;
    if (FT_Get_Glyph(glyphSlot, &glyph) != 0) {
      FT_THROW("can't get glyph");
    }
    if (glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
      FT_THROW("non-outline glyph (can't stroke)");
    }
    if (FT_Glyph_StrokeBorder(&glyph, stroker, false, true) != 0) {
      FT_THROW("stroke failed");
    }
    if (FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, nullptr, true) != 0) {
      FT_THROW("bitmap failed");
    }
    FT_BitmapGlyph bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
    /* TODO: Don't hardcode banner shift */
    drawBitmap(bitmapGlyph->bitmap, image, shiftX-1, shiftY-1, strokeColor);
    FT_Done_Glyph(glyph);
  }
  if (FT_Render_Glyph(glyphSlot, FT_RENDER_MODE_NORMAL) != 0) {
    FT_THROW("render failed");
  }
  drawBitmap(glyphSlot->bitmap, image, shiftX, shiftY, fontColor);
}

void drawText(
  FT_Face face,
  cimg_t& image,
  int textHeight,
  std::u32string const & text,
  int leftTopX,
  int leftTopY,
  uint8_t fontColor[],
  FT_Stroker stroker,
  uint8_t strokeColor[],
  int separeteGlyphWidth
) {
  FT_Set_Pixel_Sizes(face, 0, textHeight);
  FT_GlyphSlot glyphSlot = face->glyph;
  int shiftX = leftTopX;
  for (size_t i = 0; i < text.length(); ++i) {
    FT_ULong symbol = text[i];
    if (FT_Load_Char(face, symbol, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP)) {
      FT_THROW("glyph failed to load");
    }
    if (!isspace(static_cast<char>(symbol))) {
      drawGlyph(
        glyphSlot, image, shiftX, leftTopY, fontColor, stroker, strokeColor);
    }
    shiftX += tfpos2int(glyphSlot->metrics.horiAdvance) + separeteGlyphWidth;
  }
}

int charWidth(FT_Face face, int textHeight, char32_t symbol) {
  FT_Set_Pixel_Sizes(face, 0, textHeight);
  FT_GlyphSlot glyphSlot = face->glyph;
  if (FT_Load_Char(face, symbol, FT_LOAD_BITMAP_METRICS_ONLY)){
      FT_THROW("glyph failed to load");
  }
  return tfpos2int(glyphSlot->metrics.horiAdvance);
}

int textWidth(
  FT_Face face,
  int textHeight,
  std::u32string const & text
) {
  int width = 0;
  for (size_t i = 0; i < text.length(); ++i) {
    width += charWidth(face, textHeight, text[i]);
  }
  return width;
}
