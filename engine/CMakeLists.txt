cmake_minimum_required(VERSION 3.20)
project(Engine VERSION 0.1.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(GenerateExportHeader)
include(CMakePackageConfigHelpers)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

set(TARGET_UNIT_TESTS       OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD      OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES          OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER           OFF CACHE BOOL "" FORCE)
set(ENABLE_ALL_WARNINGS     OFF CACHE BOOL "" FORCE)
set(JPH_USE_VK              OFF CACHE BOOL "" FORCE)
set(ENABLE_INSTALL          OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG        v5.6.0
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  Build
)
FetchContent_MakeAvailable(JoltPhysics)

set_target_properties(Jolt PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

add_library(engine SHARED
    src/engine.cpp
    src/open_gl.cpp
    src/obj_loader.cpp
    src/image.cpp
    src/stb_image_impl.cpp
    src/file_utils.cpp
    src/standard_shapes.cpp
    third_party/glad/gl.c
    src/physics.cpp
)
add_library(Engine::engine ALIAS engine)

set(Engine_ASSET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets
    CACHE INTERNAL "Engine asset directory")

set_target_properties(engine PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_VISIBILITY_PRESET hidden
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

generate_export_header(engine
    BASE_NAME ENGINE
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/engine/engine_export.h
)

target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(engine
    PUBLIC  glm::glm
    PRIVATE glfw OpenGL::GL
    PRIVATE Jolt
)

install(TARGETS engine
    EXPORT EngineTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}      # engine.dll (Windows)
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}      # libengine.so (Linux)
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}      # engine.lib import lib
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/engine/engine_export.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/engine)
install(DIRECTORY assets/ DESTINATION ${CMAKE_INSTALL_DATADIR}/engine/assets)

install(EXPORT EngineTargets
    FILE      EngineTargets.cmake
    NAMESPACE Engine::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Engine
)

set(ENGINE_INSTALL_ASSETDIR ${CMAKE_INSTALL_DATADIR}/engine/assets)

configure_package_config_file(
    cmake/EngineConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/EngineConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Engine
    PATH_VARS ENGINE_INSTALL_ASSETDIR
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/EngineConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/EngineConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/EngineConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Engine
)
