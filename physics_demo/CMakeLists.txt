cmake_minimum_required(VERSION 3.20)
project(PhysicsDemo LANGUAGES CXX)

if(NOT TARGET Engine::engine)
    find_package(Engine REQUIRED)
endif()

include(FetchContent)

set(TARGET_UNIT_TESTS       OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD      OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES          OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER           OFF CACHE BOOL "" FORCE)
set(ENABLE_ALL_WARNINGS     OFF CACHE BOOL "" FORCE)
set(JPH_USE_VK              OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG        v5.6.0
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  Build
)
FetchContent_MakeAvailable(JoltPhysics)

add_executable(physics_demo src/main.cpp)

set_target_properties(physics_demo PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(physics_demo PRIVATE Engine::engine Jolt)

add_custom_command(TARGET physics_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${Engine_ASSET_DIR} $<TARGET_FILE_DIR:physics_demo>
    COMMENT "Copying engine assets"
)
