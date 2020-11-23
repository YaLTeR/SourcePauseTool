#include <codecvt>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <future>
#include <iomanip>
#include <locale>
#include <map>
#include <sstream>
#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

// Some hacks to make this json library behave well with its disgusting macros

#ifdef null
#undef null
#endif

#include "thirdparty\json.hpp"
#undef and
#undef or

// Remove min/max definitions from some SDK versions
#undef min
#undef max