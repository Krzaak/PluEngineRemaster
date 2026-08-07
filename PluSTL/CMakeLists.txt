project(PluSTL)

# Header-only library. LibEngine and PluEditor still consume PluSTL through their own
# include-dir + precompiled-header setup; this target is additive, and exists so that
# standalone consumers (Tests/) have something to link against.
add_library(PluSTL INTERFACE)
target_include_directories(PluSTL INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# PluSTL/Concurrent/ uses std::thread / std::mutex / std::atomic. On glibc >= 2.34 pthread
# is folded into libc so this is a no-op there, but it is what makes the containers link on
# every other toolchain.
find_package(Threads REQUIRED)
target_link_libraries(PluSTL INTERFACE Threads::Threads)
