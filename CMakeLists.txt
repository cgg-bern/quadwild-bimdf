cmake_minimum_required (VERSION 3.9)
cmake_policy(SET CMP0069 NEW) # honor INTERPROCEDURAL_OPTIMIZATION

project (Quadwild
    VERSION 0.0.1
    LANGUAGES C CXX # OpenMesh cmake does not work without C enabled
    )

list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-library/finders)
set(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Build/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Build/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Build/lib")

include(CheckIPOSupported)
check_ipo_supported()

find_package(Threads)
find_package(Boost REQUIRED)
find_package(Gurobi QUIET)

add_library(Eigen3::Eigen INTERFACE IMPORTED)
target_include_directories(Eigen3::Eigen INTERFACE "libs/eigen")


add_subdirectory("libs/lemon")
add_subdirectory("libs/blossom5-cmake")
add_subdirectory("libs/satsuma")
add_subdirectory("libs/timekeeper/src")
add_subdirectory("libs/json")
add_subdirectory("libs/OpenMesh")
add_subdirectory("libs/lpsolve")

set( WANT_COMISO_QT OFF CACHE BOOL "Enable Qt support in CoMISo")
add_subdirectory("libs/CoMISo")
#add_subdirectory("libs/AntTweakBar") # vcglib wrapper is not compiled with vcglib's cmake :(

#find_package(LPSolve)

add_library(libigl INTERFACE IMPORTED) # "libs/libigl/include/igl/principal_curvature.cpp")
target_include_directories(libigl INTERFACE "libs/libigl/include")
target_link_libraries(libigl INTERFACE Eigen3::Eigen)
target_link_libraries(libigl INTERFACE Threads::Threads)
add_library(libigl::libigl ALIAS libigl)

add_library(vcglib "libs/vcglib/wrap/ply/plylib.cpp")
target_include_directories(vcglib PUBLIC "libs/vcglib")
target_link_libraries(vcglib PUBLIC Eigen3::Eigen)
target_link_libraries(vcglib PUBLIC libigl::libigl)
add_library(vcglib::vcglib ALIAS vcglib)

add_subdirectory("libs/quadretopology")
#add_subdirectory("libs/vcglib") 
add_library(quadwild::xfield_tracer INTERFACE IMPORTED)
target_include_directories(quadwild::xfield_tracer INTERFACE "libs/xfield_tracer")
target_link_libraries(quadwild::xfield_tracer INTERFACE vcglib::vcglib)
target_link_libraries(quadwild::xfield_tracer INTERFACE vcglib::vcglib)
#add_subdirectory("components/field_tracing")
add_subdirectory("components/quad_from_patches")
add_subdirectory("components/field_computation")
add_subdirectory("components/viz_mesh_results")



add_subdirectory(quadwild)
