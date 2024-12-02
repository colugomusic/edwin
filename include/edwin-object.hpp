#pragma once

#include "edwin.hpp"
#include <stdexcept>

namespace edwin {

struct object {
	object() = delete;
	object(window_config cfg) : wnd_{create(cfg)} {
		if (!wnd_) {
			throw std::runtime_error("Failed to create window.");
		}
	}
	~object() {
		if (wnd_) {
			destroy(wnd_);
		}
	}
	object(const object&) = delete;
	object& operator=(const object&) = delete;
	object(object&& rhs) noexcept : wnd_{rhs.wnd_} { rhs.wnd_ = nullptr; }
	object& operator=(object&& rhs) noexcept {
		if (this != &rhs) {
			if (wnd_) {
				destroy(wnd_);
			}
			wnd_ = rhs.wnd_;
			rhs.wnd_ = nullptr;
		}
		return *this;
	}
	[[nodiscard]] auto get() const noexcept -> window* {
		return wnd_;
	}
private:
	window* wnd_ = nullptr;
};

} // edwin
