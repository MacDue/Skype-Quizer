#ifndef DUE_FONT_H
#define DUE_FONT_H

#include "fonts/cimg_freetype.h"

namespace DueUtil::Images {
  class DueFont {
    FT_Library m_lib;
    FT_Face m_face;
    FT_Stroker m_stroker;

    public:
      DueFont(std::string const & filepath);
      DueFont(DueFont const & font);

      DueFont& operator=(DueFont const & font);

      FT_Face face();

      FT_Stroker stroker(int width);

      ~DueFont();
  };
}

#endif
