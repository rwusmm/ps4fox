cmake_minimum_required(VERSION 3.15)
project(ps4fox LANGUAGES C CXX)

# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the litehtml subdirectory from tp folder
add_subdirectory(tp/litehtml)

# PS4-specific compile definitions
add_definitions(-D_PS4)

# Set source files - direct src/ and src/ps4/ subdirectory
set(PS4_SOURCES
    src/artwork.cpp
    src/js_canvas.cpp
    src/PS4b_main.cpp
    src/surface.cpp
    src/touch_cursor.cpp
    src/ui.cpp
    src/ps4/litehtml_wrapper.cpp
    src/ps4/modules.cpp
    src/ps4/native_audio.cpp
    src/ps4/piglet.cpp
    src/ps4/platform.cpp
    src/ps4/ps4_browser_main.cpp
    src/ps4/run_logs.cpp
    src/ps4/rust_runtime.cpp
    src/ps4/services.cpp
    src/ps4/startup.cpp
    src/ps4/videoout.cpp
)

# Set header files
set(PS4_HEADERS
    src/ps4/litehtml_wrapper.h
    src/include/
)

# Create the PS4b executable (the PS4 Browser target)
add_executable(PS4b ${PS4_SOURCES} ${PS4_HEADERS})

# Include directories
target_include_directories(PS4b PUBLIC
    src
    src/include
    tp/litehtml/include
)

# Link against litehtml
target_link_libraries(PS4b PRIVATE litehtml)

# Set output name to PS4b (matching the expected build/PS4b.self)
set_target_properties(PS4b PROPERTIES OUTPUT_NAME "PS4b")