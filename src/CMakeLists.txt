cmake_minimum_required(VERSION 3.12)

project(vulkan)

set(ASAN -fsanitize=address -fsanitize=undefined)

set(SRC src/main.cpp)
set(CXXFLAGS 
-g -Wall -Wextra -Werror -pedantic 
-Wno-unused-parameter -Wno-error=unused-variable 
-Wno-missing-field-initializers
-std=c++20 
${ASAN})
set(LIBS vulkan glfw)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(LDFLAGS ${ASAN})

add_executable(main ${SRC})
target_link_libraries(main ${LIBS} ${LDFLAGS})
target_compile_options(main PUBLIC ${CXXFLAGS})
