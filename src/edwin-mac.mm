#include "edwin.hpp"
#include <Cocoa/Cocoa.h>
#include <thread>

@interface EdwinWindow : NSWindow
@property (nonatomic, readwrite) edwin::window* wnd;
@end

namespace edwin {

struct window {
    EdwinWindow* nswindow = nullptr;
    NSView* nsview        = nullptr;
    fn::on_window_closed on_window_closed;
    fn::on_window_resized on_window_resized;
    fn::on_window_resizing on_window_resizing;
};

} // edwin

@implementation EdwinWindow
- (void) windowDidResize: (NSNotification*) notification {
	NSRect frame = [self frame];
	if (self.wnd->on_window_resized.fn) {
        const auto w = (int)(frame.size.width);
        const auto h = (int)(frame.size.height);
        self.wnd->on_window_resized.fn({w, h});
	}
}
- (void) windowWillResize: (NSWindow*) sender toSize: (NSSize) frameSize {
	if (self.wnd->on_window_resizing.fn) {
        const auto w = (int)(frameSize.width);
        const auto h = (int)(frameSize.height);
        self.wnd->on_window_resizing.fn({w, h});
	}
}
- (void) windowWillClose: (NSNotification*) notification {
	if (self.wnd->on_window_closed.fn) {
		self.wnd->on_window_closed.fn();
	}
}
@end

namespace edwin {

static fn::frame app_frame_;
static bool app_schedule_stop_ = false;
static NSApplication* app_ = nullptr;

static
auto get_app() -> NSApplication* {
	if (!app_) {
		app_ = [NSApplication sharedApplication];
	}
	return app_;
}

auto create(window_config cfg) -> window* {
	get_app();
	auto wnd = std::make_unique<window>();
	const auto rect = NSMakeRect(cfg.position.x, cfg.position.y, cfg.size.width, cfg.size.height);
	wnd->nswindow = [[EdwinWindow alloc]
		initWithContentRect: rect
		styleMask:           NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
		backing:             NSBackingStoreBuffered
		defer:               NO
	];
	wnd->nswindow.wnd = wnd.get();
	wnd->nsview       = [[NSView alloc] initWithFrame: rect];
	[wnd->nswindow setContentView: wnd->nsview];
	set(wnd.get(), cfg.title);
	set(wnd.get(), cfg.resizable);
	set(wnd.get(), cfg.visible);
	set(wnd.get(), cfg.on_closed);
	set(wnd.get(), cfg.on_resized);
	set(wnd.get(), cfg.on_resizing);
	return wnd.release();
}

auto destroy(window* wnd) -> void {
	if (!wnd)          { return; }
	if (wnd->nswindow) { [wnd->nswindow close]; }
	if (wnd->nsview)   { [wnd->nsview release]; }
	delete wnd;
}

auto get_native_handle(const window& wnd) -> native_handle {
	return {wnd.nsview};
}

auto get_nsview(const window& wnd) -> NSView* {
    return wnd.nsview;
}

auto set(window* wnd, edwin::icon icon) -> void {
	// No-op on macOS.
}

auto set(window* wnd, edwin::position position) -> void {
	auto frame = [wnd->nswindow frame];
	frame.origin.x = position.x;
	frame.origin.y = position.y;
	[wnd->nswindow setFrame: frame display: YES];
}

auto set(window* wnd, edwin::position position, edwin::size size) -> void {
	auto frame = [wnd->nswindow frame];
	frame.origin.x = position.x;
	frame.origin.y = position.y;
	frame.size.width = size.width;
	frame.size.height = size.height;
	[wnd->nswindow setFrame: frame display: YES];
}

auto set(window* wnd, edwin::resizable resizable) -> void {
	const auto mask = [wnd->nswindow styleMask];
	if (resizable.value) {
		[wnd->nswindow setStyleMask: mask | NSWindowStyleMaskResizable];
	} else {
		[wnd->nswindow setStyleMask: mask & ~NSWindowStyleMaskResizable];
	}
}

auto set(window* wnd, edwin::size size) -> void {
	auto frame = [wnd->nswindow frame];
	frame.size.width = size.width;
	frame.size.height = size.height;
	[wnd->nswindow setFrame: frame display: YES];
}

auto set(window* wnd, edwin::title title) -> void {
	[wnd->nswindow setTitle: [NSString stringWithUTF8String: title.value.data()]];
}

auto set(window* wnd, edwin::visible visible) -> void {
	if (visible.value) {
		[wnd->nswindow makeKeyAndOrderFront: nil];
	} else {
		[wnd->nswindow orderOut: nil];
	}
}

auto set(window* wnd, fn::on_window_closed cb) -> void {
	wnd->on_window_closed = cb;
}

auto set(window* wnd, fn::on_window_resized cb) -> void {
	wnd->on_window_resized = cb;
}

auto set(window* wnd, fn::on_window_resizing cb) -> void {
	wnd->on_window_resizing = cb;
}

auto process_messages() -> void {
	const auto app = get_app();
	[app updateWindows];
}

auto app_beg(edwin::fn::frame frame, edwin::frame_interval interval) -> void {
	const auto app = get_app();
	app_frame_ = frame;
	auto next_frame = std::chrono::steady_clock::now();
	for (;;) {
		[app updateWindows];
		if (frame.fn) {
			frame.fn();
		}
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
