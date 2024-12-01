#define NOMINMAX
#include "dwmapi.h"
#include "edwin.hpp"
#include <memory>
#include <Windows.h>

namespace edwin {

static constexpr auto MIN_SIZE = 10;

struct window {
	HWND hwnd   = nullptr;
	HICON hicon = nullptr;
	fn::on_window_closed on_closed;
	fn::on_window_resized on_resized;
	fn::on_window_resizing on_resizing;
};

static UINT_PTR app_timer_ = 0;
static fn::frame app_frame_;
static bool app_schedule_stop_ = false;

static
auto app_timer_proc(HWND hwnd, UINT msg, UINT_PTR id, DWORD time) -> void {
	if (app_frame_.fn) {
		app_frame_.fn();
	}
}

static
auto get_window(HWND hwnd) -> window* {
	return (window*)(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

static
auto wm_close(HWND hwnd, UINT msg, WPARAM w, LPARAM l) -> LRESULT {
	if (const auto wnd = get_window(hwnd)) {
		if (wnd->on_closed.fn) {
			wnd->on_closed.fn();
		}
		destroy(wnd);
	}
	return 0;
}

static
auto wm_create(HWND hwnd, UINT msg, WPARAM w, LPARAM l) -> LRESULT {
	auto* const create_params = (CREATESTRUCT*)(l);
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)(create_params->lpCreateParams));
	return 0;
}

static
auto wm_destroy(HWND hwnd, UINT msg, WPARAM w, LPARAM l) -> LRESULT {
	if (const auto wnd = get_window(hwnd)) {
		delete wnd;
	}
	return 0;
}

static
auto wm_size(HWND hwnd, UINT msg, WPARAM w, LPARAM l) -> LRESULT {
	if (const auto wnd = get_window(hwnd)) {
		if (wnd->on_resized.fn) {
			const auto type   = w;
			const auto width  = LOWORD(l);
			const auto height = HIWORD(l);
			wnd->on_resized.fn(size{width, height});
		}
	}
	return 0;
}

static
auto wm_sizing(HWND hwnd, UINT msg, WPARAM w, LPARAM l) -> LRESULT {
	if (const auto wnd = get_window(hwnd)) {
		if (wnd->on_resizing.fn) {
			const auto edge   = w;
			const auto rect   = (RECT*)(l);
			wnd->on_resizing.fn(size{rect->right - rect->left, rect->bottom - rect->top});
		}
	}
	return 0;
}

static
auto CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) -> LRESULT {
	switch (msg) {
		case WM_CLOSE:   { return wm_close(hwnd, msg, w, l); }
		case WM_CREATE:  { return wm_create(hwnd, msg, w, l); }
		case WM_DESTROY: { return wm_destroy(hwnd, msg, w, l); }
		case WM_SIZE:    { return wm_size(hwnd, msg, w, l); }
		case WM_SIZING:  { return wm_sizing(hwnd, msg, w, l); }
	}
	return DefWindowProc(hwnd, msg, w, l);
}

static
auto init_wndclass() -> WNDCLASS {
	const auto wndclass = WNDCLASS{
		.lpfnWndProc = wndproc,
		.hInstance = GetModuleHandle(nullptr),
		.lpszClassName = "edwin",
	};
	RegisterClass(&wndclass);
	return wndclass;
}

static
auto get_wndclass() -> std::string_view {
	static const auto wndclass = init_wndclass();
	return wndclass.lpszClassName;
}

static
auto make_style(edwin::resizable resizable) -> DWORD {
	auto style = WS_OVERLAPPEDWINDOW;
	if (!resizable.value) {
		style &= ~WS_SIZEBOX;
	}
	return style;
}

static
auto make_hicon_mask(edwin::size size) -> HBITMAP {
	const auto width = size.width;
	const auto height = size.height;
	const auto mask = CreateBitmap(width, height, 1, 1, nullptr);
	return mask;
}

static
auto make_hicon_color(edwin::icon icon) -> HBITMAP {
	const auto hdc = GetDC(nullptr);
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth       = icon.size.width;
	bmi.bmiHeader.biHeight      = -icon.size.height;
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biBitCount    = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	void* dib_pixels = nullptr;
	const auto bitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &dib_pixels, nullptr, 0);
	if (!bitmap) {
		ReleaseDC(nullptr, hdc);
		return nullptr;
	}
	for (auto y = 0; y < icon.size.height; y++) {
		for (auto x = 0; x < icon.size.width; x++) {
			const auto i = y * icon.size.width + x;
			const auto r = icon.pixels[i].r;
			const auto g = icon.pixels[i].g;
			const auto b = icon.pixels[i].b;
			const auto a = icon.pixels[i].a;
			const auto index = (y * icon.size.width + x) * 4;
			reinterpret_cast<BYTE*>(dib_pixels)[index + 0] = static_cast<BYTE>(b);
			reinterpret_cast<BYTE*>(dib_pixels)[index + 1] = static_cast<BYTE>(g);
			reinterpret_cast<BYTE*>(dib_pixels)[index + 2] = static_cast<BYTE>(r);
			reinterpret_cast<BYTE*>(dib_pixels)[index + 3] = static_cast<BYTE>(a);
		}
	}
	ReleaseDC(nullptr, hdc);
	return bitmap;
}

