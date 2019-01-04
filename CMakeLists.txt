cmake_minimum_required(VERSION 3.7.1)
project(sgcp VERSION 0.0.1 LANGUAGES CXX)

# Common part

set(WARNING_OPTIONS             -Wall -Wextra -Werror)
set(ARCH_OPTIONS                -march=native -mtune=native)
set(GCC_WARNING_OPTIONS         -Wduplicated-cond -Wlogical-op -Wuseless-cast -Wrestrict)
set(RELEASE_OPTIONS             -O3)
set(DEBUG_OPTIONS               -O0 -g)
set(GDB_DEBUG_OPTIONS           -ggdb)
set(LINKER_OPTIONS              -flto -Wl,--no-as-needed)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Cplex)
include_directories(SYSTEM ${CPLEX_INCLUDE_DIRS})

find_package(Boost COMPONENTS filesystem)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

find_package(Exactcolors)
include_directories(SYSTEM ${EXACTCOLORS_INCLUDE_DIRS})

# Target 1: Solver base-sgcp

file(GLOB_RECURSE BASE_HEADERS base-sgcp/source-code/*.h)
file(GLOB_RECURSE BASE_TUS base-sgcp/source-code/*.cpp)
set(BASE_SOURCES ${BASE_HEADERS} ${BASE_TUS})
add_executable(sgcp ${BASE_SOURCES})

target_compile_features(    sgcp PRIVATE cxx_std_17)
target_compile_options(     sgcp PRIVATE ${WARNING_OPTIONS} ${ARCH_OPTIONS})
target_compile_options(     sgcp PRIVATE $<$<CXX_COMPILER_ID:GNU>:${GCC_WARNING_OPTIONS}>)
target_compile_options(     sgcp PRIVATE $<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>)
target_compile_options(     sgcp PRIVATE $<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>)
target_compile_options(     sgcp PRIVATE $<$<AND:$<CONFIG:DEBUG>,$<CXX_COMPILER_ID:GNU>>:${GDB_DEBUG_OPTIONS}>)
target_link_libraries(      sgcp PRIVATE ${LINKER_OPTIONS})
target_link_libraries(      sgcp PRIVATE ${CPLEX_LIBRARIES})
target_link_libraries(      sgcp PRIVATE ${EXACTCOLORS_LIBRARIES})
target_link_libraries(      sgcp PRIVATE Boost::filesystem)
target_link_libraries(      sgcp PRIVATE dl)

# Target 2: Solver min-weight-sgcp

file(GLOB MW_HEADERS min-weight-sgcp/source-code/*.h)
file(GLOB MW_TUS min-weight-sgcp/source-code/*.cpp)
set(MW_SOURCES ${MW_HEADERS} ${MW_TUS})
add_executable(mwsgcp ${MW_SOURCES})

target_compile_features(    mwsgcp PRIVATE cxx_std_17)
target_compile_options(     mwsgcp PRIVATE ${WARNING_OPTIONS} ${ARCH_OPTIONS})
target_compile_options(     mwsgcp PRIVATE $<$<CXX_COMPILER_ID:GNU>:${GCC_WARNING_OPTIONS}>)
target_compile_options(     mwsgcp PRIVATE $<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>)
target_compile_options(     mwsgcp PRIVATE $<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>)
target_compile_options(     mwsgcp PRIVATE $<$<AND:$<CONFIG:DEBUG>,$<CXX_COMPILER_ID:GNU>>:${GDB_DEBUG_OPTIONS}>)
target_link_libraries(      mwsgcp PRIVATE ${LINKER_OPTIONS})
target_link_libraries(      mwsgcp PRIVATE ${CPLEX_LIBRARIES})
target_link_libraries(      mwsgcp PRIVATE ${EXACTCOLORS_LIBRARIES})
target_link_libraries(      mwsgcp PRIVATE dl)
target_link_libraries(      mwsgcp PRIVATE stdc++fs)