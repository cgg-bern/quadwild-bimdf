add_library(lib_field_computation INTERFACE) #fields/n_polyvector.cpp fields/polyroots.cpp)
target_compile_definitions(lib_field_computation INTERFACE COMISO_FIELD)
target_include_directories(lib_field_computation INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_field_computation INTERFACE libigl::libigl)
target_link_libraries(lib_field_computation INTERFACE Eigen3::Eigen)
target_link_libraries(lib_field_computation INTERFACE CoMISo::CoMISo)
add_library(quadwild::lib_field_computation ALIAS lib_field_computation)

# TODO: needs qt:
#add_executable(field_computation main.cpp)
#target_link_libraries(field_computation PRIVATE lib_field_computation)
