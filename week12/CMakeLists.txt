cmake_minimum_required(VERSION 3.15)
project(cg_week12 CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

add_executable(cg main.cpp)

# stb_image.h external/stb/ ішінде
target_include_directories(cg PRIVATE external/stb)

# assets/ жолы кодқа беріледі, сондықтан бағдарламаны кез келген бумадан іске қосуға болады
target_compile_definitions(cg PRIVATE ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")

target_link_libraries(cg PRIVATE glfw GLEW::GLEW OpenGL::GL)