static
auto make_hicon(edwin::icon icon) -> HICON {
	ICONINFO iconinfo;
	iconinfo.fIcon    = TRUE;
	iconinfo.hbmMask  = make_hicon_mask(icon.size);
	iconinfo.hbmColor = make_hicon_color(icon);
	if (!iconinfo.hbmMask || !iconinfo.hbmColor) {
		if (iconinfo.hbmMask)  { DeleteObject(iconinfo.hbmMask); }
		if (iconinfo.hbmColor) { DeleteObject(iconinfo.hbmColor); }
		return nullptr;
	}
	const auto hicon = CreateIconIndirect(&iconinfo);
	DeleteObject(iconinfo.hbmMask);
	DeleteObject(iconinfo.hbmColor);
	return hicon;
}

auto create(window_config cfg) -> window* {
	auto wnd = std::make_unique<window>();
	const auto exstyle = DWORD{0};
	const auto wndclass = get_wndclass();
	const auto title = cfg.title.value;
	const auto style = make_style(cfg.resizable);
	const auto x = std::max(0, cfg.position.x);
	const auto y = std::max(0, cfg.position.y);
	const auto w = std::max(MIN_SIZE, cfg.size.width);
	const auto h = std::max(MIN_SIZE, cfg.size.height);
	const auto parent = (HWND)(cfg.parent.value);
	const auto menu = (HMENU)(0);
	const auto hinstance = GetModuleHandleA(0);
	const auto create_params = wnd.get();
	wnd->hwnd = CreateWindowEx(exstyle, wndclass.data(), title.data(), style, x, y, w, h, parent, menu, hinstance, create_params);
	if (!wnd->hwnd) {
		return nullptr;
	}
	set(wnd.get(), cfg.on_closed);
	set(wnd.get(), cfg.on_resized);
	set(wnd.get(), cfg.on_resizing);
	set(wnd.get(), cfg.icon);
	set(wnd.get(), cfg.visible);
	return wnd.release();
}

auto destroy(window* wnd) -> void {
	if (!wnd)       { return; }
	if (!wnd->hwnd) { return; }
	DestroyWindow(wnd->hwnd);
}

auto get_native_handle(const window& wnd) -> native_handle {
	return native_handle{wnd.hwnd};
}

auto get_hwnd(const window& wnd) -> HWND {
	return wnd.hwnd;
}

auto set(window* wnd, edwin::icon icon) -> void {
	auto old_icon = wnd->hicon;
	wnd->hicon = make_hicon(icon);
	if (!wnd->hicon) {
		return;
	}
	SendMessage(wnd->hwnd, WM_SETICON, ICON_BIG, (LPARAM)(wnd->hicon));
	SendMessage(wnd->hwnd, WM_SETICON, ICON_SMALL, (LPARAM)(wnd->hicon));
	if (old_icon) {
		DestroyIcon(old_icon);
	}
}

auto set(window* wnd, edwin::position position) -> void {
	SetWindowPos(wnd->hwnd, nullptr, position.x, position.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

auto set(window* wnd, edwin::position position, edwin::size size) -> void {
	set(wnd, position);
	set(wnd, size);
}

auto set(window* wnd, edwin::resizable resizable) -> void {
	auto style = GetWindowLongPtr(wnd->hwnd, GWL_STYLE);
	style = resizable.value ? (style | WS_SIZEBOX) : (style & ~WS_SIZEBOX);
	SetWindowLongPtr(wnd->hwnd, GWL_STYLE, style);
	SetWindowPos(wnd->hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
}

auto set(window* wnd, edwin::size size) -> void {
	RECT rect = {0, 0, size.width, size.height};
	const auto style    = GetWindowLong(wnd->hwnd, GWL_STYLE);
	const auto exstyle  = GetWindowLong(wnd->hwnd, GWL_EXSTYLE);
	const auto has_menu = GetMenu(wnd->hwnd) != nullptr;
	AdjustWindowRectEx(&rect, style, has_menu, exstyle);
	size.width = rect.right - rect.left;
	size.height = rect.bottom - rect.top;
	SetWindowPos(wnd->hwnd, nullptr, 0, 0, size.width, size.height, SWP_NOMOVE | SWP_NOZORDER);
}

auto set(window* wnd, edwin::title title) -> void {
	SetWindowText(wnd->hwnd, title.value.data());
}

auto set(window* wnd, edwin::visible visible) -> void {
	ShowWindow(wnd->hwnd, visible.value ? SW_SHOW : SW_HIDE);
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

auto process_messages() -> void {
	auto msg = MSG{};
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

auto app_beg(edwin::fn::frame frame, edwin::frame_interval interval) -> void {
	app_frame_ = frame;
	app_timer_ = SetTimer(nullptr, 1, interval.value.count(), app_timer_proc);
	auto msg = MSG{};
	while (GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (app_schedule_stop_) {
			return;
		}
	}
}

auto app_end() -> void {
	app_schedule_stop_ = true;
}

} // edwin
