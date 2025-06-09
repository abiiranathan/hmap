cmake_minimum_required(VERSION 3.30)

project(hmap VERSION 1.0.0 LANGUAGES C)
# Install rules
include(GNUInstallDirs)
enable_testing()

# Define the library
add_library(hmap STATIC hmap.c)
target_include_directories(hmap PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Define the test executable
add_executable(hmap_test hmap_test.c)
target_link_libraries(hmap_test PRIVATE hmap)

# Register test with ctest
add_test(NAME hmap_test COMMAND hmap_test)


# Install targets
install(TARGETS hmap
    EXPORT hmapTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES hmap.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Export targets for package config
install(EXPORT hmapTargets
    FILE hmapTargets.cmake
    NAMESPACE hmap::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hmap
)

# Optional: generate a basic package config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/hmapConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/hmapConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/hmapConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hmap
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/hmapConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/hmapConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hmap
)

# Generate and install pkg-config file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/hmap.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/hmap.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/hmap.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)