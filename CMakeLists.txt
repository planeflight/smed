cmake_minimum_required(VERSION 3.5)

project(smed)

file(GLOB_RECURSE SRC
    "smed/*.cpp"
    "smed/*.hpp"
)

set (CMAKE_CXX_STANDARD 20)

include_directories(".")
include_directories("./lib/")
include_directories("./lib/freetype/include/")
include_directories("../games/omega/")
include_directories("../games/omega/lib/SDL3/include/")
include_directories("../games/omega/lib/SDL_image/include/")
include_directories("../games/omega/lib/SDL_mixer/include/")
include_directories("../games/omega/lib/glad/include/")
include_directories("../games/omega/lib/box2d/include/")

add_executable(${PROJECT_NAME} ${SRC})
set(CMAKE_BUILD_TYPE Debug)

target_include_directories(${PROJECT_NAME}
    PUBLIC ../games/omega/
    PUBLIC ../games/omega/lib/
    PUBLIC ../games/omega/lib/SDL/include/
    PUBLIC ../games/omega/lib/SDL_image/include/
    PUBLIC ../games/omega/lib/SDL_mixer/include/
)

target_link_libraries(${PROJECT_NAME}
    omega
    freetype
    SDL3
    SDL3_image
    SDL3_mixer
    imgui
    glad
    stb
    box2d
    libtmx-parser
    tomlplusplus
    entt
)

target_link_directories(${PROJECT_NAME}
    PUBLIC ../games/omega/build/
    PUBLIC ../games/omega/build/lib/SDL/
    PUBLIC ../games/omega/build/lib/SDL_image/
    PUBLIC ../games/omega/build/lib/SDL_mixer/
    PUBLIC ../games/omega/build/lib/imgui/
    PUBLIC ../games/omega/build/lib/glad/
    PUBLIC ../games/omega/build/lib/stb/
    PUBLIC ../games/omega/build/lib/box2d/bin/
    PUBLIC ../games/omega/build/lib/libtmx-parser/
    PUBLIC ../games/omega/build/lib/tomlplusplus/
    PUBLIC ../games/omega/build/lib/json/
    PUBLIC ../games/omega/build/lib/entt/
)
