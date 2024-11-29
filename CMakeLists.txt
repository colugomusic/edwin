cmake_minimum_required(VERSION 3.20)
project(edwin)
list (APPEND src
	include/edwin.hpp
	include/edwin-object.hpp
	$<$<BOOL:${APPLE}>:src/edwin-mac.mm>
	$<$<BOOL:${LINUX}>:src/edwin-lin.cpp>
	$<$<BOOL:${WIN32}>:src/edwin-win.cpp>
)
add_library(edwin STATIC ${src})
add_library(edwin::edwin ALIAS edwin)
target_include_directories(edwin PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(edwin PUBLIC
	$<$<BOOL:${WIN32}>:dwmapi>
)
#if (APPLE)
#	target_link_libraries(edwin
#		"-framework Foundation"
#		"-framework Cocoa"
#		"-framework UniformTypeIdentifiers"
#	)
#endif()
set_target_properties(edwin PROPERTIES
	CXX_STANDARD 20
)
