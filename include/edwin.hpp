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
              auto destroy(window* w) -> void;

[[nodiscard]] auto get_native_handle(const window& w) -> native_handle;

              auto set(window* wnd, edwin::icon icon) -> void;
              auto set(window* wnd, edwin::position position) -> void;
              auto set(window* wnd, edwin::position position, edwin::size size) -> void;
              auto set(window* wnd, edwin::resizable resizable) -> void;
              auto set(window* wnd, edwin::size size) -> void;
              auto set(window* wnd, edwin::title title) -> void;
              auto set(window* wnd, edwin::visible visible) -> void;
              auto set(window* wnd, fn::on_window_closed cb) -> void;
              auto set(window* wnd, fn::on_window_resized cb) -> void;
              auto set(window* wnd, fn::on_window_resizing cb) -> void;

              // Call this in a loop.
              auto process_messages() -> void;

              // Instead of calling process_messages() in your own loop, you can use app_beg()
              // and app_end() to have edwin process the message loop for you. Unfortunately
              // it's necessary to use this if you want resizing to work without freezing your
              // application on Windows.
              auto app_beg(edwin::fn::frame frame, edwin::frame_interval interval) -> void;
              auto app_end() -> void;

} // edwin
