cmake_minimum_required(VERSION 3.16)
project(turbojang C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------- raylib
# Agar raylib manbasi -DRAYLIB_SRC=... bilan berilmasa, GitHub'dan yuklab olinadi.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
set(CUSTOMIZE_BUILD OFF CACHE BOOL "" FORCE)

if(RAYLIB_SRC)
    add_subdirectory(${RAYLIB_SRC} raylib_build)
else()
    include(FetchContent)
    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    FetchContent_Declare(raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz)
    FetchContent_MakeAvailable(raylib)
endif()

set(GAME_SOURCES
    src/main.c
    src/game.c
    src/net.c
    src/plat.c
    src/track.c
    src/parking.c
    src/fonts_data.c)

# ---------------------------------------------------------------- Android
if(ANDROID)
    # NativeActivity "libturbojang.so" ni yuklaydi (AndroidManifest.xml ga qarang)
    add_library(turbojang SHARED ${GAME_SOURCES})
    target_link_libraries(turbojang raylib android log EGL GLESv2 OpenSLES m)
    target_link_options(turbojang PRIVATE -u ANativeActivity_onCreate)
    return()
endif()

# ---------------------------------------------------------------- Desktop
add_executable(turbojang ${GAME_SOURCES})
target_link_libraries(turbojang raylib)

if(WIN32)
    target_sources(turbojang PRIVATE src/turbojang.rc)   # .exe ikonkasi
    target_link_libraries(turbojang ws2_32 winmm)
    if(MINGW)
        # konsol oynasisiz ishga tushadi, kutubxonalar exe ichiga joylanadi
        target_link_options(turbojang PRIVATE -mwindows -static -static-libgcc)
    elseif(MSVC)
        target_link_options(turbojang PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
    endif()
elseif(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(turbojang Threads::Threads m)
endif()

# ---------------------------------------------------------------- Test
add_executable(net_test tests/net_test.c src/game.c src/track.c src/net.c src/plat.c)
if(WIN32)
    target_link_libraries(net_test ws2_32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(net_test Threads::Threads m)
endif()

add_executable(parking_test tests/parking_test.c src/parking.c)
target_include_directories(parking_test PRIVATE src)
if(NOT WIN32)
    target_link_libraries(parking_test m)
endif()
