INCLUDEPATH += $$PWD

#Include patterns
include($$PWD/patterns/patterns.pri)

SOURCES += \
        $$PWD/quadretopology/includes/qr_charts.cpp \
        $$PWD/quadretopology/includes/qr_convert.cpp \
        $$PWD/quadretopology/includes/qr_ilp.cpp \
        $$PWD/quadretopology/includes/qr_patterns.cpp \
        $$PWD/quadretopology/includes/qr_mapping.cpp \
        $$PWD/quadretopology/includes/qr_utils.cpp \
        $$PWD/quadretopology/qr_eval_quantization.cpp \
        $$PWD/quadretopology/qr_singularity_pairs.cpp \
        $$PWD/quadretopology/quadretopology.cpp \
        $$PWD/quadretopology/qr_flow.cpp

HEADERS += \
        $$PWD/quadretopology/includes/qr_convert.h \
        $$PWD/quadretopology/includes/qr_charts.h \
        $$PWD/quadretopology/includes/qr_convert.h \
        $$PWD/quadretopology/includes/qr_ilp.h \
        $$PWD/quadretopology/includes/qr_parameters.h \
        $$PWD/quadretopology/includes/qr_patterns.h \
        $$PWD/quadretopology/includes/qr_mapping.h \
        $$PWD/quadretopology/includes/qr_utils.h \
        $$PWD/quadretopology/qr_eval_quantization.h \
        $$PWD/quadretopology/qr_singularity_pairs.h \
        $$PWD/quadretopology/quadretopology.h \
        $$PWD/quadretopology/qr_flow.h


# satsuma TODO: this is quite the hack:
LIBS += -L$$SATSUMA_PATH/build/src/ -lsatsuma
LIBS += -L$$SATSUMA_PATH/build/_deps/lemon-build/lemon -lemon
LIBS += -L$$SATSUMA_PATH/build/_deps/blossom5-build/ -lBlossom5
INCLUDEPATH += $$SATSUMA_PATH/src
INCLUDEPATH += $$SATSUMA_PATH/external/lemon
INCLUDEPATH += $$SATSUMA_PATH/build/_deps/lemon-build/ # for lemon/config.h

INCLUDEPATH += $$TIMEKEEPER_PATH/src
