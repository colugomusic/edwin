#include "edwin.hpp"
#include <chrono>
#include <memory>
#include <thread>
#include <vector>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace edwin {

static constexpr auto MIN_SIZE = 10;
static constexpr auto MAX_SIZE = 10000;

struct window {
	Window xwindow = 0;
	edwin::resizable resizable;
	edwin::size size;
	fn::on_window_closed on_closed;
	fn::on_window_resized on_resized;
	fn::on_window_resizing on_resizing;
};

struct entry {
	Window xwindow;
	window* wnd;
};

static std::vector<entry> window_list_;
static fn::frame app_frame_;
static bool app_schedule_stop_ = false;

static
auto get_xdisplay() -> Display* {
	static auto xdisplay = XOpenDisplay(nullptr);
	return xdisplay;
}

static
auto get_window(Window xwindow) -> window* {
	for (const auto& entry : window_list_) {
		if (entry.xwindow == xwindow) {
			return entry.wnd;
		}
	}
	return nullptr;
}

auto create(window_config cfg) -> window* {
	auto wnd = std::make_unique<window>();
	const auto xdisplay = get_xdisplay();
	if (!xdisplay) {
		return nullptr;
	}
	const auto screen = DefaultScreen(xdisplay);
	const auto parent = cfg.parent.value ? (Window)(cfg.parent.value) : RootWindow(xdisplay, screen);
	const auto visual = DefaultVisual(xdisplay, screen);
	const auto depth = DefaultDepth(xdisplay, screen);
	const auto border_width = 0;
	wnd->xwindow = XCreateSimpleWindow(xdisplay, parent, cfg.position.x, cfg.position.y, cfg.size.width, cfg.size.height, border_width, BlackPixel(xdisplay, screen), WhitePixel(xdisplay, screen));
	if (!wnd->xwindow) {
		return nullptr;
	}
	wnd->size = cfg.size;
	window_list_.push_back({wnd->xwindow, wnd.get()});
	set(wnd.get(), cfg.on_closed);
	set(wnd.get(), cfg.on_resized);
	set(wnd.get(), cfg.on_resizing);
	set(wnd.get(), cfg.icon);
	set(wnd.get(), cfg.resizable);
	set(wnd.get(), cfg.title);
	set(wnd.get(), cfg.visible);
	return wnd.release();
}

auto destroy(window* wnd) -> void {
	if (!wnd) { return; }
	if (!wnd->xwindow) { return; }
	const auto xdisplay = get_xdisplay();
	if (!xdisplay) { return; }
	XDestroyWindow(xdisplay, wnd->xwindow);
}

auto get_native_handle(const window& w) -> native_handle {
	return native_handle{(void*)(w.xwindow)};
}

auto get_xwindow(const window& w) -> Window {
	return w.xwindow;
}

auto set(window* wnd, edwin::icon icon) -> void {
	std::vector<unsigned long> icon_data;
	icon_data.resize(2 + (icon.size.width * icon.size.height));
	icon_data[0] = icon.size.width;
	icon_data[1] = icon.size.height;
	for (size_t i = 0; i < icon.pixels.size(); ++i) {
		long unsigned int value = 0;
		value |= (long unsigned int)(icon.pixels[i].a); value <<= 8;
		value |= (long unsigned int)(icon.pixels[i].r); value <<= 8;
		value |= (long unsigned int)(icon.pixels[i].g); value <<= 8;
		value |= (long unsigned int)(icon.pixels[i].b);
		icon_data[i + 2] = value;
	}
	const auto xdisplay = get_xdisplay();
	const auto property = XInternAtom(xdisplay, "_NET_WM_ICON", False);
	XChangeProperty(xdisplay, wnd->xwindow, property, XA_CARDINAL, 32, PropModeReplace, reinterpret_cast<const unsigned char*>(icon_data.data()), icon_data.size());
}

auto set(window* wnd, edwin::position position) -> void {
	XMoveWindow(get_xdisplay(), wnd->xwindow, position.x, position.y);
}

auto set(window* wnd, edwin::position position, edwin::size size) -> void {
	XMoveResizeWindow(get_xdisplay(), wnd->xwindow, position.x, position.y, size.width, size.height);
	set(wnd, wnd->resizable);
}

auto set(window* wnd, edwin::resizable resizable) -> void {
	XSizeHints hints = {0};
	hints.flags = PMinSize | PMaxSize;
	if (resizable.value) {
		hints.min_width  = MIN_SIZE;
		hints.min_height = MIN_SIZE;
		hints.max_width  = MAX_SIZE;
		hints.max_height = MAX_SIZE;
		hints.flags |= PResizeInc | PBaseSize;
		hints.base_width = 1;
		hints.base_height = 1;
		hints.resize_inc_width = 10;
		hints.resize_inc_height = 10;
	}
	else {
		hints.min_width = hints.max_width = wnd->size.width;
		hints.min_height = hints.max_height = wnd->size.height;
	}
	XSetWMNormalHints(get_xdisplay(), wnd->xwindow, &hints);
}

auto set(window* wnd, edwin::size size) -> void {
	XResizeWindow(get_xdisplay(), wnd->xwindow, size.width, size.height);
	set(wnd, wnd->resizable);
}

auto set(window* wnd, edwin::title title) -> void {
	XStoreName(get_xdisplay(), wnd->xwindow, title.value.data());
}

auto set(window* wnd, edwin::visible visible) -> void {
	if (visible.value) { XMapWindow(get_xdisplay(), wnd->xwindow); }
	else               { XUnmapWindow(get_xdisplay(), wnd->xwindow); }
}

auto set(window* wnd, fn::on_window_closed cb) -> void {
	wnd->on_closed = cb;
}

auto set(window* wnd, fn::on_window_resized cb) -> void {
	wnd->on_resized = cb;
}

auto set(window* wnd, fn::on_window_resizing cb) -> void {
	wnd->on_resizing = cb;
}

static
auto on_notify_configure(const XConfigureEvent& event) -> void {
	if (const auto wnd = get_window(event.window)) {
		if (wnd->on_resizing.fn) {
			wnd->on_resizing.fn(size{event.width, event.height});
		}
	}
}

static
auto on_client_message(const XEvent& event) -> void {
	if (const auto wnd = get_window(event.xany.window)) {
		if (wnd->on_closed.fn) {
			wnd->on_closed.fn();
		}
		destroy(wnd);
	}
}

auto process_messages() -> void {
	const auto xdisplay = get_xdisplay();
	XEvent event;
	while (XPending(xdisplay)) {
		XNextEvent(xdisplay, &event);
		switch (event.type) {
			case ConfigureNotify: { on_notify_configure(event.xconfigure); break; }
			case DestroyNotify:   { on_client_message(event); break; }
			default:              { break; }
		}
	}
}

auto app_beg(edwin::fn::frame frame, edwin::frame_interval interval) -> void {
	app_frame_ = frame;
	auto next_frame = std::chrono::steady_clock::now();
	for (;;) {
		process_messages();
		if (app_schedule_stop_) {
			return;
		}
		next_frame += interval.value;
		std::this_thread::sleep_until(next_frame);
	}
}

auto app_end() -> void {
	app_schedule_stop_ = true;
}

} // edwin
