This is C++20 library for opening a window on Windows, macOS or Linux. It's intended for situations where you just need to be able to open a window and then pass the native window handle on to something which wants to "attach" to it in some way (in my case it is audio plugin editor GUIs.) I tried to make the code as simple as possible and easy to fork and modify for your use case.

APIs used:
- Windows: Win32
- macOS: Cocoa
- Linux: Xlib

# Usage
Add it as a cmake subproject and link to `edwin::edwin`. Then `#include <edwin.hpp>` in your code. There is some documentation [in there](https://github.com/colugomusic/edwin/blob/master/include/edwin.hpp).

# Alternative libraries that I didn't like

I tried several existing libraries and had problems with all of them, but if edwin isn't ideal for your use case then maybe one of these is:

- CrossWindow: https://github.com/alaingalvan/CrossWindow
- choc::ui::DesktopWindow: https://github.com/Tracktion/choc/blob/main/gui/choc_DesktopWindow.h
- nappgui: https://nappgui.com/en/home/web/home.html
