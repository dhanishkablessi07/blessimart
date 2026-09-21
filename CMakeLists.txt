cmake_minimum_required(VERSION 3.20)
project(BlessiMart CXX)

# Use C++20 for all targets
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Drogon (must be installed first, see README.md)
find_package(Drogon CONFIG REQUIRED)

# Collect all .cc files in src/
file(GLOB_RECURSE SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cc
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

# Include headers in src/ (controllers, models, filters)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Link Drogon
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)
