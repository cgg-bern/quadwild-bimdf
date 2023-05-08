add_executable(viz_mesh_results
        main.cpp
)
target_link_libraries(viz_mesh_results vcglib::vcglib)
target_link_libraries(viz_mesh_results libigl::libigl)
