// Copyright 2017 Global Phasing Ltd.
//
// to_str(float|double), handling -D USE_STD_SNPRINTF

#ifndef GEMMI_SPRINTF_HPP_
#define GEMMI_SPRINTF_HPP_

#ifdef USE_STD_SNPRINTF  // for benchmarking and testing only
# include <cstdio>
# define gstb_snprintf std::snprintf
# define gstb_sprintf std::sprintf
#else
# ifdef GEMMI_WRITE_IMPLEMENTATION
#  define STB_SPRINTF_IMPLEMENTATION
# endif
# define STB_SPRINTF_DECORATE(name) gstb_##name
# include "third_party/stb_sprintf.h"
#endif
#include <string>

namespace gemmi {

inline std::string to_str(double d) {
  char buf[24];
  int len = gstb_sprintf(buf, "%.9g", d);
  return std::string(buf, len > 0 ? len : 0);
}

inline std::string to_str(float d) {
  char buf[16];
  int len = gstb_sprintf(buf, "%.6g", d);
  return std::string(buf, len > 0 ? len : 0);
}

template<int Prec>
std::string to_str_prec(double d) {
  static_assert(Prec >= 0 && Prec < 7, "unsupported precision");
  char buf[16];
  int len = d > -1e8 && d < 1e8 ? gstb_sprintf(buf, "%.*f", Prec, d)
                                : gstb_sprintf(buf, "%g", d);
  return std::string(buf, len > 0 ? len : 0);
}

// this is equivalent of stbsp_snprintf, but with __attribute__(format)
#if defined(__GNUC__) || (defined(__clang) && __has_attribute(format))
__attribute__((format(printf, 3, 4)))
#endif
inline int gf_snprintf(char *buf, int count, char const *fmt, ...) {
   int result;
   va_list va;
   va_start(va, fmt);
   result = gstb_vsnprintf(buf, count, fmt, va);
   va_end(va);
   return result;
}


} // namespace gemmi
#endif
