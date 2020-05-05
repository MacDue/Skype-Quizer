#include "util.h"

#include <locale>
#include <codecvt>

namespace DueUtil::Util {
  static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> utf16_cov;
  std::wstring to_utf16(std::string const & utf8) {
    return utf16_cov.from_bytes(utf8);
  }

  static std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t> utf32_cov;
  std::u32string to_utf32(std::string const & utf8) {
    return utf32_cov.from_bytes(utf8);
  }
}
