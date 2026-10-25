cmake_minimum_required(VERSION 3.15)
project(ps4fox LANGUAGES C CXX)

# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the litehtml subdirectory from tp folder
add_subdirectory(tp/litehtml)

# PS4-specific compile definitions
add_definitions(-D_PS4)

# Set source files - modular browser core and ps4 platform
set(PS4_SOURCES
    src/artwork.cpp
    src/js_canvas.cpp
    src/PS4b.cpp
    src/browser_navigation.cpp
    src/net/http_client.cpp
    src/shader_table.cpp
    src/surface.cpp
    src/touch_cursor.cpp
    src/ui.cpp
    src/ps4/html_renderer.cpp
    src/ps4/modules.cpp
    src/ps4/native_audio.cpp
    src/ps4/platform.cpp
    src/ps4/run_logs.cpp
    src/ps4/services.cpp
    src/ps4/videoout.cpp
)

# Set header files
set(PS4_HEADERS
    src/browser_navigation.h
    src/net/http_client.h
    src/ps4/html_renderer.h
    include/
    src/include/
)

# Create the PS4b executable (the PS4 Browser target)
add_executable(PS4b ${PS4_SOURCES} ${PS4_HEADERS})

# PS4 cross-compilation flags for PS4b target only
target_compile_options(PS4b PRIVATE
    $<$<COMPILE_LANGUAGE:C>:--target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -isysroot ${CMAKE_CURRENT_SOURCE_DIR}/tp/openorbis -isystem ${CMAKE_CURRENT_SOURCE_DIR}/tp/openorbis/include>
    $<$<COMPILE_LANGUAGE:CXX>:--target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -isysroot ${CMAKE_CURRENT_SOURCE_DIR}/tp/openorbis -isystem ${CMAKE_CURRENT_SOURCE_DIR}/tp/openorbis/include -isystem ${CMAKE_CURRENT_SOURCE_DIR}/tp/openorbis/include/c++/v1>
)

target_link_options(PS4b PRIVATE
    -m elf_x86_64 -pie --script ${CMAKE_CURRENT_SOURCE_DIR}/tp/openorbis/link.x --eh-frame-hdr -L${CMAKE_CURRENT_SOURCE_DIR}/tp/openorbis/lib
)

# Add CRT1.o object file for PS4
target_link_libraries(PS4b PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tp/openorbis/lib/crt1.o
)

# Include directories
target_include_directories(PS4b PUBLIC
    include
    src
    src/include
    tp/litehtml/include
    tp/openorbis/include
)

# Link libraries
target_link_directories(PS4b PRIVATE
    tp/openorbis/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/tp/openorbis/lib
)

# OpenOrbis library stubs required for PS4 system calls
target_link_libraries(PS4b PRIVATE
    litehtml
    kernel
    SceSysmodule
    SceNet
    SceSsl
    SceHttp
    SceVideoOut
    SceAudioOut
    SceUserService
    ScePad
    SceSystemService
)

# Set output name to PS4b (matching the expected build/PS4b.self)
set_target_properties(PS4b PROPERTIES OUTPUT_NAME "PS4b")
