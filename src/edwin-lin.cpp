#include "edwin.hpp"
#include <vector>
#include <X11/Xlib.h>

namespace edwin {

static constexpr auto MIN_SIZE = 10;
static constexpr auto MAX_SIZE = 10000;

struct window {
	Window xwindow = 0;
	edwin::resizable resizable;
	edwin::size size;
	fn::on_window_closed on_closed;
	fn::on_window_resized on_resized;
};

struct entry {
	Window xwindow;
	window* wnd;
};

static std::vector<entry> window_list_;

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
	const auto xdisplay = XOpenDisplay(nullptr);
	if (!xdisplay) {
		return nullptr;
	}
	const auto screen = DefaultScreen(xdisplay);
	const auto parent = cfg.parent.value ? (Window)(cfg.parent.value) : RootWindow(xdisplay, screen);
	const auto border_width = 0;
	const auto border = BlackPixel(xdisplay, screen);
	const auto background = WhitePixel(xdisplay, screen);
	wnd.xwindow = XCreateSimpleWindow(xdisplay, parent, cfg.position.x, cfg.position.y, cfg.size.width, cfg.size.height, border_width, border, background);
	if (!wnd.xwindow) {
		return nullptr;
	}
	wnd.size = cfg.size;
	window_list_.push_back({wnd.xwindow, wnd.get()});
	set(wnd.get(), cfg.on_closed);
	set(wnd.get(), cfg.on_resized);
	set(wnd.get(), cfg.icon);
	set(wnd.get(), cfg.resizable);
	set(wnd.get(), cfg.title);
	set(wnd.get(), cfg.visible);
	return wnd.release();
}

auto destroy(window* wnd) -> void {
	if (!wnd) { return; }
	if (!wnd->xwindow) { return; }
	const auto xdisplay = XOpenDisplay(nullptr);
	if (!xdisplay) { return; }
	XDestroyWindow(xdisplay, wnd->xwindow);
}

auto get_native_handle(const window& w) -> native_handle {
	return native_handle{w.xwindow};
}

auto set(window* wnd, edwin::icon icon) -> void {
	std::vector<unsigned long> icon_data;
	icon_data.resize(2 + (icon.width * icon.height));
	icon_data[0] = icon.width;
	icon_data[1] = icon.height;
	for (size_t i = 0; i < icon.pixels.size(); ++i) {
		icon_data[i + 2] = (icon.pixels[i].a << 24) | (icon.pixels[i].r << 16) | (icon.pixels[i].g << 8) | icon.pixels[i].b;
	}
	const auto xdisplay = XOpenDisplay(nullptr);
	const auto property = XInternAtom(xdisplay, "_NET_WM_ICON", False);
	XChangeProperty(xdisplay, wnd->xwindow, property, XA_CARDINAL, 32, PropModeReplace, reinterpret_cast<const unsigned char*>(icon_data.data()), icon_data.size());
}

auto set(window* wnd, edwin::position position) -> void {
	XMoveWindow(XOpenDisplay(nullptr), wnd->xwindow, position.x, position.y);
}

auto set(window* wnd, edwin::position position, edwin::size size) -> void {
	XMoveResizeWindow(XOpenDisplay(nullptr), wnd->xwindow, position.x, position.y, size.width, size.height);
	set(wnd, wnd->resizable);
}

auto set(window* wnd, edwin::resizable resizable) -> void {
	XSizeHints hints;
	hints.flags = PMinSize | PMaxSize;
	if (resizable.value) {
		hints.min_width  = MIN_SIZE
		hints.min_height = MIN_SIZE;
		hints.max_width  = MAX_SIZE;
		hints.max_height = MAX_SIZE;
	}
	else {
		hints.min_width = hints.max_width = wnd->size.width;
		hints.min_height = hints.max_height = wnd->size.height;
	}
	XSetWMNormalHints(XOpenDisplay(nullptr), wnd->xwindow, &hints);
}

auto set(window* wnd, edwin::size size) -> void {
	XResizeWindow(XOpenDisplay(nullptr), wnd->xwindow, size.width, size.height);
	set(wnd, wnd->resizable);
}

auto set(window* wnd, edwin::title title) -> void {
	XStoreName(XOpenDisplay(nullptr), wnd->xwindow, title.value.data());
}

auto set(window* wnd, edwin::visible visible) -> void {
	if (visible.value) { XMapWindow(XOpenDisplay(nullptr), wnd->xwindow); }
	else               { XUnmapWindow(XOpenDisplay(nullptr), wnd->xwindow); }
}

auto set(window* wnd, fn::on_window_closed cb) -> void {
	wnd->on_closed(cb);
}

auto set(window* wnd, fn::on_window_resized cb) -> void {
	wnd->on_resized(cb);
}

static
auto on_notify_configure(const XConfigureEvent& event) -> void {
	if (const auto wnd = get_window(event.xconfigure.window)) {
		if (wnd->on_resized) {
			wnd->on_resized(size{event.xconfigure.width, event.xconfigure.height});
		}
	}
}

static
auto on_notify_destroy(const XDestroyWindowEvent& event) -> void {
	if (const auto wnd = get_window(event.window)) {
		if (wnd->on_closed) {
			wnd->on_closed();
		}
		destroy(wnd);
	}
}

auto process_messages() -> void {
	const auto xdisplay = XOpenDisplay(nullptr);
	XEvent event;
	while (XPending(xdisplay)) {
		XNextEvent(xdisplay, &event);
		switch (event.type) {
			case ConfigureNotify: { on_notify_configure(event.xconfigure); break; }
			case DestroyNotify:   { on_notify_destroy(event.xdestroywindow); break; }
			default:              { break; }
		}
	}
}

} // edwin
