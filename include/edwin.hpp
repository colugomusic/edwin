#pragma once

#include <cstddef>
#include <functional>
#include <span>
#include <string_view>

namespace edwin {

struct window;
struct native_handle { void* value = nullptr; };
struct position      { int x = 0; int y = 0; }; 
struct resizable     { bool value = false; };
struct size          { int width  = 0; int height = 0; }; 
struct title         { std::string_view value; };
struct rgba          { std::byte r, g, b, a; };
struct icon          { edwin::size size; std::span<rgba> pixels; };
struct visible       { bool value = false; };

static constexpr auto show = visible{true};
static constexpr auto hide = visible{false};

namespace sig {
using on_window_closed  = void();
using on_window_resized = void(edwin::size size);
} // sig

namespace fn {
using on_window_closed  = std::function<sig::on_window_closed>;
using on_window_resized = std::function<sig::on_window_resized>;
} // fn

struct window_config {
	edwin::fn::on_window_closed on_closed;
	edwin::fn::on_window_resized on_resized;
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
              auto set(window* w, edwin::icon icon) -> void;
              auto set(window* w, edwin::position position) -> void;
              auto set(window* w, edwin::position position, edwin::size size) -> void;
              auto set(window* w, edwin::resizable resizable) -> void;
              auto set(window* w, edwin::size size) -> void;
              auto set(window* w, edwin::title title) -> void;
              auto set(window* w, edwin::visible visible) -> void;
              auto set(window* w, fn::on_window_closed cb) -> void;
              auto set(window* w, fn::on_window_resized cb) -> void;
              auto process_messages() -> void;

} // edwin
