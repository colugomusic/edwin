cmake_minimum_required(VERSION 3.20)
project(edwin)
list (APPEND edwin_src
	include/edwin.hpp
	include/edwin-ext.hpp
	include/edwin-object.hpp
	$<$<BOOL:${APPLE}>:src/edwin-mac.mm>
	$<$<BOOL:${LINUX}>:src/edwin-lin.cpp>
	$<$<BOOL:${WIN32}>:src/edwin-win.cpp>
)
add_library(edwin STATIC ${edwin_src})
add_library(edwin::edwin ALIAS edwin)
if (LINUX)
	find_package(X11 REQUIRED)
endif()
target_include_directories(edwin PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(edwin PUBLIC
	$<$<BOOL:${WIN32}>:dwmapi>
	$<$<BOOL:${LINUX}>:X11::X11>
	"-framework Cocoa"
)
set_target_properties(edwin PROPERTIES
	CXX_STANDARD 20
)
