cmake_minimum_required(VERSION 3.16)

project(canopen-device-simulator)

set_directory_properties(PROTPERTIES COMPILE_OPTIONS "-std=c++17;-Wall;-Wextra;-Werror")

# Find Lua and SOL2
find_package(Lua REQUIRED)
find_package(sol2 REQUIRED)

# Global include for SOL2 to find lua.h
include_directories(SYSTEM
    ${LUA_INCLUDE_DIR}
    ${SOL2_INCLUDE_DIRS}
)

include_directories(
    include/
)

# Setup Lely CANopen
find_package(PkgConfig REQUIRED)
pkg_check_modules(LELY_CANOPEN REQUIRED IMPORTED_TARGET liblely-coapp)

# Boost for program options
find_package(Boost COMPONENTS program_options REQUIRED)

add_executable(co-sim
    src/main.cpp
)

target_link_libraries(co-sim
    PkgConfig::LELY_CANOPEN
    ${LUA_LIBRARIES}
    ${Boost_LIBRARIES}
    ${SOL2_LIBRARIES}
)
