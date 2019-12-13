# DUMMY CMAKELISTS, Sigma is built with meson. See per subproject folders for cmakelists files, this is only for CLion
cmake_minimum_required(VERSION 3.10)

project(Sigma)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

file(GLOB_RECURSE SOURCE_FILES_1 ./*.cpp ./*.c ./*.asm ./*.S)

include_directories(kernel/include)
include_directories(libs/libsigma/include)
include_directories(zeta/include)

add_executable(Sigma ${SOURCE_FILES_1})