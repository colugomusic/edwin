#include "edwin.h"
#include <Cocoa/Cocoa.h>

namespace edwin {

@interface EdwinWindow : NSWindow
	window* wnd = nullptr;
@end

@implementation EdwinWindow
- (void) windowDidResize: (NSNotification*) notification {
	NSRect frame = [self frame];
	if (wnd->on_window_resized.fn) {
		wnd->on_window_resized.fn(frame.size.width, frame.size.height);
	}
}
- (void) windowWillResize: (NSWindow*) sender toSize: (NSSize) frameSize {
	if (wnd->on_window_resizing.fn) {
		wnd->on_window_resizing.fn(frameSize.width, frameSize.height);
	}
}
- (void) windowWillClose: (NSNotification*) notification {
	if (wnd->on_window_closed.fn) {
		wnd->on_window_closed.fn();
	}
}
@end

struct window {
	EdwinWindow* nswindow = nullptr;
	NSView* nsview        = nullptr;
	fn::on_window_closed on_window_closed;
	fn::on_window_resized on_window_resized;
	fn::on_window_resizing on_window_resizing;
};

static fn::frame app_frame_;
static bool app_schedule_stop_ = false;

auto create(window_config cfg) -> window* {
	auto wnd = std::make_unique<window>();
	const auto rect = NSMakeRect(cfg.position.x, cfg.position.y, cfg.size.width, cfg.size.height);
	wnd->nswindow = [[EdwinWindow alloc]
		initWithContentRect: rect
		styleMask:           NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
		backing:             NSBackingStoreBuffered
		defer:               NO
	];
	wnd->nswindow->wnd = wnd.get();
	wnd->nsview        = [[NSView alloc] initWithFrame: rect];
	[wnd->nswindow setContentView: wnd->nsview];
	set(wnd.get(), cfg.title);
	set(wnd.get(), cfg.resizable);
	set(wnd.get(), cfg.visible);
	set(wnd.get(), cfg.on_window_closed);
	set(wnd.get(), cfg.on_window_resized);
	set(wnd.get(), cfg.on_window_resizing);
	return wnd.release();
}

auto destroy(window* wnd) -> void {
	if (!wnd)          { return; }
	if (wnd->nswindow) { [wnd->nswindow close]; })
	if (wnd->nsview)   { [wnd->nsview release]; })
	delete wnd;
}

auto get_native_handle(const window& wnd) -> native_handle {
	return {wnd.nsview};
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
	wnd->nswindow->on_window_closed = cb;
}

auto set(window* wnd, fn::on_window_resized cb) -> void {
	wnd->nswindow->on_window_resized = cb;
}

auto set(window* wnd, fn::on_window_resizing cb) -> void {
	wnd->nswindow->on_window_resizing = cb;
}

auto process_messages() -> void {
	const auto run_loop = [NSRunLoop currentRunLoop];
	[run_loop
		runMode:    NSDefaultRunLoopMode
		beforeDate: [NSDate distantFuture]
	];
}

auto app_beg(edwin::fn::frame frame, edwin::frame_interval interval) -> void {
	app_frame_ = frame;
	const auto run_loop = [NSRunLoop currentRunLoop];
	auto next_frame = std::chrono::steady_clock::now();
	for (;;) {
		[run_loop
			runMode:    NSDefaultRunLoopMode
			beforeDate: [NSDate distantFuture]
		];
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
