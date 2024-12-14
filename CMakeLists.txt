cmake_minimum_required(VERSION 3.2)

project(smed)

file(GLOB_RECURSE SRC
    "smed/*.cpp"
    "smed/*.hpp"
)

SET (CMAKE_CXX_FLAGS "-Wall -Wextra -Wcast-qual -Wconversion-null -Wformat-security \
-Wmissing-declarations -Woverlength-strings -Wpointer-arith -Wundef \
-Wunused-local-typedefs -Wunused-result -Wvarargs -Wvla \
-Wwrite-strings -DNOMINMAX -fno-omit-frame-pointer \
-std=c++20 -fPIC -g")

include_directories(".")
include_directories("./lib/")
include_directories("../games/omega/")
include_directories("../games/omega/lib/")
include_directories("../games/omega/lib/glad/include/")
include_directories("../games/omega/lib/box2d/include/")

add_executable(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME}
    PUBLIC lib/omega/
)

target_link_libraries(${PROJECT_NAME}
    omega
    imgui
    glad
    box2d
    stb
    libtmx-parser
    SDL2
)

target_link_directories(${PROJECT_NAME}
    PUBLIC ../games/omega/build/
    PUBLIC ../games/omega/build/lib/imgui/
    PUBLIC ../games/omega/build/lib/box2d/bin
    PUBLIC ../games/omega/build/lib/glad/
    PUBLIC ../games/omega/build/lib/stb/
    PUBLIC ../games/omega/build/lib/libtmx-parser/
)
