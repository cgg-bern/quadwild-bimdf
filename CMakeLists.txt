cmake_minimum_required (VERSION 3.9)
cmake_policy(SET CMP0069 NEW) # honor INTERPROCEDURAL_OPTIMIZATION

project (Quadwild
    VERSION 0.0.2
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

set(BUILD_SHARED_LIBS OFF) # static libs make distribution easier
if(WIN32)
    add_definitions(/D_USE_MATH_DEFINES /DNOMINMAX)
    #vcglib has some broken debug code enabled with _DEBUG
    #add_compile_options(/U_DEBUG)
    # XXX: hacky workaround, seems i cannot undefine _DEBUG even in a release build?!
    add_definitions(/DHETYPE=auto)
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

find_package(Threads)
find_package(Gurobi QUIET)

add_library(Eigen3::Eigen INTERFACE IMPORTED)
target_include_directories(Eigen3::Eigen INTERFACE "libs/eigen")

find_package(GMM QUIET)

if (NOT TARGET GMM::GMM)
    FetchContent_Declare(gmm
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        URL http://download-mirror.savannah.gnu.org/releases/getfem/stable/gmm-5.4.2.tar.gz
        URL_HASH SHA224=8f4951901a55a1d1987d8199ed0e0b36ff2da26c870a4c3d55694a14
        SOURCE_DIR "libs/gmm"
    )
    FetchContent_MakeAvailable(gmm)

    message("Downloaded GMM to ${gmm_SOURCE_DIR}")
    set(GMM_INCLUDE_DIR "${gmm_SOURCE_DIR}/include")
    find_package(GMM REQUIRED)
endif()

option(SATSUMA_ENABLE_BLOSSOM5 "Enable Blossom-V (non-free license)" OFF)

add_subdirectory("libs/lemon")
if(SATSUMA_ENABLE_BLOSSOM5)
    add_subdirectory("libs/blossom5-cmake")
endif()
add_subdirectory("libs/satsuma")
add_subdirectory("libs/timekeeper/src")
add_subdirectory("libs/json")
set(OPENMESH_BUILD_SHARED OFF) # required on windows
add_subdirectory("libs/OpenMesh")
if(WIN32)
    add_library( OpenMesh::Core ALIAS OpenMeshCore)
else()
    add_library( OpenMesh::Core ALIAS OpenMeshCoreStatic)
endif()
add_subdirectory("libs/lpsolve")


set( WANT_COMISO_QT OFF CACHE BOOL "Enable Qt support in CoMISo")
set( DISABLE_MPI ON)
set(COMISO_ENABLE_METIS   OFF CACHE BOOL "Build with METIS (if found)" )
set(COMISO_ENABLE_MUMPS   OFF CACHE BOOL "Build with MUMPS (if found)" )
set(COMISO_ENABLE_LPSOLVE OFF CACHE BOOL "Build with lpsolve (if found)" )
set(COMISO_ENABLE_GUROBI  OFF CACHE BOOL "Build with Gurobi (if found)" )
set( COMISO_BUILD_WITHOUT_BOOST ON)
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
