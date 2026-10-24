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
    src/native_chrome.cpp
    src/native_ui_loop.cpp
    src/PS4b_main.cpp
    src/shader_table.cpp
    src/surface.cpp
    src/swgl_canvas.cpp
    src/touch_cursor.cpp
    src/ui.cpp
    src/ps4/homebrew_return.cpp
    src/ps4/litehtml_wrapper.cpp
    src/ps4/main.cpp
    src/ps4/mesa_runtime_bridge.cpp
    src/ps4/modules.cpp
    src/ps4/native_allocator.cpp
    src/ps4/native_audio.cpp
    src/ps4/native_clock.cpp
    src/ps4/native_cpu.cpp
    src/ps4/native_fcntl.cpp
    src/ps4/native_jit.cpp
    src/ps4/native_keyboard.cpp
    src/ps4/native_mesa.cpp
    src/ps4/native_mesa_probe.cpp
    src/ps4/native_network.cpp
    src/ps4/native_stat.cpp
    src/ps4/native_stdio.cpp
    src/ps4/piglet.cpp
    src/ps4/platform.cpp
    src/ps4/ps4_browser_main.cpp
    src/ps4/run_logs.cpp
    src/ps4/rust_runtime.cpp
    src/ps4/sdk_heap_memory.cpp
    src/ps4/services.cpp
    src/ps4/startup.cpp
    src/ps4/videoout.cpp
    src/ps4/mesa_gnm_stubs.S
)

# Set header files
set(PS4_HEADERS
    src/ps4/litehtml_wrapper.h
    src/include/
)

# Create the executable
add_executable(ps4fox ${PS4_SOURCES} ${PS4_HEADERS})

# Include directories
target_include_directories(ps4fox PUBLIC
    src
    src/include
    tp/litehtml/include
)

# Link against litehtml
target_link_libraries(ps4fox PRIVATE litehtml)

# Set output name
set_target_properties(ps4fox PROPERTIES OUTPUT_NAME "ps4fox")