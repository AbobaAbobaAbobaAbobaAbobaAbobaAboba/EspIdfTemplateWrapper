# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)



include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(project-name)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/copy_dir_done
    COMMAND bash deploy_vue.sh vue_working_folder/dist littlefs
    COMMAND touch ${CMAKE_BINARY_DIR}/copy_dir_done
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running copy_dir.sh to copy files"
)
add_custom_target(
    CopyDir
    ALL  # Ensures this target is built every time
    DEPENDS ${CMAKE_BINARY_DIR}/copy_dir_done
)
