#[[
if (NOT TARGET Qt5::Widgets)
    find_package(Qt5 COMPONENTS Widgets OpenGL)
endif()

if (NOT TARGET OpenGL::OpenGL)
    find_package(OpenGL)
endif()
if (NOT TARGET Qt5::Widgets)
    message(WARNING "required Qt5 packages not found, skipping  field_tracing binary")
else()
    add_executable(field_tracing
            glwidget.cpp
            glwidget.h
            main.cpp
    )
    set(CMAKE_AUTOMOC ON)

    target_include_directories(field_tracing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(field_tracing PUBLIC Qt5::Widgets)
    target_link_libraries(field_tracing PUBLIC Qt5::OpenGL)
    target_link_libraries(field_tracing PUBLIC OpenGL::OpenGL)
    target_link_libraries(field_tracing PUBLIC AntTweakBar)
#=]]
    add_executable(field_tracing_cli
            cli.cpp
    )
    target_link_libraries(field_tracing PUBLIC quadwild::xfield_tracer)
    target_link_libraries(field_tracing PUBLIC vcglib::vcglib)
    target_link_libraries(field_tracing PUBLIC libigl::libigl)
    target_link_libraries(field_tracing PUBLIC Eigen3::Eigen)


#target_link_libraries(field_tracing PUBLIC Timekeeper::libTimekeeper)
#target_link_libraries(field_tracing PUBLIC quadwild::quadretopology)

#target_link_libraries(quad_from_patches quadwild::quad_from_patches nlohmann_json::nlohmann_json)
endif()
