QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    evaluationengine.cpp \
    formulaparser.cpp \
    helpers.cpp \
    lumen-src/compiler.cpp \
    lumen-src/compiler_math.cpp \
    lumen-src/lumen_helpers.cpp \
    lumen-src/programfile.cpp \
    lumen-src/tokenizer.cpp \
    lumen-src/vm.cpp \
    lumen-src/vmfuncmap.cpp \
    main.cpp \
    operations.cpp \
    saveload.cpp \
    scriptingpanel.cpp \
    smartfill.cpp \
    styles.cpp \
    tablemodel.cpp \
    widget.cpp \
    about.cpp

HEADERS += \
    evaluationengine.h \
    global.h \
    lumen-inc/compiler.h \
    lumen-inc/helpers.h \
    lumen-inc/includes.h \
    lumen-inc/programfile.h \
    lumen-inc/tokenizer.h \
    lumen-inc/types.h \
    lumen-inc/vm.h \
    scriptingpanel.h \
    tablemodel.h \
    widget.h \
    about.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
