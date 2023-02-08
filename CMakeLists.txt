cmake_minimum_required(VERSION 3.15)
project(jit)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -Wall -O3")
# see https://cmake.org/cmake/help/latest/module/FindBoost.html
find_package(Boost REQUIRED unit_test_framework date_time)
if (Boost_FOUND)
    message("BOOST FOUND")
    message("VERSION: ${Boost_VERSION}")
    message("INCLUDE: ${Boost_INCLUDE_DIRS}")
    message("LINK: ${Boost_LIBRARY_DIRS}")
endif()

include_directories(src)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})
add_subdirectory(Test)

add_executable(jit main.cpp)
target_link_libraries(jit boost_chrono)

# enable testing
enable_testing()

add_test(
        NAME utest
        COMMAND Boost_Tests_run
)

