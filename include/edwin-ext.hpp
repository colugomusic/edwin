#pragma once

#include "edwin.hpp"

#if defined(_WIN32) ////////////////////////////////////////////////////////

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace edwin {

[[nodiscard]] auto get_hwnd(const window& w) -> HWND;

} // edwin

#endif

#if defined(__linux__) /////////////////////////////////////////////////////

#include <X11/Xlib.h>

namespace edwin {

[[nodiscard]] auto get_xwindow(const window& w) -> Window;

} // edwin

#endif

#if defined(__APPLE__) /////////////////////////////////////////////////////

#include <Cocoa/Cocoa.h>

namespace edwin {

[[nodiscard]] auto get_nsview(const window& w) -> NSView*;

} // edwin

#endif
