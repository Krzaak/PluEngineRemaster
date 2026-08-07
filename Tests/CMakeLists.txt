project(PluTests)

# ThreadSanitizer build of the tests. Opt-in, because TSan roughly halves the speed and
# needs its own binary — the stress suites are where it earns its keep (striped map,
# lock-free array, SPSC ring).
option(PLU_TESTS_TSAN "Build the tests with ThreadSanitizer" OFF)

add_executable(PluSTLTests
        PluSTLTests/Main.cpp
        PluSTLTests/Test_Queue.cpp
        PluSTLTests/Test_ConcurrentHashMap.cpp
        PluSTLTests/Test_ConcurrentHashSet.cpp
        PluSTLTests/Test_ConcurrentQueue.cpp
        PluSTLTests/Test_ConcurrentArray.cpp
        PluSTLTests/Test_ConcurrentString.cpp
)

target_include_directories(PluSTLTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/PluSTLTests)

# Pure container tests — no engine, no window, no GL. PluSTL brings the include dir and
# Threads::Threads with it.
target_link_libraries(PluSTLTests PRIVATE PluSTL)

target_compile_features(PluSTLTests PRIVATE cxx_std_20)

if(PLU_TESTS_TSAN)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(PluSTLTests PRIVATE -fsanitize=thread -g -O1 -fno-omit-frame-pointer)
        target_link_options(PluSTLTests PRIVATE -fsanitize=thread)
    else()
        message(WARNING "PLU_TESTS_TSAN is only supported on GCC/Clang — building without it")
    endif()
endif()

enable_testing()
add_test(NAME PluSTLTests COMMAND PluSTLTests)
