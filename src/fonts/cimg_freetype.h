#ifndef CIMG_FREETYPE
#define CIMG_FREETYPE

/*
Code from
https://github.com/ilc-opensource/smart_mug

BSD-3-Clause
*/
#include <cstdint>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftstroke.h>

#include <CImg.h>

using namespace cimg_library;
typedef CImg<uint8_t> cimg_t;

void initFreetype(
  FT_Library& ftlib,
  FT_Face& face,
  std::string const & fontFullName);

void closeFreetype(
  FT_Library ftlib,
  FT_Face face);

void drawText(
  FT_Face face,
  cimg_t& image,
  int textHeight,
  std::u32string const & text,
  int leftTopX,
  int leftTopY,
  uint8_t fontColor[] = nullptr,
  FT_Stroker stroker = nullptr,
  uint8_t strokeColor[] = nullptr,
  int separeteGlyphWidth = 1);

int charWidth(FT_Face face, int textHeight, char32_t symbol);

int textWidth(
  FT_Face face,
  int textHeight,
  std::u32string const & text);

#endif
