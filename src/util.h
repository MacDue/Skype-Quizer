#ifndef UTIL_H_
#define UTIL_H_

#include <string>

namespace DueUtil::Util {
  std::wstring to_utf16(std::string const & utf8);
  std::u32string to_utf32(std::string const & utf8);
}

#endif
