cmake_minimum_required(VERSION 3.20)
project(ray CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

# Set C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fdiagnostics-color=always)
endif()

if(APPLE)
    # Raylib viene de Homebrew
    # Las cabeceras de raylib deben coincidir con la librería.
    find_package(raylib REQUIRED)
endif()

# Include directories
include_directories(
    include
    include/json
    src
)
if(NOT APPLE)
    include_directories(include/raylib)
endif()

# Collect all source files recursively from src/
file(GLOB_RECURSE SOURCES src/*.cpp)

# Define the executable target
add_executable(ray ${SOURCES})

if(EMSCRIPTEN)
    # --- WebAssembly Configuration ---

    # Link WebAssembly static libraries
    target_link_libraries(ray PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/web/libraylib.web.a
    )

    # Emscripten link flags & output extension (.html)
    set_target_properties(ray PROPERTIES
        SUFFIX ".html"
        LINK_FLAGS "-s USE_GLFW=3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s ASYNCIFY -s FORCE_FILESYSTEM=1"
    )

    target_link_options(ray PRIVATE "--pre-js" "${CMAKE_CURRENT_SOURCE_DIR}/src/web/api.js")
    set_property(TARGET ray APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/web/api.js")

    # Automatically preload assets folder if it contains files
    file(GLOB ASSET_FILES "assets/*")
    if(ASSET_FILES)
        target_link_options(ray PRIVATE "--preload-file" "${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets")
    endif()

elseif(APPLE)
    target_link_libraries(ray PRIVATE raylib)

else()
    # --- Native Linux Configuration ---

    # Link native static library and system graphics/windowing libraries
    target_link_libraries(ray PRIVATE
        raylib
        GL
        m
        pthread
        dl
        rt
        X11
    )
endif()
