cmake_minimum_required(VERSION 3.30)
project(AT VERSION 1.0.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_SUPPRESS_REGENERATION TRUE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set (SOURCES src/main.cpp
	)
set (HEADER 
)

source_group("Source files/Source" FILES ${SOURCES}) 
source_group("Header files/Header" FILES ${HEADER})

add_executable(${PROJECT_NAME} ${SOURCES} )

find_package(jsoncpp REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json)

target_link_libraries(${PROJECT_NAME} PRIVATE jsoncpp_lib CURL::libcurl nlohmann_json::nlohmann_json) 

