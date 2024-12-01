#include "edwin.h"
#include <Cocoa/Cocoa.h>

namespace edwin {

@interface EdwinWindow : NSWindow
	fn::on_window_closed on_window_closed;
	fn::on_window_resized on_window_resized;
	fn::on_window_resizing on_window_resizing;
@end

@implementation EdwinWindow
- (void) windowDidResize: (NSNotification*) notification {
	NSRect frame = [self frame];
	if (on_window_resized.fn) {
		on_window_resized.fn(frame.size.width, frame.size.height);
	}
}
- (void) windowWillResize: (NSWindow*) sender toSize: (NSSize) frameSize {
	if (on_window_resizing.fn) {
		on_window_resizing.fn(frameSize.width, frameSize.height);
	}
}
- (void) windowWillClose: (NSNotification*) notification {
	if (on_window_closed.fn) {
		on_window_closed.fn();
	}
}
@end

struct window {
	EdwinWindow* impl = nullptr;
};

auto create(window_config cfg) -> window* {
	auto wnd = std::make_unique<window>();
	wnd->impl = [[EdwinWindow alloc]
		initWithContentRect: NSMakeRect(cfg.position.x, cfg.position.y, cfg.size.width, cfg.size.height)
		styleMask:           NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
		backing:             NSBackingStoreBuffered
		defer:               NO
	];
	set(wnd.get(), cfg.title);
	set(wnd.get(), cfg.resizable);
	set(wnd.get(), cfg.visible);
	set(wnd.get(), cfg.on_window_closed);
	set(wnd.get(), cfg.on_window_resized);
	set(wnd.get(), cfg.on_window_resizing);
	return wnd.release();
}

auto destroy(window* wnd) -> void {
	if (!wnd) { return; }
	if (wnd->impl) { [wnd->impl close]; })
	delete wnd;
}

auto get_native_handle(const window& wnd) -> native_handle {
	return [wnd.impl contentView];
}

auto set(window* wnd, edwin::icon icon) -> void {
	// No-op on macOS.
}

auto set(window* wnd, edwin::position position) -> void {
	auto frame = [wnd->impl frame];
	frame.origin.x = position.x;
	frame.origin.y = position.y;
	[wnd->impl setFrame: frame display: YES];
}

auto set(window* wnd, edwin::position position, edwin::size size) -> void {
	auto frame = [wnd->impl frame];
	frame.origin.x = position.x;
	frame.origin.y = position.y;
	frame.size.width = size.width;
	frame.size.height = size.height;
	[wnd->impl setFrame: frame display: YES];
}

auto set(window* wnd, edwin::resizable resizable) -> void {
	const auto mask = [wnd->impl styleMask];
	if (resizable.value) {
		[wnd->impl setStyleMask: mask | NSWindowStyleMaskResizable];
	} else {
		[wnd->impl setStyleMask: mask & ~NSWindowStyleMaskResizable];
	}
}

auto set(window* wnd, edwin::size size) -> void {
	auto frame = [wnd->impl frame];
	frame.size.width = size.width;
	frame.size.height = size.height;
	[wnd->impl setFrame: frame display: YES];
}

auto set(window* wnd, edwin::title title) -> void {
	[wnd->impl setTitle: [NSString stringWithUTF8String: title.value.data()]];
}

auto set(window* wnd, edwin::visible visible) -> void {
	if (visible.value) {
		[wnd->impl makeKeyAndOrderFront: nil];
	} else {
		[wnd->impl orderOut: nil];
	}
}

auto set(window* wnd, fn::on_window_closed cb) -> void {
	wnd->impl->on_window_closed = cb;
}

auto set(window* wnd, fn::on_window_resized cb) -> void {
	wnd->impl->on_window_resized = cb;
}

auto set(window* wnd, fn::on_window_resizing cb) -> void {
	wnd->impl->on_window_resizing = cb;
}

auto process_messages() -> void {
	// No-op on macOS.
}

auto app_beg(edwin::fn::frame frame, edwin::frame_interval interval) -> void {
	// No-op on macOS.
}

auto app_end() -> void {
	// No-op on macOS.
}

} // edwin
