############################ PROJECT FILES ############################

include(libs.pri)

HEADERS = \
    glwidget.h \
    triangle_mesh_type.h

SOURCES = \
    glwidget.cpp \
    main.cpp

DEFINES += GLEW_STATIC
DEFINES += INCLUDE_TEMPLATES
DEFINES += COMISO_FIELD


############################ TARGET ############################

#App config
TARGET = field_computation

TEMPLATE = app
CONFIG += qt
CONFIG += c++11
CONFIG -= app_bundle

QT += core gui opengl xml widgets

#Debug/release optimization flags
CONFIG(debug, debug|release){
    DEFINES += DEBUG
}
CONFIG(release, debug|release){
    DEFINES -= DEBUG
    #just uncomment next line if you want to ignore asserts and got a more optimized binary
    CONFIG += FINAL_RELEASE
}

#Final release optimization flag
FINAL_RELEASE {
    unix:!macx{
        QMAKE_CXXFLAGS_RELEASE -= -g -O2
        QMAKE_CXXFLAGS += -O3 -DNDEBUG
    }
}

macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.13
    QMAKE_MAC_SDK = macosx10.13
}


############################ INCLUDES ############################

#vcglib
INCLUDEPATH += $$VCGLIBPATH
SOURCES += $$VCGLIBPATH/wrap/ply/plylib.cpp
SOURCES += $$VCGLIBPATH/wrap/gui/trackball.cpp
SOURCES += $$VCGLIBPATH/wrap/gui/trackmode.cpp
SOURCES += $$VCGLIBPATH/wrap/qt/anttweakbarMapperNew.cpp

#eigen
INCLUDEPATH += $$EIGENPATH

#libigl
INCLUDEPATH += $$LIBIGLPATH/include
HEADERS += \
    $$LIBIGLPATH/include/igl/principal_curvature.h \
    $$LIBIGLPATH/include/igl/copyleft/comiso/nrosy.h
SOURCES += \
    $$LIBIGLPATH/include/igl/principal_curvature.cpp \
    $$LIBIGLPATH/include/igl/copyleft/comiso/nrosy.cpp

#AntTweakBar
INCLUDEPATH += $$ANTTWEAKBARPATH/include
LIBS += -L$$ANTTWEAKBARPATH/lib -lAntTweakBar
win32{ # Awful problem with windows..
    DEFINES += NOMINMAX
}

#glew
LIBS += -lGLU
INCLUDEPATH += $$GLEWPATH/include
SOURCES += $$GLEWPATH/src/glew.c

contains(DEFINES, COMISO_FIELD) {
    #comiso
    LIBS += -L$$COMISOPATH/build/Build/lib/CoMISo/ -lCoMISo
    INCLUDEPATH += $$COMISOPATH/..

    #gmm (we have to use comiso gmm)
    INCLUDEPATH += $$GMMPATH/include
}

# Mac specific Config required to avoid to make application bundles
macx{
#    CONFIG -= app_bundle
#    LIBS += $$ANTTWEAKBARPATH/lib/libAntTweakBar.dylib
    QMAKE_POST_LINK +="cp -P ../../../code/lib/AntTweakBar1.16/lib/libAntTweakBar.dylib . ; "
    QMAKE_POST_LINK +="install_name_tool -change ../lib/libAntTweakBar.dylib ./libAntTweakBar.dylib $$TARGET ; "
#    QMAKE_POST_LINK +="install_name_tool -change libCoMISo.dylib $$COMISOPATH/build/Build/lib/CoMISo/libCoMISo.dylib $$TARGET ;"
    DEPENDPATH += .
#    DEPENDPATH += $$COMISOPATH/build/Build/lib/CoMISo
#    INCLUDEPATH += $$COMISOPATH/build/Build/lib/CoMISo
}
