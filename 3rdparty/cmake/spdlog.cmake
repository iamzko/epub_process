cmake_minimum_required( VERSION 3.0 )

include(FetchContent)

FetchContent_Declare(
    spdlog
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spdlog-1.8.5
)

Fetchcontent_GetProperties(spdlog)
FetchContent_MakeAvailable(spdlog)
set(SPDLOG_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/spdlog-1.8.5/include/ CACHE INTERNAL "")