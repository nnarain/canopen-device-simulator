cmake_minimum_required(VERSION 3.16)

project(canopen-device-simulator)

set_directory_properties(PROTPERTIES COMPILE_OPTIONS "-std=c++17;-Wall;-Wextra;-Werror")

# Find Lua and SOL2
find_package(Lua REQUIRED)
find_package(SOL2 REQUIRED)

# Global include for SOL2 to find lua.h
include_directories(
    ${LUA_INCLUDE_DIR}
)

# Setup Lely CANopen
find_package(PkgConfig REQUIRED)
pkg_check_modules(LELY_CANOPEN REQUIRED IMPORTED_TARGET liblely-coapp)

# Boost for program options
find_package(Boost COMPONENTS program_options REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    sol2
)

target_link_libraries(${PROJECT_NAME}
    PkgConfig::LELY_CANOPEN
    ${LUA_LIBRARIES}
    ${Boost_LIBRARIES}
)
