cmake_minimum_required(VERSION 3.20)
project(edwin)
add_library(edwin STATIC ${edwin_src})
add_library(edwin::edwin ALIAS edwin)
target_sources(edwin PRIVATE
	$<$<BOOL:${APPLE}>:src/edwin-mac.mm>
	$<$<BOOL:${LINUX}>:src/edwin-lin.cpp>
	$<$<BOOL:${WIN32}>:src/edwin-win.cpp>
)
target_sources(edwin PUBLIC
	FILE_SET HEADERS
	BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
	FILES
		${CMAKE_CURRENT_SOURCE_DIR}/include/edwin.hpp
		${CMAKE_CURRENT_SOURCE_DIR}/include/edwin-ext.hpp
		${CMAKE_CURRENT_SOURCE_DIR}/include/edwin-object.hpp
)
if (LINUX)
	find_package(X11 REQUIRED)
endif()
target_link_libraries(edwin PUBLIC
	$<$<BOOL:${WIN32}>:dwmapi>
	$<$<BOOL:${LINUX}>:X11::X11>
)
if (APPLE)
	target_link_libraries(edwin PUBLIC
		"-framework Cocoa"
	)
endif()
set_target_properties(edwin PROPERTIES CXX_STANDARD 20)
include(CMakePackageConfigHelpers)
install(TARGETS edwin EXPORT edwin-targets FILE_SET HEADERS DESTINATION include/edwin)
install(EXPORT edwin-targets FILE edwin-targets.cmake NAMESPACE edwin:: DESTINATION lib/cmake/edwin)
configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/edwin-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/edwin-config.cmake"
    INSTALL_DESTINATION lib/cmake/edwin
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/edwin-config.cmake" DESTINATION lib/cmake/edwin)