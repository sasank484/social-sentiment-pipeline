# Requires vcpkg with curl[ssl], nlohmann-json, spdlog installed
cmake_minimum_required(VERSION 3.20)
project(ingest_yt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg integration
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

add_executable(ingest_yt
src/main.cpp
src/youtube_client.cpp
src/writer.cpp
src/http_client.cpp
src/youtube_errors.cpp
)

	target_include_directories(ingest_yt PRIVATE include)
	target_link_libraries(ingest_yt PRIVATE CURL::libcurl nlohmann_json::nlohmann_json spdlog::spdlog)
