add_executable(quadwild quadwild.cpp trace.cpp)
target_link_libraries(quadwild PRIVATE quadwild::quadretopology)
target_link_libraries(quadwild PRIVATE quadwild::lib_field_computation)
target_link_libraries(quadwild PRIVATE quadwild::xfield_tracer)
target_link_libraries(quadwild PRIVATE quadwild::quad_from_patches)

add_executable(cli_trace cli_trace.cpp trace.cpp)
target_link_libraries(cli_trace PRIVATE quadwild::xfield_tracer)

