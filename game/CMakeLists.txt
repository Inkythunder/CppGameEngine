cmake_minimum_required(VERSION 3.20)
project(Game LANGUAGES CXX)

if(NOT TARGET Engine::engine)
    find_package(Engine REQUIRED)
endif()

add_executable(game src/main.cpp)

set_target_properties(game PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(game PRIVATE Engine::engine)

add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:game>
    COMMENT "Copying game assets"
)

add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${Engine_ASSET_DIR} $<TARGET_FILE_DIR:game>
    COMMENT "Copying engine assets"
)
