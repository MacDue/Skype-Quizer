#include <iostream>
#include "fonts/due_font.h"

namespace DueUtil::Images {
  DueFont::DueFont(std::string const & filepath) {
    initFreetype(m_lib, m_face, filepath.c_str());
    FT_Stroker_New(m_lib, &m_stroker);
  }

  DueFont::DueFont(DueFont const & font) {
    m_lib = font.m_lib;
    m_face = font.m_face;
  }

  DueFont& DueFont::operator=(DueFont const & that) {
    if (this != &that) {
      FT_Stroker_Done(m_stroker);
      closeFreetype(m_lib, m_face);
      m_lib = that.m_lib;
      m_face = that.m_face;
      m_stroker = that.m_stroker;
    }
    return *this;
  }

  FT_Face DueFont::face() {
    return m_face;
  }

  FT_Stroker DueFont::stroker(int width) {
    FT_Stroker_Set(m_stroker, width * (1 << 6),
      FT_STROKER_LINECAP_SQUARE, FT_STROKER_LINEJOIN_MITER_FIXED, 0);
    return m_stroker;
  }

  DueFont::~DueFont() {
    FT_Stroker_Done(m_stroker);
    closeFreetype(m_lib, m_face);
  }
}
