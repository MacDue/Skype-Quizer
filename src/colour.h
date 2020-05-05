#ifndef COLOUR_H
#define COLOUR_H

#include <array>
#include <cstdint>

namespace DueUtil::Images {
  struct Colour {
    Colour() = default;
    Colour(int r, int g, int b)
      : rgb{r, g, b} {};

    std::array<int, 3> rgb;

    int32_t to_int();

    int const * c_arr() const  { return &rgb[0]; }
    uint8_t * as_bytes(uint8_t bytes[]) const;

    int luminance() const;

    bool operator==(Colour const & rhs) const
    { return this->rgb == rhs.rgb; }
    bool operator!=(Colour const & rhs) const
    { return !operator==(rhs); }
  };

  inline const Colour NOT_A_COLOUR { -255, -255, -255 };
  inline const Colour WHITE        {  255,  255,  255 };
  inline const Colour GRAY         {  204,  204,  204 };
  inline const Colour DUE_BLACK    {  48,   48,   48  };
  inline const Colour DUE_WHITE    {  238,  238,  238	};
}

#endif
