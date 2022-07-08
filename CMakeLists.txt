project(ConcurrencyInActionCpp)

cmake_minimum_required(VERSION 3.0)

include_directories ( Common )

#set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE)

set(BOOST_ROOT "$ENV{THIRD_PARTY_LIB}/boost_1_72_0")
set(BOOST_INCLUDE_DIR ${BOOST_ROOT})
set(BOOST_LIBRARY_DIR ${BOOST_ROOT}/stage/lib)

include_directories(${BOOST_INCLUDE_DIR})
link_directories(${BOOST_LIBRARY_DIR})

add_executable(exec "ch2/launching_thread.cpp")
target_link_libraries(exec ${BOOST_LIBRARIES})