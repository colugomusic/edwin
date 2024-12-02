#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <span>
#include <string_view>

namespace edwin {

struct window;
struct frame_interval { std::chrono::milliseconds value = std::chrono::milliseconds{100}; };
struct native_handle  { void* value = nullptr; };
struct position       { int x = 0; int y = 0; }; 
struct resizable      { bool value = false; };
struct size           { int width = 0; int height = 0; }; 
struct title          { std::string_view value; };
struct rgba           { std::byte r, g, b, a; };
struct icon           { edwin::size size; std::span<rgba> pixels; };
struct visible        { bool value = false; };

static constexpr auto show = visible{true};
static constexpr auto hide = visible{false};

namespace sig {
using frame              = void();
using on_window_closed   = void();
using on_window_resized  = void(edwin::size size);
using on_window_resizing = void(edwin::size size);
} // sig

namespace fn {
struct frame              { std::function<sig::frame> fn; };
struct on_window_closed   { std::function<sig::on_window_closed> fn; };
struct on_window_resized  { std::function<sig::on_window_resized> fn; };
struct on_window_resizing { std::function<sig::on_window_resizing> fn; };
} // fn

struct window_config {
	edwin::fn::on_window_closed on_closed;
	edwin::fn::on_window_resized on_resized;
	edwin::fn::on_window_resizing on_resizing;
	edwin::icon icon;
	edwin::native_handle parent;
	edwin::position position;
	edwin::resizable resizable;
	edwin::size size;
	edwin::title title;
	edwin::visible visible;
};

[[nodiscard]] auto create(window_config cfg) -> window*;
              auto destroy(window* wnd) -> void;

              // Return the native handle for the window.
              // Windows: HWND
              // Linux: Window
              // macOS: NSView
			  // Also look at edwin-ext.hpp for platform-specific functions.
[[nodiscard]] auto get_native_handle(const window& wnd) -> native_handle;

              // Set the icon for the window.
              // Windows: It will show up in the title bar.
              // Linux: I don't know if it works because my window manager doesn't actually have window icons.
              // macOS: This is a no-op because having individual window icons isn't really a thing AFAIK.
              auto set(window* wnd, edwin::icon icon) -> void;
              
              // Other properties.
              auto set(window* wnd, edwin::position position) -> void;
              auto set(window* wnd, edwin::position position, edwin::size size) -> void;
              auto set(window* wnd, edwin::resizable resizable) -> void;
              auto set(window* wnd, edwin::size size) -> void;
              auto set(window* wnd, edwin::title title) -> void;
              auto set(window* wnd, edwin::visible visible) -> void;
              auto set(window* wnd, fn::on_window_closed cb) -> void;
              auto set(window* wnd, fn::on_window_resized cb) -> void;
              auto set(window* wnd, fn::on_window_resizing cb) -> void;

              // How to process window messages.
              // This varies according the the stupidity of the platform.

              // macOS:
              // This is the most stupid platform. You have to call app_beg() which will
              // enter a message loop and keep going until app_end() is called. The frame
              // callback you pass in will be called on a timer at the given interval.
              // process_messages() is a no-op on macOS.

              // Windows:
              // This is the second most stupid platform. You can run your own message loop
              // and call process_messages() on each iteration to process window messages.
              // However this will cause your entire application to freeze while the window
              // is being resized, so to avoid this you can instead call app_beg() to have
              // edwin run a message loop for you and app_end() to stop it. The frame
              // callback you pass in will be called on a timer at the given interval.

              // Linux:
              // This is the least stupid platform. You can either run your own message loop
              // and call process_messages() on each iteration, or use app_beg/app_end to
              // have edwin run the loop for you.

              // Don't mix both process_messages and app_beg/app_end. Just do one or the other.
              auto process_messages() -> void;
              auto app_beg(edwin::fn::frame frame, edwin::frame_interval interval) -> void;
              auto app_end() -> void;

} // edwin
