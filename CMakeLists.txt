cmake_minimum_required(VERSION 3.15)
project(mop)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -Wall -O3 -fopenmp -lpthread")
# see https://cmake.org/cmake/help/latest/module/FindBoost.html
find_package(Boost REQUIRED unit_test_framework date_time)
if (Boost_FOUND)
    message("BOOST FOUND")
    message("VERSION: ${Boost_VERSION}")
    message("INCLUDE: ${Boost_INCLUDE_DIRS}")
    message("LINK: ${Boost_LIBRARY_DIRS}")
endif()
find_package(OpenMP REQUIRED)
if (OpenMP_FOUND)
    message("OMP FOUND")
include_directories(src)
endif()

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})
add_subdirectory(Test)

add_executable(mop main.cpp)
target_link_libraries(mop boost_chrono)

# enable testing
enable_testing()

add_test(
        NAME utest
        COMMAND Boost_Tests_run
)

