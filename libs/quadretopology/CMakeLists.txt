
option(QUADRETOPOLOGY_WITH_GUROBI "Build libQuadRetopology with Gurobi" OFF)

set(WITH_GUROBI 0)

if(QUADRETOPOLOGY_WITH_GUROBI)
    if(TARGET Gurobi::GurobiCXX)
        set(WITH_GUROBI 1)
    else()
        message(WARNING "QUADRETOPOLOGY_WITH_GUROBI was specified, but Gurobi was not found.")
    endif()
endif()
configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/quadretopology/includes/config/gurobi.hh.in"
  "${CMAKE_CURRENT_BINARY_DIR}/quadretopology/includes/config/gurobi.hh"
)

add_library(quadretopology
    #    quadretopology/includes/qr_charts.cpp
    #    quadretopology/includes/qr_convert.cpp
    #    quadretopology/includes/qr_patterns.cpp
    #    quadretopology/includes/qr_utils.cpp
        quadretopology/includes/qr_mapping.cpp
        quadretopology/qr_flow.cpp
        quadretopology/qr_eval_quantization.cpp
        quadretopology/qr_singularity_pairs.cpp
        quadretopology/quadretopology.cpp

        patterns/patterns/generate_patch.cpp
        patterns/patterns/determine_geometry.cpp
        patterns/patterns/patchgen/adjust_parameter.cpp
        patterns/patterns/patchgen/get_boundary_geometry.cpp
        patterns/patterns/patchgen/get_constraint_matrix.cpp
        patterns/patterns/patchgen/get_constraint_rhs.cpp
        patterns/patterns/patchgen/get_default_parameter.cpp
        patterns/patterns/patchgen/get_param_str.cpp
        patterns/patterns/patchgen/get_variable.cpp
        patterns/patterns/patchgen/get_variable_indicators.cpp
        patterns/patterns/patchgen/switch_pattern.cpp
    )

if(WITH_GUROBI)
    message(STATUS "Building QuadRetopology WITH Gurobi.")
    target_sources(quadretopology PRIVATE quadretopology/includes/qr_ilp.cpp)
    target_link_libraries(quadretopology PUBLIC Gurobi::GurobiCXX)
else()
    target_sources(quadretopology PRIVATE quadretopology/includes/qr_ilp_nogurobi.cpp)
    message(STATUS "Building QuadRetopology WITHOUT Gurobi.")
endif()


target_include_directories(quadretopology PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")
target_include_directories(quadretopology PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/")
target_include_directories(quadretopology PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/patterns")
target_include_directories(quadretopology PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/patterns/patterns")

target_link_libraries(quadretopology PUBLIC satsuma::satsuma)
target_link_libraries(quadretopology PUBLIC Timekeeper::libTimekeeper)
target_link_libraries(quadretopology PUBLIC Eigen3::Eigen)
target_link_libraries(quadretopology PUBLIC libigl::libigl)
target_link_libraries(quadretopology PUBLIC vcglib::vcglib)
target_link_libraries(quadretopology PUBLIC lpsolve::lpsolve)
target_link_libraries(quadretopology PUBLIC OpenMeshCore)
target_link_libraries(quadretopology PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(quadretopology PUBLIC Boost::boost)

add_library(quadwild::quadretopology ALIAS quadretopology)
